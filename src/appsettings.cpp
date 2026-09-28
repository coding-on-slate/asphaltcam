#include "appsettings.h"

#include <QDebug>
#include <QDir>
#include <QList>
#include <QStandardPaths>
#include <QVariant>

static const char kConfBase[] = "/org/asphaltcam/harbour-asphaltcam/";
static const char kOrg[] = "org.asphaltcam";
static const char kApp[] = "harbour-asphaltcam";

static bool asphaltcam_isUserMediaDir(const QString &path)
{
    return path.contains(QLatin1String("/Videos/"))
            || path.endsWith(QLatin1String("/Videos"))
            || path.contains(QLatin1String("/Movies/"))
            || path.endsWith(QLatin1String("/Movies"));
}

static QString asphaltcam_privateRoot()
{
    QString cache = QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation);
    if (cache.isEmpty() || asphaltcam_isUserMediaDir(cache))
        cache = QDir::home().filePath(QStringLiteral(".cache"));
    const QString root = cache + QLatin1Char('/') + QLatin1String(kOrg) + QLatin1Char('/')
            + QLatin1String(kApp);
    if (asphaltcam_isUserMediaDir(root))
        return QDir::home().filePath(QStringLiteral(".cache/%1/%2").arg(QLatin1String(kOrg),
                                                                      QLatin1String(kApp)));
    return root;
}

static int asphaltcam_clamp_window_seconds(int seconds)
{
    const QList<int> allowed = QList<int>() << 30 << 60 << 180 << 300;
    if (!allowed.contains(seconds))
        return 60;
    return seconds;
}

static QVariant asphaltcam_conf_value(const MDConfItem &item, const QVariant &fallback)
{
    const QVariant value = item.value();
    return value.isValid() ? value : fallback;
}

AppSettings::AppSettings(QObject *parent)
    : QObject(parent)
    , m_preIncidentSeconds(QLatin1String(kConfBase) + QStringLiteral("preIncidentSeconds"))
    , m_postIncidentSeconds(QLatin1String(kConfBase) + QStringLiteral("postIncidentSeconds"))
    , m_qualityIndex(QLatin1String(kConfBase) + QStringLiteral("qualityIndex"))
    , m_cameraMode(QLatin1String(kConfBase) + QStringLiteral("cameraMode"))
    , m_rearDeviceId(QLatin1String(kConfBase) + QStringLiteral("rearDeviceId"))
    , m_audioEnabled(QLatin1String(kConfBase) + QStringLiteral("audioEnabled"))
    , m_recordingNoticeAccepted(QLatin1String(kConfBase) + QStringLiteral("recordingNoticeAccepted"))
    , m_preventDisplayBlanking(QLatin1String(kConfBase) + QStringLiteral("preventDisplayBlanking"))
    , m_speedUnitKmh(QLatin1String(kConfBase) + QStringLiteral("speedUnitKmh"))
    , m_crashThresholdG(QLatin1String(kConfBase) + QStringLiteral("crashThresholdG"))
    , m_thermalProtectionEnabled(QLatin1String(kConfBase) + QStringLiteral("thermalProtectionEnabled"))
    , m_heatWarnC(QLatin1String(kConfBase) + QStringLiteral("heatWarnC"))
    , m_storageProtectionEnabled(QLatin1String(kConfBase) + QStringLiteral("storageProtectionEnabled"))
    , m_storageWarnPercent(QLatin1String(kConfBase) + QStringLiteral("storageWarnPercent"))
    , m_batteryProtectionEnabled(QLatin1String(kConfBase) + QStringLiteral("batteryProtectionEnabled"))
    , m_batteryWarnPercent(QLatin1String(kConfBase) + QStringLiteral("batteryWarnPercent"))
{
    const QString movies = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
    const QString root = asphaltcam_privateRoot();
    m_incidentsPath = movies + QStringLiteral("/AsphaltCam");
    m_loopPath = root + QStringLiteral("/loop");
    m_workPath = root + QStringLiteral("/work");
    qWarning() << "AsphaltCam: loop" << m_loopPath << "incidents" << m_incidentsPath;
    ensureDirectories();

    connect(&m_preIncidentSeconds, SIGNAL(valueChanged()), this, SIGNAL(preIncidentSecondsChanged()));
    connect(&m_postIncidentSeconds, SIGNAL(valueChanged()), this, SIGNAL(postIncidentSecondsChanged()));
    connect(&m_qualityIndex, SIGNAL(valueChanged()), this, SIGNAL(qualityIndexChanged()));
    connect(&m_cameraMode, SIGNAL(valueChanged()), this, SIGNAL(cameraModeChanged()));
    connect(&m_rearDeviceId, SIGNAL(valueChanged()), this, SIGNAL(rearDeviceIdChanged()));
    connect(&m_audioEnabled, SIGNAL(valueChanged()), this, SIGNAL(audioEnabledChanged()));
    connect(&m_recordingNoticeAccepted, SIGNAL(valueChanged()), this, SIGNAL(recordingNoticeAcceptedChanged()));
    connect(&m_preventDisplayBlanking, SIGNAL(valueChanged()), this, SIGNAL(preventDisplayBlankingChanged()));
    connect(&m_speedUnitKmh, SIGNAL(valueChanged()), this, SIGNAL(speedUnitKmhChanged()));
    connect(&m_crashThresholdG, SIGNAL(valueChanged()), this, SIGNAL(crashThresholdGChanged()));
    connect(&m_thermalProtectionEnabled, SIGNAL(valueChanged()), this, SIGNAL(thermalProtectionEnabledChanged()));
    connect(&m_heatWarnC, SIGNAL(valueChanged()), this, SIGNAL(heatWarnCChanged()));
    connect(&m_storageProtectionEnabled, SIGNAL(valueChanged()), this, SIGNAL(storageProtectionEnabledChanged()));
    connect(&m_storageWarnPercent, SIGNAL(valueChanged()), this, SIGNAL(storageWarnPercentChanged()));
    connect(&m_batteryProtectionEnabled, SIGNAL(valueChanged()), this, SIGNAL(batteryProtectionEnabledChanged()));
    connect(&m_batteryWarnPercent, SIGNAL(valueChanged()), this, SIGNAL(batteryWarnPercentChanged()));
}

void AppSettings::ensureDirectories()
{
    QDir().mkpath(m_loopPath);
    QDir().mkpath(m_workPath);
    QDir().mkpath(m_incidentsPath);
}

int AppSettings::preIncidentSeconds() const
{
    return asphaltcam_clamp_window_seconds(
                asphaltcam_conf_value(m_preIncidentSeconds, 60).toInt());
}

int AppSettings::postIncidentSeconds() const
{
    return asphaltcam_clamp_window_seconds(
                asphaltcam_conf_value(m_postIncidentSeconds, 60).toInt());
}

int AppSettings::qualityIndex() const
{
    return qBound(0, asphaltcam_conf_value(m_qualityIndex, 1).toInt(), 2);
}

int AppSettings::cameraMode() const
{
    return qBound(0, asphaltcam_conf_value(m_cameraMode, 0).toInt(), 1);
}

bool AppSettings::audioEnabled() const
{
    return asphaltcam_conf_value(m_audioEnabled, false).toBool();
}

bool AppSettings::speedUnitKmh() const
{
    return asphaltcam_conf_value(m_speedUnitKmh, true).toBool();
}

qreal AppSettings::crashThresholdG() const
{
    return qBound(qreal(1.2),
                  asphaltcam_conf_value(m_crashThresholdG, 2.5).toReal(),
                  qreal(6.0));
}

QString AppSettings::rearDeviceId() const
{
    return asphaltcam_conf_value(m_rearDeviceId, QString()).toString();
}

void AppSettings::setPreIncidentSeconds(int seconds)
{
    m_preIncidentSeconds.set(asphaltcam_clamp_window_seconds(seconds));
}

void AppSettings::setPostIncidentSeconds(int seconds)
{
    m_postIncidentSeconds.set(asphaltcam_clamp_window_seconds(seconds));
}

void AppSettings::setQualityIndex(int index)
{
    m_qualityIndex.set(qBound(0, index, 2));
}

void AppSettings::setCameraMode(int mode)
{
    m_cameraMode.set(qBound(0, mode, 1));
}

void AppSettings::setRearDeviceId(const QString &id)
{
    m_rearDeviceId.set(id);
}

void AppSettings::setAudioEnabled(bool enabled)
{
    m_audioEnabled.set(enabled);
}

bool AppSettings::recordingNoticeAccepted() const
{
    return asphaltcam_conf_value(m_recordingNoticeAccepted, false).toBool();
}

void AppSettings::setRecordingNoticeAccepted(bool accepted)
{
    m_recordingNoticeAccepted.set(accepted);
}

bool AppSettings::preventDisplayBlanking() const
{
    return asphaltcam_conf_value(m_preventDisplayBlanking, true).toBool();
}

void AppSettings::setPreventDisplayBlanking(bool enabled)
{
    m_preventDisplayBlanking.set(enabled);
}

void AppSettings::setSpeedUnitKmh(bool kmh)
{
    m_speedUnitKmh.set(kmh);
}

void AppSettings::setCrashThresholdG(qreal gForce)
{
    m_crashThresholdG.set(qBound(qreal(1.2), gForce, qreal(6.0)));
}

bool AppSettings::thermalProtectionEnabled() const
{
    return asphaltcam_conf_value(m_thermalProtectionEnabled, true).toBool();
}

void AppSettings::setThermalProtectionEnabled(bool enabled)
{
    m_thermalProtectionEnabled.set(enabled);
}

int AppSettings::heatWarnC() const
{
    const int value = asphaltcam_conf_value(m_heatWarnC, 43).toInt();
    if (value == 37 || value == 35)
        return 37;
    if (value == 40)
        return 40;
    return 43;
}

void AppSettings::setHeatWarnC(int celsius)
{
    if (celsius == 37)
        m_heatWarnC.set(37);
    else if (celsius == 43)
        m_heatWarnC.set(43);
    else
        m_heatWarnC.set(40);
}

bool AppSettings::storageProtectionEnabled() const
{
    return asphaltcam_conf_value(m_storageProtectionEnabled, true).toBool();
}

void AppSettings::setStorageProtectionEnabled(bool enabled)
{
    m_storageProtectionEnabled.set(enabled);
}

int AppSettings::storageWarnPercent() const
{
    const int value = asphaltcam_conf_value(m_storageWarnPercent, 10).toInt();
    if (value == 20 || value == 15)
        return value;
    return 10;
}

void AppSettings::setStorageWarnPercent(int percent)
{
    if (percent == 20 || percent == 15)
        m_storageWarnPercent.set(percent);
    else
        m_storageWarnPercent.set(10);
}

bool AppSettings::batteryProtectionEnabled() const
{
    return asphaltcam_conf_value(m_batteryProtectionEnabled, true).toBool();
}

void AppSettings::setBatteryProtectionEnabled(bool enabled)
{
    m_batteryProtectionEnabled.set(enabled);
}

int AppSettings::batteryWarnPercent() const
{
    const int value = asphaltcam_conf_value(m_batteryWarnPercent, 15).toInt();
    if (value == 10 || value == 25 || value == 30)
        return value;
    return 15;
}

void AppSettings::setBatteryWarnPercent(int percent)
{
    if (percent == 10 || percent == 25 || percent == 30)
        m_batteryWarnPercent.set(percent);
    else
        m_batteryWarnPercent.set(15);
}

int AppSettings::preIncidentSegments() const
{
    return qMax(1, preIncidentSeconds() / segmentSeconds());
}

qint64 AppSettings::loopRetentionMs() const
{
    return static_cast<qint64>(preIncidentSeconds()) * 1000;
}

int AppSettings::videoWidth() const
{
    switch (qualityIndex()) {
    case 0: return 854;
    case 2: return 1920;
    default: return 1280;
    }
}

int AppSettings::videoHeight() const
{
    switch (qualityIndex()) {
    case 0: return 480;
    case 2: return 1080;
    default: return 720;
    }
}

int AppSettings::videoBitrateKbps() const
{
    switch (qualityIndex()) {
    case 0: return 1500;
    case 2: return 8000;
    default: return 4000;
    }
}
