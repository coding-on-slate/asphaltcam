#ifndef SUBTITLELOG_H
#define SUBTITLELOG_H

#include <QDateTime>
#include <QList>
#include <QMutex>
#include <QObject>
#include <QString>

class Telemetry;

class SubtitleLog : public QObject
{
    Q_OBJECT
public:
    explicit SubtitleLog(Telemetry *telemetry, QObject *parent = nullptr);

    static QString sidecarSrt(const QString &mediaPath);
    static bool writeClockSrt(const QString &path, qint64 durationMs, const QDateTime &endTime,
                              const QString &speedLabel);
    Q_INVOKABLE QString textAt(const QString &srtPath, qint64 positionMs) const;

    struct SrtCue {
        qint64 startMs;
        qint64 endMs;
        QString text;
    };

    void beginFragment(const QString &path);
    void finishFragment(const QString &path);

private slots:
    void onTelemetryUpdated();

private:
    struct Cue {
        qint64 startMs;
        QString text;
    };

    QString writeCues(const QString &mediaPath, const QList<Cue> &cues, qint64 durationMs) const;

    Telemetry *m_telemetry;
    mutable QMutex m_mutex;
    QString m_path;
    QDateTime m_start;
    QList<Cue> m_cues;
    mutable QString m_cachedSrtPath;
    mutable QList<SrtCue> m_cachedCues;
};

#endif
