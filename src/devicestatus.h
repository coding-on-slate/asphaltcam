#ifndef DEVICESTATUS_H
#define DEVICESTATUS_H

#include <QObject>
#include <QString>
#include <QTimer>

class AppSettings;

class DeviceStatus : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool temperatureValid READ temperatureValid NOTIFY updated)
    Q_PROPERTY(qreal temperatureC READ temperatureC NOTIFY updated)
    Q_PROPERTY(QString temperatureText READ temperatureText NOTIFY updated)
    Q_PROPERTY(bool heatWarning READ heatWarning NOTIFY updated)
    Q_PROPERTY(bool heatCritical READ heatCritical NOTIFY heatCriticalChanged)
    Q_PROPERTY(qint64 bytesFree READ bytesFree NOTIFY updated)
    Q_PROPERTY(qint64 bytesTotal READ bytesTotal NOTIFY updated)
    Q_PROPERTY(qreal freePercent READ freePercent NOTIFY updated)
    Q_PROPERTY(QString storageText READ storageText NOTIFY updated)
    Q_PROPERTY(bool storageLow READ storageLow NOTIFY updated)
    Q_PROPERTY(bool storageCritical READ storageCritical NOTIFY storageCriticalChanged)
    Q_PROPERTY(bool batteryValid READ batteryValid NOTIFY updated)
    Q_PROPERTY(int batteryPercent READ batteryPercent NOTIFY updated)
    Q_PROPERTY(QString batteryText READ batteryText NOTIFY updated)
    Q_PROPERTY(QString batteryTempText READ batteryTempText NOTIFY updated)
    Q_PROPERTY(bool batteryWarning READ batteryWarning NOTIFY updated)
    Q_PROPERTY(bool batteryCritical READ batteryCritical NOTIFY batteryCriticalChanged)
public:
    explicit DeviceStatus(AppSettings *settings, QObject *parent = nullptr);

    bool temperatureValid() const { return m_temperatureValid; }
    qreal temperatureC() const { return m_temperatureC; }
    QString temperatureText() const { return m_temperatureText; }
    bool heatWarning() const { return m_heatWarning; }
    bool heatCritical() const { return m_heatCritical; }
    qint64 bytesFree() const { return m_bytesFree; }
    qint64 bytesTotal() const { return m_bytesTotal; }
    qreal freePercent() const { return m_freePercent; }
    QString storageText() const { return m_storageText; }
    bool storageLow() const { return m_storageLow; }
    bool storageCritical() const { return m_storageCritical; }
    bool batteryValid() const { return m_batteryValid; }
    int batteryPercent() const { return m_batteryPercent; }
    QString batteryText() const { return m_batteryText; }
    QString batteryTempText() const { return m_batteryTempText; }
    bool batteryWarning() const { return m_batteryWarning; }
    bool batteryCritical() const { return m_batteryCritical; }

signals:
    void updated();
    void heatCriticalChanged();
    void storageCriticalChanged();
    void batteryCriticalChanged();

private slots:
    void poll();

private:
    void readTemperature();
    void readStorage();
    void readBattery();
    void rebuildFlags();

    AppSettings *m_settings;
    QTimer m_timer;
    bool m_temperatureValid;
    qreal m_temperatureC;
    QString m_temperatureText;
    bool m_heatWarning;
    bool m_heatCritical;
    qint64 m_bytesFree;
    qint64 m_bytesTotal;
    qreal m_freePercent;
    QString m_storageText;
    bool m_storageLow;
    bool m_storageCritical;
    bool m_batteryValid;
    int m_batteryPercent;
    QString m_batteryText;
    QString m_batteryTempText;
    bool m_batteryWarning;
    bool m_batteryCritical;
};

#endif
