import QtQuick 2.0
import Sailfish.Silica 1.0

Dialog {
    id: dialog
    allowedOrientations: Orientation.All

    onAccepted: appSettings.recordingNoticeAccepted = true
    onRejected: Qt.quit()

    Column {
        width: parent.width

        DialogHeader {
            title: qsTr("Recording in public")
        }

        Label {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * x
            wrapMode: Text.Wrap
            font.pixelSize: Theme.fontSizeSmall
            color: Theme.primaryColor
            text: qsTr("You are responsible for following the law where you use AsphaltCam. Video, optional microphone audio, and GPS in saved incidents may record other people. Audio is off until you turn it on in Settings. This app does not guarantee that a clip is valid evidence.")
        }
    }
}
