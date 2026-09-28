#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <QDateTime>
#include <QGeoPositionInfo>
#include <QGeoPositionInfoSource>
#include <QObject>
#include <QString>
#include <QTimer>

class AppSettings;

class Telemetry : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool valid READ valid NOTIFY updated)
    Q_PROPERTY(QString overlayText READ overlayText NOTIFY updated)
    Q_PROPERTY(QString coordinatesText READ coordinatesText NOTIFY updated)
    Q_PROPERTY(QString speedText READ speedText NOTIFY updated)
    Q_PROPERTY(QString gpsQualityText READ gpsQualityText NOTIFY updated)
    Q_PROPERTY(int gpsBars READ gpsBars NOTIFY updated)
    Q_PROPERTY(bool gpsAvailable READ gpsAvailable NOTIFY updated)

public:
    explicit Telemetry(AppSettings *settings, QObject *parent = nullptr);

    bool valid() const { return m_valid; }
    bool gpsAvailable() const;
    QString overlayText() const { return m_overlayText; }
    QString coordinatesText() const { return m_coordinatesText; }
    QString speedText() const { return m_speedText; }
    QString gpsQualityText() const { return m_gpsQualityText; }
    int gpsBars() const { return m_gpsBars; }

    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();

    static QString formatOverlay(const QString &coords, const QString &speed,
                                 const QDateTime &when);

signals:
    void updated();

private slots:
    void onPositionUpdated(const QGeoPositionInfo &info);
    void rebuildText();

private:
    AppSettings *m_settings;
    QGeoPositionInfoSource *m_source;
    bool m_valid;
    double m_latitude;
    double m_longitude;
    double m_speedMps;
    double m_horizontalAccuracyM;
    QDateTime m_fixTime;
    QString m_overlayText;
    QString m_coordinatesText;
    QString m_speedText;
    QString m_gpsQualityText;
    int m_gpsBars;
    QTimer m_clock;
};

#endif
