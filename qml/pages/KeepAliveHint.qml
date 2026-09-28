import QtQuick 2.0
import Nemo.KeepAlive 1.2

Item {
    DisplayBlanking {
        preventBlanking: appSettings.preventDisplayBlanking
                         && recorder && recorder.recording
                         && Qt.application.state === Qt.ApplicationActive
    }
    KeepAlive {
        enabled: recorder && recorder.recording
    }
}
