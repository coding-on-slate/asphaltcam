import QtQuick 2.0
import Sailfish.Silica 1.0
import QtMultimedia 5.0

Page {
    id: page
    allowedOrientations: Orientation.All
    property string clipPath
    property var clipPaths: []
    property int partIndex: 0
    property string cueLine: subtitleLog ? subtitleLog.textAt(srtPath(), player.position) : ""
    property var cue: parseCue(cueLine)

    function parseCue(line) {
        var empty = { time: "--:--", coords: "--", speed: "--" }
        if (!line)
            return empty
        var parts = line.split(" | ")
        var time = ""
        var coords = ""
        var speed = ""
        if (parts.length >= 3 && parts[0].indexOf(":") >= 0) {
            time = parts[0]
            coords = parts[1]
            speed = parts[2]
        } else if (parts.length >= 4) {
            coords = parts[0]
            speed = parts[1]
            time = parts[3]
            var gpsMatch = parts[2].match(/±\s*(\d+)/)
            if (gpsMatch)
                coords += " (±" + gpsMatch[1] + " m)"
        } else {
            empty.time = line
            return empty
        }
        var date = time.match(/(\d{4}-\d{2}-\d{2})/)
        var clock = time.match(/(\d{2}:\d{2}:\d{2})/)
        var stamp = time
        if (date && clock)
            stamp = date[1] + " " + clock[1]
        else if (clock)
            stamp = clock[1]
        return {
            time: stamp,
            coords: coords,
            speed: speed
        }
    }

    function fileUrl(path) {
        if (!path)
            return ""
        return path.indexOf("file:") === 0 ? path : ("file://" + path)
    }

    function playlist() {
        if (clipPaths && clipPaths.length)
            return clipPaths
        return clipPath ? [clipPath] : []
    }

    function srtPath() {
        var parts = playlist()
        if (!parts.length || partIndex < 0 || partIndex >= parts.length)
            return ""
        var media = parts[partIndex]
        var dot = media.lastIndexOf(".")
        return (dot > 0 ? media.substring(0, dot) : media) + ".srt"
    }

    MediaPlayer {
        id: player
        source: playlist().length ? fileUrl(playlist()[partIndex]) : ""
        autoPlay: true
        onError: {
            errorLabel.text = errorString.length ? errorString : qsTr("This clip is not playable yet.")
            stop()
        }
        onStopped: {
            if (error !== MediaPlayer.NoError || status !== MediaPlayer.EndOfMedia)
                return
            if (partIndex + 1 < playlist().length) {
                partIndex += 1
                source = fileUrl(playlist()[partIndex])
                play()
            } else {
                pageStack.pop()
            }
        }
    }

    VideoOutput {
        anchors.fill: parent
        source: player
        fillMode: VideoOutput.PreserveAspectFit
    }

    Column {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: Theme.horizontalPageMargin
        anchors.rightMargin: Theme.horizontalPageMargin
        anchors.bottomMargin: Theme.paddingLarge
        spacing: Theme.paddingSmall

        Label {
            width: parent.width
            text: page.cue.time
            font.pixelSize: Theme.fontSizeHuge
            color: Theme.highlightColor
            wrapMode: Text.Wrap
        }
        Label {
            width: parent.width
            text: page.cue.coords
            font.pixelSize: Theme.fontSizeLarge
            color: Theme.primaryColor
            truncationMode: TruncationMode.Fade
        }
        Label {
            width: parent.width
            text: page.cue.speed
            font.pixelSize: Theme.fontSizeLarge
            color: Theme.primaryColor
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            if (player.playbackState === MediaPlayer.PlayingState)
                player.pause()
            else
                player.play()
        }
    }

    Label {
        id: errorLabel
        anchors.centerIn: parent
        width: parent.width - 2 * Theme.horizontalPageMargin
        wrapMode: Text.Wrap
        horizontalAlignment: Text.AlignHCenter
        color: Theme.highlightColor
        visible: text.length > 0
    }

    Label {
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: Theme.paddingLarge * 2
        font.pixelSize: Theme.fontSizeExtraSmall
        color: Theme.secondaryColor
        text: qsTr("Tap to pause")
    }
}
