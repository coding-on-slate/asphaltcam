#include "devicestatus.h"
#include "appsettings.h"

#include <QDir>
#include <QFile>
#include <QStorageInfo>
#include <QtGlobal>

static const qreal kHeatStopC = 45.0;
static const qreal kStorageStopPercent = 5.0;
static const qreal kBatteryStopPercent = 5.0;

static bool asphaltcam_parse_temp(qint64 raw, qreal *celsius)
{
    if (!celsius)
        return false;
    qreal c = 0;
    const qint64 mag = qAbs(raw);
    if (mag > 1000)
        c = raw / 1000.0;
    else if (mag > 100)
        c = raw / 10.0;
    else
        c = raw;
    if (c < 0 || c > 95)
        return false;
    *celsius = c;
    return true;
}

static bool asphaltcam_read_sys_temp(const QString &path, qreal *celsius)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;
    const QByteArray line = file.readLine().trimmed();
    bool ok = false;
    const qint64 raw = line.toLongLong(&ok);
    return ok && asphaltcam_parse_temp(raw, celsius);
}

static QString asphaltcam_format_bytes(qint64 bytes)
{
    const qreal gb = bytes / 1073741824.0;
    if (gb >= 1.0)
        return QString::number(gb, 'f', 1) + QStringLiteral(" GB");
    const qreal mb = bytes / 1048576.0;
    return QString::number(qMax(0.0, mb), 'f', 0) + QStringLiteral(" MB");
}

DeviceStatus::DeviceStatus(AppSettings *settings, QObject *parent)
    : QObject(parent)
    , m_settings(settings)
    , m_temperatureValid(false)
    , m_temperatureC(0)
    , m_heatWarning(false)
    , m_heatCritical(false)
    , m_bytesFree(0)
    , m_bytesTotal(0)
    , m_freePercent(100)
    , m_storageLow(false)
    , m_storageCritical(false)
    , m_batteryValid(false)
    , m_batteryPercent(0)
    , m_batteryWarning(false)
    , m_batteryCritical(false)
{
    m_timer.setInterval(5000);
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(poll()));
    if (m_settings) {
        connect(m_settings, SIGNAL(thermalProtectionEnabledChanged()), this, SLOT(poll()));
        connect(m_settings, SIGNAL(heatWarnCChanged()), this, SLOT(poll()));
        connect(m_settings, SIGNAL(storageProtectionEnabledChanged()), this, SLOT(poll()));
        connect(m_settings, SIGNAL(storageWarnPercentChanged()), this, SLOT(poll()));
        connect(m_settings, SIGNAL(batteryProtectionEnabledChanged()), this, SLOT(poll()));
        connect(m_settings, SIGNAL(batteryWarnPercentChanged()), this, SLOT(poll()));
    }
    poll();
    m_timer.start();
}

void DeviceStatus::poll()
{
    readTemperature();
    readStorage();
    readBattery();
    rebuildFlags();
}

void DeviceStatus::readTemperature()
{
    m_temperatureValid = false;
    m_temperatureC = 0;

    static const char *batteryPaths[] = {
        "/sys/class/power_supply/battery/temp",
        "/sys/class/power_supply/BAT0/temp",
        "/sys/class/power_supply/bms/temp",
        0
    };
    for (int i = 0; batteryPaths[i]; ++i) {
        qreal c = 0;
        if (!asphaltcam_read_sys_temp(QLatin1String(batteryPaths[i]), &c))
            continue;
        m_temperatureC = c;
        m_temperatureValid = true;
        return;
    }

    qreal best = -1;
    const QDir zoneDir(QStringLiteral("/sys/class/thermal"));
    const QStringList zones = zoneDir.entryList(QStringList() << QStringLiteral("thermal_zone*"),
                                                QDir::Dirs | QDir::NoDotAndDotDot);
    foreach (const QString &zone, zones) {
        qreal c = 0;
        if (!asphaltcam_read_sys_temp(zoneDir.absoluteFilePath(zone) + QStringLiteral("/temp"), &c))
            continue;
        if (c > best)
            best = c;
    }
    if (best >= 0) {
        m_temperatureC = best;
        m_temperatureValid = true;
    }
}

void DeviceStatus::readStorage()
{
    m_bytesFree = 0;
    m_bytesTotal = 0;
    m_freePercent = 100;
    if (!m_settings)
        return;
    QStorageInfo info(m_settings->incidentsPath());
    info.refresh();
    if (!info.isValid() || info.bytesTotal() <= 0)
        return;
    m_bytesTotal = info.bytesTotal();
    m_bytesFree = qMax(qint64(0), info.bytesAvailable());
    m_freePercent = 100.0 * qreal(m_bytesFree) / qreal(m_bytesTotal);
}

void DeviceStatus::readBattery()
{
    m_batteryValid = false;
    m_batteryPercent = 0;
    static const char *capacityPaths[] = {
        "/sys/class/power_supply/battery/capacity",
        "/sys/class/power_supply/BAT0/capacity",
        0
    };
    for (int i = 0; capacityPaths[i]; ++i) {
        QFile file(QLatin1String(capacityPaths[i]));
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            continue;
        const QByteArray line = file.readLine().trimmed();
        bool ok = false;
        const int percent = line.toInt(&ok);
        if (!ok)
            continue;
        m_batteryPercent = qBound(0, percent, 100);
        m_batteryValid = true;
        return;
    }
}

void DeviceStatus::rebuildFlags()
{
    const bool heatProtect = m_settings && m_settings->thermalProtectionEnabled();
    const bool storageProtect = m_settings && m_settings->storageProtectionEnabled();
    const bool batteryProtect = m_settings && m_settings->batteryProtectionEnabled();
    const int warnC = m_settings ? m_settings->heatWarnC() : 43;
    const int warnPct = m_settings ? m_settings->storageWarnPercent() : 10;
    const int batteryWarnPct = m_settings ? m_settings->batteryWarnPercent() : 15;

    const bool heatWarning = heatProtect && m_temperatureValid && m_temperatureC >= warnC;
    const bool heatCritical = heatProtect && m_temperatureValid && m_temperatureC >= kHeatStopC;
    const bool haveStorage = m_bytesTotal > 0;
    const bool storageLow = storageProtect && haveStorage && m_freePercent <= warnPct;
    const bool storageCritical = storageProtect && haveStorage && m_freePercent <= kStorageStopPercent;
    const bool batteryWarning = batteryProtect && m_batteryValid && m_batteryPercent <= batteryWarnPct;
    const bool batteryCritical = batteryProtect && m_batteryValid && m_batteryPercent <= kBatteryStopPercent;

    QString temperatureText = QStringLiteral("--");
    if (m_temperatureValid)
        temperatureText = QString::number(qRound(m_temperatureC)) + QStringLiteral(" °C");

    QString storageText = QStringLiteral("--");
    if (haveStorage) {
        storageText = tr("%1 free (%2%)")
                .arg(asphaltcam_format_bytes(m_bytesFree))
                .arg(qRound(m_freePercent));
    }

    QString batteryText = QStringLiteral("--");
    if (m_batteryValid)
        batteryText = QString::number(m_batteryPercent) + QLatin1Char('%');
    const QString batteryTempText = batteryText + QStringLiteral(", ") + temperatureText;

    const bool heatCritChanged = heatCritical != m_heatCritical;
    const bool storageCritChanged = storageCritical != m_storageCritical;
    const bool batteryCritChanged = batteryCritical != m_batteryCritical;

    m_heatWarning = heatWarning;
    m_heatCritical = heatCritical;
    m_storageLow = storageLow;
    m_storageCritical = storageCritical;
    m_temperatureText = temperatureText;
    m_storageText = storageText;
    m_batteryText = batteryText;
    m_batteryTempText = batteryTempText;
    m_batteryWarning = batteryWarning;
    m_batteryCritical = batteryCritical;

    emit updated();
    if (heatCritChanged)
        emit heatCriticalChanged();
    if (storageCritChanged)
        emit storageCriticalChanged();
    if (batteryCritChanged)
        emit batteryCriticalChanged();
}
