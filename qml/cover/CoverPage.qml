import QtQuick 2.0
import Sailfish.Silica 1.0

CoverBackground {
    Column {
        anchors.centerIn: parent
        width: parent.width - Theme.paddingLarge
        spacing: Theme.paddingSmall

        Label {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            text: qsTr("AsphaltCam")
            color: Theme.highlightColor
            font.pixelSize: Theme.fontSizeSmall
        }
        Label {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            font.pixelSize: Theme.fontSizeExtraSmall
            color: Theme.primaryColor
            text: recorder.recording
                  ? (clipStorage.incidentBusy
                     ? clipStorage.incidentStatus
                     : (qsTr("Recording") + " " + recorder.elapsedSeconds + "s"))
                  : qsTr("Ready")
        }
    }

    CoverActionList {
        enabled: appSettings.recordingNoticeAccepted
        CoverAction {
            iconSource: recorder.recording
                        ? "image://theme/icon-cover-cancel"
                        : "image://theme/icon-cover-play"
            onTriggered: {
                if (recorder.recording)
                    recorder.stopRecording()
                else if (deviceStatus.heatCritical || deviceStatus.batteryCritical
                         || deviceStatus.storageCritical)
                    return
                else
                    recorder.startRecording()
            }
        }
        CoverAction {
            iconSource: "image://theme/icon-cover-new"
            onTriggered: {
                if (recorder.recording && !clipStorage.incidentBusy)
                    recorder.saveClip()
            }
        }
    }
}
