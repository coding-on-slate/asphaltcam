#ifndef RECORDINGENGINE_H
#define RECORDINGENGINE_H

#include <QElapsedTimer>
#include <QMutex>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QUrl>

class AppSettings;
class ClipStorage;
class SubtitleLog;

class RecordingEngine : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool recording READ recording NOTIFY recordingChanged)
    Q_PROPERTY(bool previewing READ previewing NOTIFY previewingChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged)
    Q_PROPERTY(int elapsedSeconds READ elapsedSeconds NOTIFY elapsedSecondsChanged)
    Q_PROPERTY(QString currentFile READ currentFile NOTIFY currentFileChanged)
    Q_PROPERTY(QUrl outputUrl READ outputUrl NOTIFY currentFileChanged)

public:
    RecordingEngine(AppSettings *settings,
                    ClipStorage *storage,
                    SubtitleLog *subtitles,
                    QObject *parent = nullptr);

    bool recording() const { return m_recording; }
    bool previewing() const { return m_previewing; }
    QString errorString() const { return m_errorString; }
    int elapsedSeconds() const { return m_elapsedSeconds; }
    QString currentFile() const;
    QUrl outputUrl() const;

    Q_INVOKABLE bool startPreview();
    Q_INVOKABLE void stopPreview();
    Q_INVOKABLE bool startRecording();
    Q_INVOKABLE void stopRecording();
    Q_INVOKABLE void saveClip();
    Q_INVOKABLE void nativeRecorderStopped();

signals:
    void recordingChanged();
    void previewingChanged();
    void errorStringChanged();
    void elapsedSecondsChanged();
    void currentFileChanged();
    void nativePreviewStart();
    void nativePreviewStop();
    void nativeRecordStart();
    void nativeRecordStop();

private slots:
    void tickElapsed();
    void rotateSegment();
    void onIncidentWatchdog();
    void flushPendingIncident();
    void continueAfterRecorderStop();

private:
    void setError(const QString &error);
    void beginRecordingFile();

    AppSettings *m_settings;
    ClipStorage *m_storage;
    SubtitleLog *m_subtitles;

    bool m_recording;
    bool m_previewing;
    QString m_errorString;
    QString m_currentFile;
    int m_elapsedSeconds;
    QTimer m_elapsedTimer;
    QElapsedTimer m_recordClock;
    QTimer m_segmentTimer;
    QTimer m_incidentWatchdog;
    mutable QMutex m_fileMutex;
    bool m_rotatingSegment;
    bool m_finalizeThenFlush;
    int m_flushToken;
    QString m_stoppedFile;
    int m_finalizeAttempts;
};

#endif
