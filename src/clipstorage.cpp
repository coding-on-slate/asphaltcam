#include "clipstorage.h"
#include "appsettings.h"
#include "subtitlelog.h"

#include <algorithm>
#include <cstring>

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutexLocker>
#include <QPair>
#include <QtEndian>

static qint64 asphaltcam_loop_epoch_ms(const QString &path)
{
    return QFileInfo(path).completeBaseName().toLongLong();
}

static const quint32 kAsphaltcamMp4Matrix[4][9] = {
    { 0x00010000, 0, 0, 0, 0x00010000, 0, 0, 0, 0x40000000 },
    { 0, 0x00010000, 0, 0xFFFF0000, 0, 0, 0, 0, 0x40000000 },
    { 0xFFFF0000, 0, 0, 0, 0xFFFF0000, 0, 0, 0, 0x40000000 },
    { 0, 0xFFFF0000, 0, 0x00010000, 0, 0, 0, 0, 0x40000000 }
};

static bool asphaltcam_patch_tkhd(QFile *file, qint64 payload, qint64 boxEnd, int degrees)
{
    if (!file->seek(payload))
        return false;
    const QByteArray versionFlags = file->read(4);
    if (versionFlags.size() != 4)
        return false;
    const int version = uchar(versionFlags.at(0));
    const qint64 matrixOff = payload + (version == 1 ? 52 : 40);
    if (matrixOff + 44 > boxEnd)
        return false;
    if (!file->seek(matrixOff + 36))
        return false;
    const QByteArray wh = file->read(8);
    if (wh.size() != 8)
        return false;
    const uchar *wp = reinterpret_cast<const uchar *>(wh.constData());
    if (qFromBigEndian<quint32>(wp) == 0 && qFromBigEndian<quint32>(wp + 4) == 0)
        return false;
    const int idx = ((degrees / 90) % 4 + 4) % 4;
    char matrix[36];
    for (int i = 0; i < 9; ++i)
        qToBigEndian(kAsphaltcamMp4Matrix[idx][i], reinterpret_cast<uchar *>(matrix + i * 4));
    if (!file->seek(matrixOff))
        return false;
    return file->write(matrix, 36) == 36;
}

static bool asphaltcam_walk_mp4(QFile *file, qint64 start, qint64 end, int degrees, int depth)
{
    if (depth > 8)
        return false;
    bool patched = false;
    qint64 pos = start;
    while (pos + 8 <= end) {
        if (!file->seek(pos))
            return patched;
        const QByteArray hdr = file->read(8);
        if (hdr.size() != 8)
            return patched;
        const uchar *h = reinterpret_cast<const uchar *>(hdr.constData());
        const quint32 size32 = qFromBigEndian<quint32>(h);
        char type[5];
        memcpy(type, hdr.constData() + 4, 4);
        type[4] = 0;
        qint64 header = 8;
        qint64 boxSize = size32;
        if (size32 == 1) {
            const QByteArray ext = file->read(8);
            if (ext.size() != 8)
                return patched;
            boxSize = static_cast<qint64>(
                        qFromBigEndian<quint64>(reinterpret_cast<const uchar *>(ext.constData())));
            header = 16;
        } else if (size32 == 0) {
            boxSize = end - pos;
        }
        if (boxSize < header || pos + boxSize > end)
            return patched;
        const qint64 payload = pos + header;
        const qint64 boxEnd = pos + boxSize;
        if (!strcmp(type, "moov") || !strcmp(type, "trak")) {
            if (asphaltcam_walk_mp4(file, payload, boxEnd, degrees, depth + 1))
                patched = true;
        } else if (!strcmp(type, "tkhd")) {
            if (asphaltcam_patch_tkhd(file, payload, boxEnd, degrees))
                patched = true;
        }
        pos = boxEnd;
    }
    return patched;
}

static bool asphaltcam_set_mp4_rotation(const QString &path, int degrees)
{
    degrees = ((degrees % 360) + 360) % 360;
    if (degrees % 90 != 0)
        return false;
    QFile file(path);
    if (!file.open(QIODevice::ReadWrite))
        return false;
    const bool ok = asphaltcam_walk_mp4(&file, 0, file.size(), degrees, 0);
    file.close();
    return ok;
}

static QString asphaltcam_incident_dir(const QString &path)
{
    const QFileInfo info(path);
    if (info.isFile())
        return info.absolutePath();
    return QDir(path).absolutePath();
}

ClipStorage::ClipStorage(AppSettings *settings, QObject *parent)
    : QObject(parent)
    , m_settings(settings)
    , m_collecting(false)
    , m_fileOrientation(0)
{
}

QString ClipStorage::nextLoopFile()
{
    QMutexLocker locker(&m_mutex);
    const QString name = QString::number(QDateTime::currentMSecsSinceEpoch()) + QStringLiteral(".mp4");
    return m_settings->loopPath() + QLatin1Char('/') + name;
}

void ClipStorage::clearLoop()
{
    const QString dirPath = m_settings->loopPath();
    QDir dir(dirPath);
    const QFileInfoList infos = dir.entryInfoList(QDir::Files);
    foreach (const QFileInfo &info, infos) {
        const QString path = info.absoluteFilePath();
        removeSidecar(path);
        QFile::remove(path);
    }
}

bool ClipStorage::incidentBusy() const
{
    QMutexLocker locker(&m_mutex);
    return m_collecting || !m_outMp4.isEmpty();
}

bool ClipStorage::incidentCapturing() const
{
    QMutexLocker locker(&m_mutex);
    return m_collecting;
}

QString ClipStorage::incidentStatus() const
{
    QMutexLocker locker(&m_mutex);
    return m_incidentStatus;
}

void ClipStorage::setIncidentStatus(const QString &status)
{
    {
        QMutexLocker locker(&m_mutex);
        if (m_incidentStatus == status)
            return;
        m_incidentStatus = status;
    }
    emit incidentStatusChanged();
}

void ClipStorage::clearIncidentStatus()
{
    setIncidentStatus(QString());
}

void ClipStorage::setPendingCurrent(const QString &path)
{
    QMutexLocker locker(&m_mutex);
    if (!m_collecting)
        return;
    m_pendingCurrent = path;
}

static bool asphaltcam_scan_moov(QFile &file, qint64 start, qint64 end)
{
    const qint64 fileSize = file.size();
    if (start < 0)
        start = 0;
    if (end > fileSize)
        end = fileSize;
    if (end - start < 8)
        return false;
    if (!file.seek(start))
        return false;
    const QByteArray buf = file.read(end - start);
    const char *data = buf.constData();
    for (int i = 0; i + 8 <= buf.size(); ++i) {
        if (qstrncmp(data + i + 4, "moov", 4) != 0)
            continue;
        const uchar *p = reinterpret_cast<const uchar *>(data + i);
        const quint32 size32 = (quint32(p[0]) << 24) | (quint32(p[1]) << 16)
                | (quint32(p[2]) << 8) | quint32(p[3]);
        const qint64 absOff = start + i;
        if (size32 == 1 && absOff + 16 <= fileSize)
            return true;
        if (size32 >= 8 && absOff + qint64(size32) <= fileSize)
            return true;
    }
    return false;
}

bool ClipStorage::mediaLooksComplete(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return false;
    const qint64 fileSize = file.size();
    if (fileSize < 65536)
        return false;
    // Qt Camera often leaves mdat size=0 and writes moov at EOF. A forward
    // box walk then swallows the index, so also scan the tail.
    if (asphaltcam_scan_moov(file, 0, qMin(fileSize, qint64(256 * 1024))))
        return true;
    const qint64 window = qMin(fileSize, qint64(4 * 1024 * 1024));
    return asphaltcam_scan_moov(file, fileSize - window, fileSize);
}

void ClipStorage::removeSidecar(const QString &mediaPath)
{
    QFile::remove(SubtitleLog::sidecarSrt(mediaPath));
}

void ClipStorage::beginIncident(const QString &currentPath, bool currentIsPending)
{
    QString work;
    QString out;
    {
        QMutexLocker locker(&m_mutex);
        if (m_collecting || !m_outMp4.isEmpty())
            return;
        m_collecting = true;
        m_pendingCurrent = currentIsPending ? currentPath : QString();
        m_parts.clear();
        work = m_settings->workPath() + QLatin1Char('/')
                + QString::number(QDateTime::currentMSecsSinceEpoch());
        out = m_settings->incidentsPath();
        m_workDir = work;
        m_outMp4 = out;
        QDir().mkpath(work);
    }
    emit incidentBusyChanged();
    emit incidentCapturingChanged();
    setIncidentStatus(tr("Saving incident"));
    emit incidentCaptureStarted();

    QStringList files = loopFiles();
    QStringList completed;
    foreach (const QString &path, files) {
        if (!currentPath.isEmpty() && path == currentPath)
            continue;
        completed.append(path);
    }
    const int n = m_settings->preIncidentSegments();
    const int start = qMax(0, completed.size() - n);
    for (int i = start; i < completed.size(); ++i)
        addCompletedPart(completed.at(i));
}

QString ClipStorage::copyWithSidecar(const QString &path, const QString &destDir)
{
    if (path.isEmpty() || !QFile::exists(path) || destDir.isEmpty())
        return QString();
    QDir().mkpath(destDir);
    const QFileInfo info(path);
    QString dest = destDir + QLatin1Char('/') + info.fileName();
    if (QFile::exists(dest)) {
        dest = destDir + QLatin1Char('/')
                + QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss-"))
                + info.fileName();
    }
    if (!QFile::copy(path, dest))
        return QString();
    const QString srt = SubtitleLog::sidecarSrt(path);
    if (QFile::exists(srt))
        QFile::copy(srt, SubtitleLog::sidecarSrt(dest));
    return dest;
}

void ClipStorage::addCompletedPart(const QString &path)
{
    if (path.isEmpty() || !QFile::exists(path))
        return;
    if (!mediaLooksComplete(path)) {
        qWarning() << "AsphaltCam: skip unfinalized clip" << path << QFileInfo(path).size();
        return;
    }
    QString destDir;
    {
        QMutexLocker locker(&m_mutex);
        destDir = m_workDir;
    }
    const QString dest = copyWithSidecar(path, destDir);
    if (dest.isEmpty())
        return;
    QMutexLocker locker(&m_mutex);
    m_parts.append(dest);
}

void ClipStorage::flushIncident(bool forceFinish)
{
    QString pending;
    {
        QMutexLocker locker(&m_mutex);
        if (!m_collecting)
            return;
        pending = m_pendingCurrent;
    }
    if (!pending.isEmpty()) {
        if (mediaLooksComplete(pending)) {
            addCompletedPart(pending);
        } else if (!forceFinish) {
            qWarning() << "AsphaltCam: waiting for finalized clip" << pending
                       << QFileInfo(pending).size();
            return;
        } else {
            qWarning() << "AsphaltCam: dropping unfinalized clip" << pending
                       << QFileInfo(pending).size();
        }
    }
    finishCollection();
}

void ClipStorage::finishCollection()
{
    QStringList parts;
    QString out;
    {
        QMutexLocker locker(&m_mutex);
        m_collecting = false;
        m_pendingCurrent.clear();
        parts = m_parts;
        parts.sort();
        out = m_outMp4;
    }
    emit incidentBusyChanged();
    emit incidentCapturingChanged();
    if (parts.isEmpty()) {
        {
            QMutexLocker locker(&m_mutex);
            m_outMp4.clear();
            m_workDir.clear();
        }
        setIncidentStatus(tr("Incident save failed"));
        emit incidentBusyChanged();
        return;
    }
    setIncidentStatus(tr("Processing incident"));
    emit incidentBatchReady(parts, out);
    assembleIncident(parts, out);
}

void ClipStorage::onSegmentCompleted(const QString &path)
{
    if (path.isEmpty() || !QFile::exists(path))
        return;
    if (!mediaLooksComplete(path)) {
        qWarning() << "AsphaltCam: ignore unfinalized segment" << path << QFileInfo(path).size();
        return;
    }

    bool shouldAdd = false;
    bool done = false;
    {
        QMutexLocker locker(&m_mutex);
        if (m_collecting) {
            if (!m_pendingCurrent.isEmpty() && path == m_pendingCurrent) {
                shouldAdd = true;
                m_pendingCurrent.clear();
                done = true;
            } else if (m_pendingCurrent.isEmpty()) {
                shouldAdd = true;
            }
        }
    }
    if (shouldAdd)
        addCompletedPart(path);
    pruneLoop();
    if (done)
        finishCollection();
}

void ClipStorage::assembleIncident(const QStringList &partMp4s, const QString &outMp4)
{
    QStringList existing;
    foreach (const QString &path, partMp4s) {
        if (mediaLooksComplete(path))
            existing.append(path);
        else
            qWarning() << "AsphaltCam: skip unplayable part" << path;
    }
    if (existing.isEmpty() || outMp4.isEmpty()) {
        finishIncident(false, tr("No clips to save."), QString());
        return;
    }

    const QDateTime start = QDateTime::fromMSecsSinceEpoch(
                asphaltcam_loop_epoch_ms(existing.first()));
    const QDateTime end = QDateTime::currentDateTime();
    const QString stamp = start.toString(QStringLiteral("yyyyMMdd-HHmmss")) + QLatin1Char('-')
            + end.toString(QStringLiteral("HHmmss"));
    const QString destDir = QDir(outMp4).filePath(stamp);
    QDir().mkpath(destDir);
    int fileRotation = 0;
    {
        QMutexLocker locker(&m_mutex);
        fileRotation = m_fileOrientation;
    }

    QStringList destMp4s;
    const int seg = m_settings ? m_settings->segmentSeconds() : 30;
    const bool kmh = !m_settings || m_settings->speedUnitKmh();
    const QString speed = kmh ? QStringLiteral("0 km/h") : QStringLiteral("0 mph");
    for (int i = 0; i < existing.size(); ++i) {
        const QString dest = destDir + QLatin1Char('/')
                + QStringLiteral("%1.mp4").arg(i + 1, 3, 10, QChar('0'));
        const QString destSrt = SubtitleLog::sidecarSrt(dest);
        QFile::remove(dest);
        QFile::remove(destSrt);
        if (!QFile::copy(existing.at(i), dest) || !mediaLooksComplete(dest)) {
            QFile::remove(dest);
            qWarning() << "AsphaltCam: could not copy incident part" << existing.at(i);
            continue;
        }
        if (!asphaltcam_set_mp4_rotation(dest, fileRotation))
            qWarning() << "AsphaltCam: could not tag MP4 rotation" << dest << fileRotation;
        const QString srcSrt = SubtitleLog::sidecarSrt(existing.at(i));
        if (QFile::exists(srcSrt))
            QFile::copy(srcSrt, destSrt);
        if (!QFile::exists(destSrt) || QFileInfo(destSrt).size() == 0)
            SubtitleLog::writeClockSrt(destSrt, qint64(seg) * 1000,
                                       QFileInfo(dest).lastModified(), speed);
        destMp4s.append(dest);
    }

    if (destMp4s.isEmpty()) {
        finishIncident(false, tr("Could not save the incident clip."), QString());
        return;
    }

    foreach (const QString &path, existing) {
        QFile::remove(SubtitleLog::sidecarSrt(path));
        QFile::remove(path);
    }
    if (!existing.isEmpty()) {
        QDir workDir(QFileInfo(existing.first()).absolutePath());
        if (workDir.path().contains(QStringLiteral("/work/")))
            workDir.rmdir(workDir.path());
    }

    finishIncident(true, tr("Incident saved."), destDir);
}

void ClipStorage::finishIncident(bool ok, const QString &message, const QString &outMp4)
{
    {
        QMutexLocker locker(&m_mutex);
        m_collecting = false;
        m_pendingCurrent.clear();
        m_parts.clear();
        m_workDir.clear();
        m_outMp4.clear();
    }
    emit incidentBusyChanged();
    emit incidentCapturingChanged();
    setIncidentStatus(message);
    emit clipsChanged();
    if (ok && !outMp4.isEmpty())
        emit incidentReady();
}

void ClipStorage::pruneLoop()
{
    QStringList files = loopFiles();
    const qint64 maxMs = m_settings->loopRetentionMs();
    qint64 total = 0;
    for (int i = files.size() - 1; i >= 0; --i)
        total += estimateDurationMs(files.at(i));

    int i = 0;
    while (i < files.size() && total > maxMs) {
        const QString path = files.at(i);
        total -= estimateDurationMs(path);
        removeSidecar(path);
        QFile::remove(path);
        ++i;
    }
}

QStringList ClipStorage::loopFiles() const
{
    QDir dir(m_settings->loopPath());
    const QFileInfoList infos = dir.entryInfoList(QStringList() << QStringLiteral("*.mp4"),
                                                  QDir::Files);
    QList<QPair<qint64, QString> > items;
    foreach (const QFileInfo &info, infos) {
        if (info.size() < 65536)
            continue;
        items.append(qMakePair(asphaltcam_loop_epoch_ms(info.absoluteFilePath()),
                               info.absoluteFilePath()));
    }
    std::sort(items.begin(), items.end());
    QStringList out;
    for (int i = 0; i < items.size(); ++i)
        out.append(items.at(i).second);
    return out;
}

QStringList ClipStorage::incidentParts(const QString &path) const
{
    QDir dir(asphaltcam_incident_dir(path));
    const QFileInfoList infos = dir.entryInfoList(QStringList() << QStringLiteral("*.mp4"),
                                                  QDir::Files, QDir::Name);
    QStringList out;
    foreach (const QFileInfo &info, infos) {
        if (info.size() < 65536)
            continue;
        out.append(info.absoluteFilePath());
    }
    return out;
}

void ClipStorage::setFileOrientation(int degrees)
{
    QMutexLocker locker(&m_mutex);
    m_fileOrientation = ((degrees % 360) + 360) % 360;
}

QStringList ClipStorage::incidentFiles() const
{
    QDir dir(m_settings->incidentsPath());
    const QFileInfoList infos = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    QStringList out;
    for (int i = infos.size() - 1; i >= 0; --i) {
        const QString dirPath = infos.at(i).absoluteFilePath();
        if (incidentParts(dirPath).isEmpty())
            continue;
        out.append(dirPath);
    }
    return out;
}

bool ClipStorage::removeFile(const QString &path)
{
    const QString dirPath = asphaltcam_incident_dir(path);
    if (dirPath.isEmpty() || dirPath == QDir(m_settings->incidentsPath()).absolutePath())
        return false;
    QDir dir(dirPath);
    if (!dir.exists() || !dir.removeRecursively())
        return false;
    emit fileRemoved(path);
    return true;
}

bool ClipStorage::removeFiles(const QStringList &paths)
{
    bool ok = false;
    foreach (const QString &path, paths) {
        const QString dirPath = asphaltcam_incident_dir(path);
        if (dirPath.isEmpty() || dirPath == QDir(m_settings->incidentsPath()).absolutePath())
            continue;
        QDir dir(dirPath);
        if (dir.exists() && dir.removeRecursively())
            ok = true;
    }
    if (ok)
        emit clipsChanged();
    return ok;
}

qint64 ClipStorage::estimateDurationMs(const QString &path) const
{
    const qint64 fallback = static_cast<qint64>(m_settings->segmentSeconds()) * 1000;
    Q_UNUSED(path);
    return fallback;
}
