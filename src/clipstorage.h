#ifndef CLIPSTORAGE_H
#define CLIPSTORAGE_H

#include <QMutex>
#include <QObject>
#include <QString>
#include <QStringList>

class AppSettings;

class ClipStorage : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool incidentBusy READ incidentBusy NOTIFY incidentBusyChanged)
    Q_PROPERTY(bool incidentCapturing READ incidentCapturing NOTIFY incidentCapturingChanged)
    Q_PROPERTY(QString incidentStatus READ incidentStatus NOTIFY incidentStatusChanged)
public:
    explicit ClipStorage(AppSettings *settings, QObject *parent = nullptr);

    QString nextLoopFile();
    void beginIncident(const QString &currentPath, bool currentIsPending = true);
    void setPendingCurrent(const QString &path);
    void clearLoop();
    void flushIncident(bool forceFinish = false);
    void pruneLoop();
    Q_INVOKABLE QStringList incidentFiles() const;
    Q_INVOKABLE bool removeFile(const QString &path);
    Q_INVOKABLE bool removeFiles(const QStringList &paths);
    Q_INVOKABLE QStringList incidentParts(const QString &path) const;
    Q_INVOKABLE void setFileOrientation(int degrees);
    bool incidentBusy() const;
    bool incidentCapturing() const;
    QString incidentStatus() const;
    void clearIncidentStatus();
    static bool mediaLooksComplete(const QString &path);

public slots:
    void onSegmentCompleted(const QString &path);

signals:
    void clipsChanged();
    void fileRemoved(const QString &path);
    void incidentBusyChanged();
    void incidentCapturingChanged();
    void incidentStatusChanged();
    void incidentBatchReady(const QStringList &partMp4s, const QString &outMp4);
    void incidentCaptureStarted();
    void incidentReady();

private:
    QString copyWithSidecar(const QString &path, const QString &destDir);
    void addCompletedPart(const QString &path);
    void finishCollection();
    void assembleIncident(const QStringList &partMp4s, const QString &outMp4);
    void finishIncident(bool ok, const QString &message, const QString &outMp4);
    void setIncidentStatus(const QString &status);
    QStringList loopFiles() const;
    qint64 estimateDurationMs(const QString &path) const;
    static void removeSidecar(const QString &mediaPath);

    AppSettings *m_settings;
    bool m_collecting;
    QString m_pendingCurrent;
    QStringList m_parts;
    QString m_workDir;
    QString m_outMp4;
    QString m_incidentStatus;
    int m_fileOrientation;
    mutable QMutex m_mutex;
};

#endif
