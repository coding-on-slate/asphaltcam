#include "telemetry.h"
#include "appsettings.h"

#include <QDateTime>
#include <QtGlobal>

Telemetry::Telemetry(AppSettings *settings, QObject *parent)
    : QObject(parent)
    , m_settings(settings)
    , m_source(QGeoPositionInfoSource::createDefaultSource(this))
    , m_valid(false)
    , m_latitude(0)
    , m_longitude(0)
    , m_speedMps(-1)
    , m_horizontalAccuracyM(-1)
    , m_gpsBars(0)
{
    if (m_source) {
        m_source->setUpdateInterval(1000);
        m_source->setPreferredPositioningMethods(QGeoPositionInfoSource::AllPositioningMethods);
        connect(m_source, SIGNAL(positionUpdated(QGeoPositionInfo)),
                this, SLOT(onPositionUpdated(QGeoPositionInfo)));
    }
    if (m_settings) {
        connect(m_settings, SIGNAL(speedUnitKmhChanged()), this, SLOT(rebuildText()));
    }
    m_clock.setInterval(1000);
    connect(&m_clock, SIGNAL(timeout()), this, SLOT(rebuildText()));
    rebuildText();
}

void Telemetry::start()
{
    if (m_source)
        m_source->startUpdates();
    m_clock.start();
    rebuildText();
}

bool Telemetry::gpsAvailable() const
{
    return m_valid && m_fixTime.isValid()
            && m_fixTime.msecsTo(QDateTime::currentDateTime()) <= 5000
            && m_horizontalAccuracyM >= 0;
}

void Telemetry::stop()
{
    m_clock.stop();
    if (m_source)
        m_source->stopUpdates();
}

QString Telemetry::formatOverlay(const QString &coords, const QString &speed,
                                 const QDateTime &when)
{
    return when.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"))
            + QStringLiteral(" | ") + coords + QStringLiteral(" | ") + speed;
}

void Telemetry::onPositionUpdated(const QGeoPositionInfo &info)
{
    if (!info.isValid())
        return;
    const QGeoCoordinate c = info.coordinate();
    m_valid = c.isValid();
    m_latitude = c.latitude();
    m_longitude = c.longitude();
    m_fixTime = info.timestamp().isValid() ? info.timestamp().toLocalTime()
                                           : QDateTime::currentDateTime();
    if (info.hasAttribute(QGeoPositionInfo::HorizontalAccuracy))
        m_horizontalAccuracyM = info.attribute(QGeoPositionInfo::HorizontalAccuracy);
    else
        m_horizontalAccuracyM = -1;
    if (info.hasAttribute(QGeoPositionInfo::GroundSpeed))
        m_speedMps = info.attribute(QGeoPositionInfo::GroundSpeed);
    else
        m_speedMps = -1;
    rebuildText();
}

void Telemetry::rebuildText()
{
    QString coords = QStringLiteral("--");
    if (m_valid) {
        coords = QString::number(m_latitude, 'f', 5) + QStringLiteral(", ")
                + QString::number(m_longitude, 'f', 5);
    }

    const bool stale = !m_fixTime.isValid()
            || m_fixTime.msecsTo(QDateTime::currentDateTime()) > 5000;
    const bool gpsOk = m_valid && !stale && m_horizontalAccuracyM >= 0;
    if (!gpsOk) {
        m_gpsQualityText = tr("None");
        m_gpsBars = 0;
        coords = QStringLiteral("--");
    } else if (m_horizontalAccuracyM > 50.0) {
        m_gpsQualityText = tr("Bad");
        m_gpsBars = 1;
    } else if (m_horizontalAccuracyM > 10.0) {
        m_gpsQualityText = tr("Good");
        m_gpsBars = 2;
    } else {
        m_gpsQualityText = tr("Excellent");
        m_gpsBars = 3;
    }

    const bool kmh = !m_settings || m_settings->speedUnitKmh();
    QString speed = QStringLiteral("--");
    if (gpsOk) {
        const double value = m_speedMps >= 0 ? m_speedMps : 0;
        speed = kmh
                ? (QString::number(value * 3.6, 'f', 0) + QStringLiteral(" km/h"))
                : (QString::number(value * 2.236936, 'f', 0) + QStringLiteral(" mph"));
    }
    m_coordinatesText = coords;
    m_speedText = speed;
    QString overlayCoords = coords;
    if (gpsOk)
        overlayCoords += QStringLiteral(" (±%1 m)").arg(qRound(m_horizontalAccuracyM));
    m_overlayText = formatOverlay(overlayCoords, speed, QDateTime::currentDateTime());
    emit updated();
}
