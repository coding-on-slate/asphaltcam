#ifndef APPSETTINGS_H
#define APPSETTINGS_H

#include <QObject>
#include <QString>
#include <MDConfItem>

class AppSettings : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int preIncidentSeconds READ preIncidentSeconds WRITE setPreIncidentSeconds NOTIFY preIncidentSecondsChanged)
    Q_PROPERTY(int postIncidentSeconds READ postIncidentSeconds WRITE setPostIncidentSeconds NOTIFY postIncidentSecondsChanged)
    Q_PROPERTY(int qualityIndex READ qualityIndex WRITE setQualityIndex NOTIFY qualityIndexChanged)
    Q_PROPERTY(int cameraMode READ cameraMode WRITE setCameraMode NOTIFY cameraModeChanged)
    Q_PROPERTY(QString rearDeviceId READ rearDeviceId WRITE setRearDeviceId NOTIFY rearDeviceIdChanged)
    Q_PROPERTY(bool audioEnabled READ audioEnabled WRITE setAudioEnabled NOTIFY audioEnabledChanged)
    Q_PROPERTY(bool recordingNoticeAccepted READ recordingNoticeAccepted WRITE setRecordingNoticeAccepted NOTIFY recordingNoticeAcceptedChanged)
    Q_PROPERTY(bool preventDisplayBlanking READ preventDisplayBlanking WRITE setPreventDisplayBlanking NOTIFY preventDisplayBlankingChanged)
    Q_PROPERTY(bool speedUnitKmh READ speedUnitKmh WRITE setSpeedUnitKmh NOTIFY speedUnitKmhChanged)
    Q_PROPERTY(qreal crashThresholdG READ crashThresholdG WRITE setCrashThresholdG NOTIFY crashThresholdGChanged)
    Q_PROPERTY(bool thermalProtectionEnabled READ thermalProtectionEnabled WRITE setThermalProtectionEnabled NOTIFY thermalProtectionEnabledChanged)
    Q_PROPERTY(int heatWarnC READ heatWarnC WRITE setHeatWarnC NOTIFY heatWarnCChanged)
    Q_PROPERTY(bool storageProtectionEnabled READ storageProtectionEnabled WRITE setStorageProtectionEnabled NOTIFY storageProtectionEnabledChanged)
    Q_PROPERTY(int storageWarnPercent READ storageWarnPercent WRITE setStorageWarnPercent NOTIFY storageWarnPercentChanged)
    Q_PROPERTY(bool batteryProtectionEnabled READ batteryProtectionEnabled WRITE setBatteryProtectionEnabled NOTIFY batteryProtectionEnabledChanged)
    Q_PROPERTY(int batteryWarnPercent READ batteryWarnPercent WRITE setBatteryWarnPercent NOTIFY batteryWarnPercentChanged)
    Q_PROPERTY(int videoWidth READ videoWidth NOTIFY qualityIndexChanged)
    Q_PROPERTY(int videoHeight READ videoHeight NOTIFY qualityIndexChanged)
    Q_PROPERTY(int videoBitrateKbps READ videoBitrateKbps NOTIFY qualityIndexChanged)
    Q_PROPERTY(QString storagePath READ storagePath CONSTANT)
    Q_PROPERTY(QString loopPath READ loopPath CONSTANT)
public:
    explicit AppSettings(QObject *parent = nullptr);

    int preIncidentSeconds() const;
    void setPreIncidentSeconds(int seconds);
    int postIncidentSeconds() const;
    void setPostIncidentSeconds(int seconds);
    int qualityIndex() const;
    void setQualityIndex(int index);
    int cameraMode() const;
    void setCameraMode(int mode);
    QString rearDeviceId() const;
    void setRearDeviceId(const QString &id);
    bool audioEnabled() const;
    void setAudioEnabled(bool enabled);
    bool recordingNoticeAccepted() const;
    void setRecordingNoticeAccepted(bool accepted);
    bool preventDisplayBlanking() const;
    void setPreventDisplayBlanking(bool enabled);
    bool speedUnitKmh() const;
    void setSpeedUnitKmh(bool kmh);
    qreal crashThresholdG() const;
    void setCrashThresholdG(qreal gForce);
    bool thermalProtectionEnabled() const;
    void setThermalProtectionEnabled(bool enabled);
    int heatWarnC() const;
    void setHeatWarnC(int celsius);
    bool storageProtectionEnabled() const;
    void setStorageProtectionEnabled(bool enabled);
    int storageWarnPercent() const;
    void setStorageWarnPercent(int percent);
    bool batteryProtectionEnabled() const;
    void setBatteryProtectionEnabled(bool enabled);
    int batteryWarnPercent() const;
    void setBatteryWarnPercent(int percent);

    QString storagePath() const { return m_incidentsPath; }
    QString loopPath() const { return m_loopPath; }
    QString workPath() const { return m_workPath; }
    QString incidentsPath() const { return m_incidentsPath; }

    int videoWidth() const;
    int videoHeight() const;
    int videoBitrateKbps() const;
    int segmentSeconds() const { return 30; }
    int preIncidentSegments() const;
    qint64 loopRetentionMs() const;

signals:
    void preIncidentSecondsChanged();
    void postIncidentSecondsChanged();
    void qualityIndexChanged();
    void cameraModeChanged();
    void rearDeviceIdChanged();
    void audioEnabledChanged();
    void recordingNoticeAcceptedChanged();
    void preventDisplayBlankingChanged();
    void speedUnitKmhChanged();
    void crashThresholdGChanged();
    void thermalProtectionEnabledChanged();
    void heatWarnCChanged();
    void storageProtectionEnabledChanged();
    void storageWarnPercentChanged();
    void batteryProtectionEnabledChanged();
    void batteryWarnPercentChanged();

private:
    void ensureDirectories();

    MDConfItem m_preIncidentSeconds;
    MDConfItem m_postIncidentSeconds;
    MDConfItem m_qualityIndex;
    MDConfItem m_cameraMode;
    MDConfItem m_rearDeviceId;
    MDConfItem m_audioEnabled;
    MDConfItem m_recordingNoticeAccepted;
    MDConfItem m_preventDisplayBlanking;
    MDConfItem m_speedUnitKmh;
    MDConfItem m_crashThresholdG;
    MDConfItem m_thermalProtectionEnabled;
    MDConfItem m_heatWarnC;
    MDConfItem m_storageProtectionEnabled;
    MDConfItem m_storageWarnPercent;
    MDConfItem m_batteryProtectionEnabled;
    MDConfItem m_batteryWarnPercent;
    QString m_incidentsPath;
    QString m_loopPath;
    QString m_workPath;
};

#endif
