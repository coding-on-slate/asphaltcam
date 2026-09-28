#include "subtitlelog.h"
#include "telemetry.h"

#include <QFile>
#include <QFileInfo>
#include <QList>
#include <QMutexLocker>
#include <QRegExp>
#include <QTextStream>

static QString asphaltcam_srt_time(qint64 ms)
{
    if (ms < 0)
        ms = 0;
    const int h = int(ms / 3600000);
    const int m = int((ms / 60000) % 60);
    const int s = int((ms / 1000) % 60);
    const int milli = int(ms % 1000);
    return QStringLiteral("%1:%2:%3,%4")
            .arg(h, 2, 10, QChar('0'))
            .arg(m, 2, 10, QChar('0'))
            .arg(s, 2, 10, QChar('0'))
            .arg(milli, 3, 10, QChar('0'));
}

static qint64 asphaltcam_parse_srt_time(const QString &value)
{
    const QRegExp re(QStringLiteral("(\\d+):(\\d+):(\\d+)[,.](\\d+)"));
    if (!re.exactMatch(value.trimmed()))
        return -1;
    return re.cap(1).toLongLong() * 3600000
            + re.cap(2).toLongLong() * 60000
            + re.cap(3).toLongLong() * 1000
            + re.cap(4).left(3).toLongLong();
}

SubtitleLog::SubtitleLog(Telemetry *telemetry, QObject *parent)
    : QObject(parent)
    , m_telemetry(telemetry)
{
    if (m_telemetry)
        connect(m_telemetry, SIGNAL(updated()), this, SLOT(onTelemetryUpdated()));
}

static QList<SubtitleLog::SrtCue> asphaltcam_parse_srt_text(const QString &content)
{
    QList<SubtitleLog::SrtCue> cues;
    const QStringList blocks = content.split(QRegExp(QStringLiteral("\\n\\s*\\n")), QString::SkipEmptyParts);
    foreach (const QString &block, blocks) {
        const QStringList lines = block.split(QRegExp(QStringLiteral("\\r?\\n")), QString::SkipEmptyParts);
        int timeLine = -1;
        qint64 start = -1;
        qint64 end = -1;
        for (int line = 0; line < lines.size(); ++line) {
            const int arrow = lines.at(line).indexOf(QStringLiteral("-->"));
            if (arrow < 0)
                continue;
            start = asphaltcam_parse_srt_time(lines.at(line).left(arrow));
            end = asphaltcam_parse_srt_time(lines.at(line).mid(arrow + 3));
            timeLine = line;
            break;
        }
        if (timeLine < 0 || start < 0 || end < 0)
            continue;
        QStringList textLines;
        for (int line = timeLine + 1; line < lines.size(); ++line)
            textLines.append(lines.at(line));
        if (textLines.isEmpty())
            continue;
        SubtitleLog::SrtCue cue;
        cue.startMs = start;
        cue.endMs = end;
        cue.text = textLines.join(QLatin1Char('\n'));
        cues.append(cue);
    }
    return cues;
}

QString SubtitleLog::textAt(const QString &srtPath, qint64 positionMs) const
{
    if (srtPath.isEmpty() || positionMs < 0)
        return QString();
    QList<SrtCue> cues;
    {
        QMutexLocker locker(&m_mutex);
        if (m_cachedSrtPath != srtPath) {
            QFile in(srtPath);
            m_cachedSrtPath = srtPath;
            m_cachedCues.clear();
            if (in.open(QIODevice::ReadOnly | QIODevice::Text))
                m_cachedCues = asphaltcam_parse_srt_text(QString::fromUtf8(in.readAll()));
        }
        cues = m_cachedCues;
    }
    for (int i = cues.size() - 1; i >= 0; --i) {
        const SrtCue &cue = cues.at(i);
        if (positionMs >= cue.startMs && positionMs < cue.endMs)
            return cue.text;
    }
    return QString();
}

QString SubtitleLog::sidecarSrt(const QString &mediaPath)
{
    const QFileInfo info(mediaPath);
    return info.absolutePath() + QLatin1Char('/') + info.completeBaseName() + QStringLiteral(".srt");
}

bool SubtitleLog::writeClockSrt(const QString &path, qint64 durationMs, const QDateTime &endTime,
                               const QString &speedLabel)
{
    if (path.isEmpty())
        return false;
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        return false;
    if (durationMs < 1000)
        durationMs = 1000;
    QTextStream stream(&file);
    stream.setCodec("UTF-8");
    int index = 1;
    for (qint64 t = 0; t < durationMs; t += 1000) {
        qint64 end = t + 1000;
        if (end > durationMs)
            end = durationMs;
        const QDateTime when = endTime.addMSecs(t - durationMs);
        stream << index++ << '\n';
        stream << asphaltcam_srt_time(t) << " --> " << asphaltcam_srt_time(end) << '\n';
        stream << Telemetry::formatOverlay(QStringLiteral("--"), speedLabel, when) << "\n\n";
    }
    return true;
}

void SubtitleLog::beginFragment(const QString &path)
{
    QMutexLocker locker(&m_mutex);
    m_path = path;
    m_start = QDateTime::currentDateTime();
    m_cues.clear();
    if (m_telemetry && !m_telemetry->overlayText().isEmpty()) {
        Cue cue;
        cue.startMs = 0;
        cue.text = m_telemetry->overlayText();
        m_cues.append(cue);
    }
}

void SubtitleLog::finishFragment(const QString &path)
{
    QList<Cue> cues;
    qint64 durationMs = 0;
    QString fallback;
    {
        QMutexLocker locker(&m_mutex);
        if (path.isEmpty() || m_path != path)
            return;
        cues = m_cues;
        durationMs = m_start.isValid() ? m_start.msecsTo(QDateTime::currentDateTime()) : 0;
        m_path.clear();
        m_cues.clear();
    }
    if (cues.isEmpty() && m_telemetry)
        fallback = m_telemetry->overlayText();
    if (cues.isEmpty() && !fallback.isEmpty()) {
        Cue cue;
        cue.startMs = 0;
        cue.text = fallback;
        cues.append(cue);
    }
    writeCues(path, cues, durationMs);
}

void SubtitleLog::onTelemetryUpdated()
{
    if (!m_telemetry)
        return;
    QMutexLocker locker(&m_mutex);
    if (m_path.isEmpty() || !m_start.isValid())
        return;
    Cue cue;
    cue.startMs = qMax(qint64(0), m_start.msecsTo(QDateTime::currentDateTime()));
    cue.text = m_telemetry->overlayText();
    if (!m_cues.isEmpty() && m_cues.last().text == cue.text
            && cue.startMs - m_cues.last().startMs < 400)
        return;
    m_cues.append(cue);
}

QString SubtitleLog::writeCues(const QString &mediaPath, const QList<Cue> &cues, qint64 durationMs) const
{
    if (mediaPath.isEmpty() || cues.isEmpty())
        return QString();
    const QString path = sidecarSrt(mediaPath);
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        return QString();
    if (durationMs < 1000)
        durationMs = 1000;
    QTextStream stream(&file);
    stream.setCodec("UTF-8");
    for (int i = 0; i < cues.size(); ++i) {
        qint64 start = cues.at(i).startMs;
        qint64 end = (i + 1 < cues.size()) ? cues.at(i + 1).startMs : durationMs;
        if (end <= start)
            end = start + 1000;
        stream << (i + 1) << '\n';
        stream << asphaltcam_srt_time(start) << " --> " << asphaltcam_srt_time(end) << '\n';
        stream << cues.at(i).text << "\n\n";
    }
    return path;
}
