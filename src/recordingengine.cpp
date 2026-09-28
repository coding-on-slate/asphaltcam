#include "recordingengine.h"

#include "appsettings.h"
#include "clipstorage.h"
#include "subtitlelog.h"

#include <QMutexLocker>
#include <QUrl>

RecordingEngine::RecordingEngine(AppSettings *settings,
                                 ClipStorage *storage,
                                 SubtitleLog *subtitles,
                                 QObject *parent)
    : QObject(parent)
    , m_settings(settings)
    , m_storage(storage)
    , m_subtitles(subtitles)
    , m_recording(false)
    , m_previewing(false)
    , m_elapsedSeconds(0)
    , m_rotatingSegment(false)
    , m_finalizeThenFlush(false)
    , m_flushToken(0)
    , m_finalizeAttempts(0)
{
    m_elapsedTimer.setInterval(1000);
    connect(&m_elapsedTimer, SIGNAL(timeout()), this, SLOT(tickElapsed()));
    connect(&m_segmentTimer, SIGNAL(timeout()), this, SLOT(rotateSegment()));
    m_incidentWatchdog.setSingleShot(true);
    connect(&m_incidentWatchdog, SIGNAL(timeout()), this, SLOT(onIncidentWatchdog()));
    if (m_storage) {
        connect(m_storage, &ClipStorage::incidentBatchReady, this,
                [this](const QStringList &, const QString &) {
            m_incidentWatchdog.stop();
        });
    }
}

void RecordingEngine::setError(const QString &error)
{
    if (m_errorString == error)
        return;
    m_errorString = error;
    emit errorStringChanged();
}

QString RecordingEngine::currentFile() const
{
    QMutexLocker locker(&m_fileMutex);
    return m_currentFile;
}

QUrl RecordingEngine::outputUrl() const
{
    return QUrl::fromLocalFile(currentFile());
}

void RecordingEngine::beginRecordingFile()
{
    const QString next = m_storage->nextLoopFile();
    {
        QMutexLocker locker(&m_fileMutex);
        m_currentFile = next;
    }
    if (m_subtitles)
        m_subtitles->beginFragment(next);
    emit currentFileChanged();
    emit nativeRecordStart();
    if (m_storage && m_storage->incidentCapturing())
        m_storage->setPendingCurrent(next);
}

bool RecordingEngine::startPreview()
{
    if (m_recording || m_previewing)
        return true;
    setError(QString());
    m_previewing = true;
    emit nativePreviewStart();
    emit previewingChanged();
    return true;
}

void RecordingEngine::stopPreview()
{
    if (m_recording)
        return;
    emit nativePreviewStop();
    m_previewing = false;
    emit previewingChanged();
}

bool RecordingEngine::startRecording()
{
    if (m_recording)
        return true;
    setError(QString());
    if (m_storage)
        m_storage->clearIncidentStatus();
    if (!m_previewing)
        startPreview();
    m_storage->clearLoop();
    m_rotatingSegment = false;
    m_recording = true;
    m_previewing = true;
    m_elapsedSeconds = 0;
    m_recordClock.start();
    m_elapsedTimer.start();
    m_segmentTimer.start(m_settings->segmentSeconds() * 1000);
    beginRecordingFile();
    emit recordingChanged();
    emit previewingChanged();
    emit elapsedSecondsChanged();
    return true;
}

void RecordingEngine::stopRecording()
{
    if (!m_recording)
        return;
    m_recording = false;
    m_segmentTimer.stop();
    m_elapsedTimer.stop();
    m_incidentWatchdog.stop();
    m_rotatingSegment = false;
    if (m_storage && !m_storage->incidentBusy())
        m_storage->clearIncidentStatus();
    emit nativeRecordStop();
    emit recordingChanged();
}

void RecordingEngine::nativeRecorderStopped()
{
    {
        QMutexLocker locker(&m_fileMutex);
        m_stoppedFile = m_currentFile;
        if (!m_recording || !m_rotatingSegment)
            m_currentFile.clear();
    }
    m_finalizeAttempts = 0;
    continueAfterRecorderStop();
}

void RecordingEngine::continueAfterRecorderStop()
{
    const QString finished = m_stoppedFile;
    if (!finished.isEmpty()
            && !ClipStorage::mediaLooksComplete(finished)
            && m_finalizeAttempts < 50) {
        ++m_finalizeAttempts;
        QTimer::singleShot(200, this, SLOT(continueAfterRecorderStop()));
        return;
    }

    m_stoppedFile.clear();
    if (!finished.isEmpty()) {
        if (m_subtitles)
            m_subtitles->finishFragment(finished);
        m_storage->onSegmentCompleted(finished);
    }
    if (!m_recording || m_finalizeThenFlush)
        m_storage->flushIncident(true);
    emit currentFileChanged();

    if (m_finalizeThenFlush) {
        m_finalizeThenFlush = false;
        ++m_flushToken;
        m_rotatingSegment = false;
        if (m_recording) {
            beginRecordingFile();
            m_segmentTimer.start(m_settings->segmentSeconds() * 1000);
        }
        return;
    }

    if (m_recording && m_rotatingSegment) {
        m_rotatingSegment = false;
        beginRecordingFile();
    }
}

void RecordingEngine::rotateSegment()
{
    if (!m_recording)
        return;
    if (m_storage && m_storage->incidentCapturing())
        return;
    m_rotatingSegment = true;
    emit nativeRecordStop();
}

void RecordingEngine::onIncidentWatchdog()
{
    m_finalizeThenFlush = true;
    m_rotatingSegment = false;
    ++m_flushToken;
    const int token = m_flushToken;
    emit nativeRecordStop();
    QTimer::singleShot(5000, this, [this, token]() {
        if (token != m_flushToken)
            return;
        flushPendingIncident();
    });
}

void RecordingEngine::flushPendingIncident()
{
    if (!m_stoppedFile.isEmpty())
        return;
    if (m_finalizeThenFlush)
        m_finalizeThenFlush = false;
    QString current;
    {
        QMutexLocker locker(&m_fileMutex);
        current = m_currentFile;
    }
    if (m_subtitles && !current.isEmpty())
        m_subtitles->finishFragment(current);
    m_storage->flushIncident(true);
}

void RecordingEngine::saveClip()
{
    if (!m_recording || !m_storage || m_storage->incidentBusy())
        return;
    m_segmentTimer.stop();
    const QString current = currentFile();
    m_storage->beginIncident(current, !m_rotatingSegment);
    m_incidentWatchdog.start(m_settings->postIncidentSeconds() * 1000);
}

void RecordingEngine::tickElapsed()
{
    if (!m_recordClock.isValid())
        return;
    m_elapsedSeconds = m_recordClock.elapsed() / 1000;
    emit elapsedSecondsChanged();
}
