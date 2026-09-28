import QtQuick 2.0
import QtMultimedia 5.6
import Nemo.Configuration 1.0

Item {
    property var backList: []
    property var frontCam: null

    ConfigurationValue {
        id: jollaBackLabels
        key: "/apps/jolla-camera/backCameraLabels"
        defaultValue: []
    }

    function refresh() {
        var list = QtMultimedia.availableCameras
        var back = []
        var front = null
        for (var i = 0; i < list.length; i++) {
            if (list[i].position === Camera.BackFace)
                back.push(list[i])
            else if (front === null && list[i].position === Camera.FrontFace)
                front = list[i]
        }
        backList = back
        frontCam = front
    }

    function backLabel(index) {
        var labels = jollaBackLabels.value
        if (typeof labels === "string")
            labels = [labels]
        if (labels && labels.length > index && labels[index] !== undefined
                && String(labels[index]).length > 0)
            return String(labels[index])
        return qsTr("Rear %1").arg(index + 1)
    }

    function resolveRearDeviceId(saved) {
        var back = backList
        if (!back || back.length === 0)
            return ""
        for (var i = 0; i < back.length; i++) {
            if (saved !== "" && back[i].deviceId === saved)
                return saved
        }
        return back[0].deviceId
    }

    function nextRearDeviceId(saved) {
        var back = backList
        if (!back || back.length === 0)
            return ""
        if (back.length === 1)
            return back[0].deviceId
        var i = rearIndex(saved)
        return back[(i + 1) % back.length].deviceId
    }

    function rearIndex(saved) {
        var id = resolveRearDeviceId(saved)
        for (var i = 0; i < backList.length; i++) {
            if (backList[i].deviceId === id)
                return i
        }
        return 0
    }
}
