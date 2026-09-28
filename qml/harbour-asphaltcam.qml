import QtQuick 2.0
import Sailfish.Silica 1.0

ApplicationWindow {
    id: window

    allowedOrientations: Orientation.All
    _defaultPageOrientations: Orientation.All
    cover: Qt.resolvedUrl("cover/CoverPage.qml")

    function playNgf(event) {
        ngfClient.play(event)
    }

    initialPage: Qt.resolvedUrl("pages/RecordPage.qml")

    NgfClient { id: ngfClient }

    Connections {
        target: clipStorage
        onIncidentCaptureStarted: ngfClient.play("video_record_start")
        onIncidentReady: ngfClient.play("video_record_stop")
    }
}
