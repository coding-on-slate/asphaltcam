import QtQuick 2.0
import Sailfish.Silica 1.0
import QtMultimedia 5.6
import QtSensors 5.2
import ".."

Page {
    id: page
    property bool previewWanted: false
    property bool pendingRecord: false
    property bool recorderWasRecording: false
    property real currentG: 1.0
    property string hudNotice: ""
    readonly property real hudPrefixIconSize: Theme.iconSizeMedium
    readonly property int hudIconNudge: Theme.paddingMedium
    readonly property bool isLandscape: orientation === Orientation.Landscape
                                        || orientation === Orientation.LandscapeInverted
    readonly property int pageRotation: {
        switch (orientation) {
        case Orientation.Landscape:
            return 270
        case Orientation.PortraitInverted:
            return 180
        case Orientation.LandscapeInverted:
            return 90
        default:
            return 0
        }
    }
    readonly property int viewfinderOrientation: (720 + camera.orientation + page.pageRotation) % 360
    readonly property int fileOrientation: camera.position === Camera.FrontFace
            ? (720 + camera.orientation - page.pageRotation + 180) % 360
            : (720 + camera.orientation + page.pageRotation + 180) % 360

    allowedOrientations: Orientation.All
    showNavigationIndicator: !recorder.recording

    Loader { source: Qt.resolvedUrl("KeepAliveHint.qml") }
    CameraDevices { id: cameraDevices }

    function playNgf(event) {
        var w = null
        if (typeof __silica_applicationwindow_instance !== "undefined")
            w = __silica_applicationwindow_instance
        else
            w = page._applicationWindow
        if (w && w.playNgf)
            w.playNgf(event)
    }

    Accelerometer {
        dataRate: 50
        active: true
        alwaysOn: true
        onReadingChanged: {
            if (!reading)
                return
            var g = Math.sqrt(reading.x * reading.x + reading.y * reading.y
                              + reading.z * reading.z) / 9.80665
            currentG = g
            if (!recorder.recording)
                return
            var impact = Math.abs(g - 1.0)
            if (impact < appSettings.crashThresholdG)
                return
            if (clipStorage.incidentBusy)
                return
            recorder.saveClip()
        }
    }

    Camera {
        id: camera
        captureMode: Camera.CaptureVideo
        cameraState: previewWanted ? Camera.ActiveState : Camera.UnloadedState
        flash.mode: Camera.FlashOff
        focus.focusMode: Camera.FocusContinuous

        videoRecorder.onRecorderStateChanged: {
            var state = camera.videoRecorder.recorderState
            if (state === CameraRecorder.RecordingState) {
                recorderWasRecording = true
            } else if (state === CameraRecorder.StoppedState && recorderWasRecording) {
                recorderWasRecording = false
                recorder.nativeRecorderStopped()
            }
        }
        videoRecorder.onRecorderStatusChanged: tryStartRecord()
        onCameraStatusChanged: {
            if (cameraStatus === Camera.ActiveStatus)
                tryStartRecord()
        }
    }

    Connections {
        target: recorder
        onRecordingChanged: {
            if (recorder.recording) {
                page.hudNotice = qsTr("Loop recording started")
                page.forwardNavigation = false
                if (pageStack.currentPage !== page)
                    pageStack.pop(page)
            } else {
                page.hudNotice = ""
                page.forwardNavigation = pageStack.nextPage(page) !== null
            }
        }
        onNativePreviewStart: page.previewWanted = true
        onNativePreviewStop: page.previewWanted = false
        onNativeRecordStart: {
            applyRecorderSettings()
            pendingRecord = true
            tryStartRecord()
            recordDelay.restart()
        }
        onNativeRecordStop: camera.videoRecorder.stop()
    }

    Timer {
        id: recordDelay
        interval: 250
        onTriggered: tryStartRecord()
    }

    function applyRecorderSettings() {
        camera.videoRecorder.audioSampleRate = 48000
        camera.videoRecorder.audioChannels = 1
        camera.videoRecorder.audioCodec = "audio/mpeg, mpegversion=(int)4"
        camera.videoRecorder.frameRate = 30
        camera.videoRecorder.videoCodec = "video/x-h264"
        camera.videoRecorder.mediaContainer = "video/quicktime, variant=(string)iso"
        camera.videoRecorder.videoEncodingMode = CameraRecorder.AverageBitRateEncoding
        camera.videoRecorder.resolution = Qt.size(appSettings.videoWidth, appSettings.videoHeight)
        camera.videoRecorder.videoBitRate = appSettings.videoBitrateKbps * 1000
        camera.videoRecorder.muted = !appSettings.audioEnabled
        camera.metaData.orientation = 0
        clipStorage.setFileOrientation(page.fileOrientation)
    }

    function tryStartRecord() {
        if (!pendingRecord)
            return
        if (camera.captureMode !== Camera.CaptureVideo)
            camera.captureMode = Camera.CaptureVideo
        if (camera.cameraStatus !== Camera.ActiveStatus)
            return
        if (camera.videoRecorder.recorderStatus < CameraRecorder.LoadedStatus)
            return
        pendingRecord = false
        recordDelay.stop()
        camera.metaData.orientation = 0
        clipStorage.setFileOrientation(page.fileOrientation)
        camera.videoRecorder.outputLocation = recorder.outputUrl
        camera.videoRecorder.record()
    }

    function applyCameraDevice() {
        if (!previewWanted || cameraDevices.backList.length === 0)
            cameraDevices.refresh()

        var wantFront = appSettings.cameraMode === 1
        var id = ""
        if (wantFront) {
            if (cameraDevices.frontCam)
                id = cameraDevices.frontCam.deviceId
        } else {
            id = cameraDevices.resolveRearDeviceId(appSettings.rearDeviceId)
            if (id !== "" && id !== appSettings.rearDeviceId)
                appSettings.rearDeviceId = id
        }
        if (id === "" && QtMultimedia.availableCameras.length > 0)
            id = QtMultimedia.availableCameras[0].deviceId
        if (id !== "" && camera.deviceId !== id)
            camera.deviceId = id
    }

    function formatElapsed(secs) {
        var m = Math.floor(secs / 60)
        var s = secs % 60
        return (m < 10 ? "0" : "") + m + ":" + (s < 10 ? "0" : "") + s
    }

    function enforceHeatStop() {
        if (!deviceStatus.heatCritical)
            return
        var hadCamera = recorder.recording || recorder.previewing
        if (recorder.recording)
            recorder.stopRecording()
        recorder.stopPreview()
        previewStartDelay.stop()
        page.hudNotice = qsTr("Phone too hot — camera stopped")
        if (hadCamera)
            playNgf("feedback_press")
    }

    function enforceStorageStop() {
        if (!deviceStatus.storageCritical || !recorder.recording)
            return
        recorder.stopRecording()
        page.hudNotice = qsTr("Storage full — recording stopped")
        playNgf("feedback_press")
    }

    function enforceBatteryStop() {
        if (!deviceStatus.batteryCritical)
            return
        var hadCamera = recorder.recording || recorder.previewing
        if (recorder.recording)
            recorder.stopRecording()
        recorder.stopPreview()
        previewStartDelay.stop()
        page.hudNotice = qsTr("Battery empty — camera stopped")
        if (hadCamera)
            playNgf("feedback_press")
    }

    function tryResumePreview() {
        if (page.status !== PageStatus.Active || recorder.recording)
            return
        if (deviceStatus.heatCritical || deviceStatus.batteryCritical)
            return
        applyCameraDevice()
        applyRecorderSettings()
        previewStartDelay.start()
    }

    function tryStartRecording() {
        if (deviceStatus.heatCritical) {
            enforceHeatStop()
            return
        }
        if (deviceStatus.batteryCritical) {
            enforceBatteryStop()
            return
        }
        if (deviceStatus.storageCritical) {
            page.hudNotice = qsTr("Storage full — recording stopped")
            playNgf("feedback_press")
            return
        }
        recorder.startRecording()
    }

    function tryStartPreview() {
        if (deviceStatus.heatCritical) {
            enforceHeatStop()
            return
        }
        if (deviceStatus.batteryCritical) {
            enforceBatteryStop()
            return
        }
        recorder.startPreview()
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: height
        clip: false

        PullDownMenu {
            visible: !recorder.recording
            enabled: !recorder.recording
            MenuItem {
                text: qsTr("Settings")
                onClicked: pageStack.animatorPush(Qt.resolvedUrl("SettingsPage.qml"))
            }
        }

        Item {
            id: layout
            anchors.fill: parent
            readonly property int recColW: Theme.itemSizeLarge + Theme.horizontalPageMargin
            readonly property int accColW: (!recorder.recording && page.isLandscape) ? Theme.itemSizeLarge : 0
            readonly property int hudColW: page.isLandscape ? Math.round(width * 0.32) : width

            Item {
                id: topRow
                x: 0
                y: 0
                width: layout.hudColW
                height: page.isLandscape ? layout.height
                                        : (hudText.height + hudText.anchors.topMargin)

                Column {
                    id: hudText
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.leftMargin: Theme.horizontalPageMargin
                    anchors.rightMargin: Theme.horizontalPageMargin
                    anchors.topMargin: Theme.paddingLarge * 2
                    spacing: Theme.paddingSmall

                    Label {
                        width: parent.width
                        text: formatElapsed(recorder.elapsedSeconds)
                        font.pixelSize: Theme.fontSizeHuge
                        color: Theme.highlightColor
                    }
                    Item {
                        width: parent.width
                        height: Math.max(coordLabel.height, gpsBars.height)

                        Row {
                            id: gpsBars
                            anchors.left: parent.left
                            anchors.verticalCenter: parent.verticalCenter
                            height: Theme.fontSizeLarge * 0.85
                            spacing: Math.round(Theme.paddingSmall / 2)

                            Repeater {
                                model: 3
                                Item {
                                    width: Math.max(Theme.paddingSmall, gpsBars.height * 0.2)
                                    height: gpsBars.height
                                    Rectangle {
                                        width: parent.width
                                        height: parent.height * (0.4 + index * 0.3)
                                        anchors.bottom: parent.bottom
                                        radius: 1
                                        color: Theme.primaryColor
                                        opacity: telemetry.gpsBars > index ? 1.0 : 0.2
                                    }
                                }
                            }
                        }

                        Label {
                            id: coordLabel
                            anchors.left: gpsBars.right
                            anchors.leftMargin: Theme.paddingMedium
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            text: telemetry.coordinatesText
                            font.pixelSize: Theme.fontSizeLarge
                            color: Theme.primaryColor
                            truncationMode: TruncationMode.Fade
                        }
                    }
                    Item {
                        width: parent.width
                        height: Math.max(speedLabel.height, hudPrefixIconSize)

                        Icon {
                            id: speedIcon
                            anchors.left: parent.left
                            anchors.leftMargin: -hudIconNudge
                            anchors.verticalCenter: parent.verticalCenter
                            width: hudPrefixIconSize
                            height: hudPrefixIconSize
                            source: "image://theme/icon-m-forward"
                        }
                        Label {
                            id: speedLabel
                            anchors.left: speedIcon.right
                            anchors.leftMargin: Theme.paddingMedium
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            text: telemetry.speedText
                            font.pixelSize: Theme.fontSizeLarge
                            color: Theme.primaryColor
                            truncationMode: TruncationMode.Fade
                        }
                    }
                    Label {
                        width: parent.width
                        text: qsTr("G %1").arg(currentG.toFixed(1))
                        font.pixelSize: Theme.fontSizeLarge
                        color: Theme.primaryColor
                    }
                    Item {
                        width: parent.width
                        height: Math.max(batteryTempLabel.height, hudPrefixIconSize)

                        Icon {
                            id: batteryIcon
                            anchors.left: parent.left
                            anchors.leftMargin: -hudIconNudge
                            anchors.verticalCenter: parent.verticalCenter
                            width: hudPrefixIconSize
                            height: hudPrefixIconSize
                            source: "image://theme/icon-m-battery"
                            color: (deviceStatus.heatWarning || deviceStatus.heatCritical
                                    || deviceStatus.batteryWarning || deviceStatus.batteryCritical)
                                   ? Theme.highlightColor : Theme.primaryColor
                        }
                        Label {
                            id: batteryTempLabel
                            anchors.left: batteryIcon.right
                            anchors.leftMargin: Theme.paddingMedium
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            text: deviceStatus.batteryTempText
                            font.pixelSize: Theme.fontSizeLarge
                            color: (deviceStatus.heatWarning || deviceStatus.heatCritical
                                    || deviceStatus.batteryWarning || deviceStatus.batteryCritical)
                                   ? Theme.highlightColor : Theme.primaryColor
                            truncationMode: TruncationMode.Fade
                        }
                    }
                    Item {
                        width: parent.width
                        height: Math.max(storageLabel.height, hudPrefixIconSize)

                        Icon {
                            id: storageIcon
                            anchors.left: parent.left
                            anchors.leftMargin: -hudIconNudge
                            anchors.verticalCenter: parent.verticalCenter
                            width: hudPrefixIconSize
                            height: hudPrefixIconSize
                            source: "image://theme/icon-m-storage"
                            color: (deviceStatus.storageLow || deviceStatus.storageCritical)
                                   ? Theme.highlightColor : Theme.primaryColor
                        }
                        Label {
                            id: storageLabel
                            anchors.left: storageIcon.right
                            anchors.leftMargin: Theme.paddingMedium
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            text: deviceStatus.storageText
                            font.pixelSize: Theme.fontSizeLarge
                            color: (deviceStatus.storageLow || deviceStatus.storageCritical)
                                   ? Theme.highlightColor : Theme.primaryColor
                            truncationMode: TruncationMode.Fade
                        }
                    }
                    Label {
                        width: parent.width
                        wrapMode: Text.Wrap
                        font.pixelSize: Theme.fontSizeLarge
                        color: Theme.highlightColor
                        text: recorder.errorString !== ""
                              ? recorder.errorString
                              : (page.hudNotice !== ""
                                 ? page.hudNotice
                                 : (deviceStatus.heatWarning
                                    ? qsTr("Phone getting hot")
                                    : (deviceStatus.batteryWarning
                                       ? qsTr("Battery low (%1%)").arg(deviceStatus.batteryPercent)
                                       : (deviceStatus.storageLow
                                          ? qsTr("Storage low (%1% free)").arg(Math.round(deviceStatus.freePercent))
                                          : clipStorage.incidentStatus))))
                    }
                }
            }

            Item {
                id: incidentArea
                x: page.isLandscape ? topRow.width : 0
                y: page.isLandscape ? 0 : (topRow.height + Theme.paddingLarge)
                width: page.isLandscape ? Math.max(0, accessoryRow.x - x) : layout.width
                height: page.isLandscape ? layout.height
                                        : Math.max(0, accessoryRow.y - y - Theme.paddingLarge)

                MouseArea {
                    id: incidentButton
                    width: parent.width * 0.6
                    height: parent.height * 0.6
                    anchors.centerIn: parent
                    visible: recorder.recording
                    enabled: recorder.recording && !clipStorage.incidentBusy
                    opacity: clipStorage.incidentBusy ? 1 : 0.5
                    onClicked: recorder.saveClip()

                    Canvas {
                        id: warningIcon
                        width: Theme.iconSizeLarge * 2.304
                        height: width
                        anchors.centerIn: parent
                        opacity: 1
                        renderTarget: Canvas.Image
                        onPaint: {
                            var ctx = getContext("2d")
                            ctx.reset()
                            var w = width
                            var h = height
                            var pad = w * 0.08
                            ctx.beginPath()
                            ctx.moveTo(w / 2, pad)
                            ctx.lineTo(w - pad, h - pad)
                            ctx.lineTo(pad, h - pad)
                            ctx.closePath()
                            ctx.lineWidth = Math.max(2, w * 0.07)
                            ctx.strokeStyle = "#e53935"
                            ctx.fillStyle = "transparent"
                            ctx.stroke()
                            var barW = w * 0.09
                            var barTop = h * 0.38
                            var barH = h * 0.28
                            ctx.fillStyle = "#e53935"
                            ctx.fillRect((w - barW) / 2, barTop, barW, barH)
                            var dot = w * 0.09
                            ctx.beginPath()
                            ctx.arc(w / 2, h * 0.78, dot / 2, 0, Math.PI * 2)
                            ctx.fill()
                        }
                        onWidthChanged: requestPaint()
                        onHeightChanged: requestPaint()
                        onVisibleChanged: if (visible) requestPaint()
                        Component.onCompleted: requestPaint()
                    }

                    Column {
                        anchors.top: warningIcon.bottom
                        anchors.topMargin: Theme.paddingMedium
                        anchors.left: parent.left
                        anchors.right: parent.right
                        spacing: Theme.paddingMedium

                        Label {
                            width: parent.width - 2 * Theme.horizontalPageMargin
                            anchors.horizontalCenter: parent.horizontalCenter
                            visible: !clipStorage.incidentBusy
                            text: qsTr("Press to save incident")
                            horizontalAlignment: Text.AlignHCenter
                            wrapMode: Text.Wrap
                            font.pixelSize: Theme.fontSizeMedium
                            color: Theme.primaryColor
                        }

                        ProgressBar {
                            id: incidentProgressBar
                            width: parent.width * 0.55
                            anchors.horizontalCenter: parent.horizontalCenter
                            visible: clipStorage.incidentBusy
                            minimumValue: 0
                            maximumValue: 1
                            value: 0
                            leftMargin: 0
                            rightMargin: 0
                        }
                    }

                    NumberAnimation {
                        id: incidentProgressAnim
                        target: incidentProgressBar
                        property: "value"
                        from: 0
                        to: 1
                        easing.type: Easing.Linear
                    }

                    Connections {
                        target: clipStorage
                        onIncidentStatusChanged: {
                            if (clipStorage.incidentStatus !== "")
                                page.hudNotice = ""
                        }
                        onIncidentCapturingChanged: {
                            if (clipStorage.incidentCapturing) {
                                incidentProgressBar.value = 0
                                incidentProgressAnim.duration = Math.max(1, appSettings.postIncidentSeconds) * 1000
                                incidentProgressAnim.restart()
                            } else {
                                incidentProgressAnim.stop()
                                if (clipStorage.incidentBusy)
                                    incidentProgressBar.value = 1
                                else
                                    incidentProgressBar.value = 0
                            }
                        }
                        onIncidentBusyChanged: {
                            if (!clipStorage.incidentBusy) {
                                incidentProgressAnim.stop()
                                incidentProgressBar.value = 0
                            }
                        }
                    }

                    SequentialAnimation {
                        running: clipStorage.incidentBusy
                        loops: Animation.Infinite
                        NumberAnimation {
                            target: warningIcon
                            property: "opacity"
                            from: 0.2
                            to: 1
                            duration: 750
                            easing.type: Easing.InOutQuad
                        }
                        NumberAnimation {
                            target: warningIcon
                            property: "opacity"
                            from: 1
                            to: 0.2
                            duration: 750
                            easing.type: Easing.InOutQuad
                        }
                        onRunningChanged: {
                            if (!running)
                                warningIcon.opacity = 1
                        }
                    }
                }

                Item {
                    id: previewTap
                    anchors.fill: parent
                    visible: !recorder.recording

                    Item {
                        id: previewBand
                        anchors.centerIn: parent
                        property int sensorW: camera.viewfinder.resolution.width > 0
                                              ? camera.viewfinder.resolution.width
                                              : (camera.videoRecorder.resolution.width > 0
                                                 ? camera.videoRecorder.resolution.width
                                                 : appSettings.videoWidth)
                        property int sensorH: camera.viewfinder.resolution.height > 0
                                              ? camera.viewfinder.resolution.height
                                              : (camera.videoRecorder.resolution.height > 0
                                                 ? camera.videoRecorder.resolution.height
                                                 : appSettings.videoHeight)
                        property bool swapAxes: (page.viewfinderOrientation % 180) !== 0
                        property real frameW: swapAxes ? sensorH : sensorW
                        property real frameH: swapAxes ? sensorW : sensorH
                        property real fit: {
                            var maxW = previewTap.width
                            var maxH = previewTap.height
                            if (frameW <= 0 || frameH <= 0)
                                return 1
                            return Math.min(maxW / frameW, maxH / frameH) * (page.isLandscape ? 0.95 : 0.98)
                        }
                        width: frameW * fit
                        height: frameH * fit
                        clip: false

                        VideoOutput {
                            anchors.centerIn: parent
                            width: (page.pageRotation % 180) ? parent.height : parent.width
                            height: (page.pageRotation % 180) ? parent.width : parent.height
                            rotation: page.pageRotation
                            source: camera
                            fillMode: VideoOutput.PreserveAspectFit
                        }

                        Rectangle {
                            anchors.fill: parent
                            color: "transparent"
                            border.width: 1
                            border.color: Theme.highlightColor
                        }
                    }
                }
            }

            Item {
                id: accessoryRow
                z: 2
                visible: !recorder.recording
                x: page.isLandscape ? controlPanel.x - width : 0
                y: page.isLandscape ? 0 : (controlPanel.y - height - Theme.paddingLarge)
                width: page.isLandscape ? layout.accColW : layout.width
                height: page.isLandscape ? layout.height : Theme.itemSizeMedium

                Flow {
                    flow: page.isLandscape ? Flow.TopToBottom : Flow.LeftToRight
                    spacing: Theme.paddingMedium
                    anchors.centerIn: parent

                    IconButton {
                        width: Theme.iconSizeMedium
                        height: Theme.iconSizeMedium
                        icon.source: "image://theme/icon-m-flip"
                        enabled: QtMultimedia.availableCameras.length > 1
                        onClicked: {
                            playNgf("feedback_press")
                            if (appSettings.cameraMode === 1)
                                appSettings.cameraMode = 0
                            else
                                appSettings.cameraMode = 1
                            applyCameraDevice()
                        }
                    }

                    MouseArea {
                        visible: appSettings.cameraMode === 0
                                 && cameraDevices.backList.length > 1
                        width: visible ? Theme.itemSizeExtraSmall : 0
                        height: Theme.itemSizeExtraSmall
                        onClicked: {
                            playNgf("feedback_press")
                            appSettings.rearDeviceId = cameraDevices.nextRearDeviceId(
                                        appSettings.rearDeviceId)
                        }

                        Rectangle {
                            anchors.fill: parent
                            radius: width / 2
                            color: "transparent"
                            border.width: 2
                            border.color: Theme.highlightColor
                        }
                        Label {
                            anchors.centerIn: parent
                            text: cameraDevices.backLabel(
                                      cameraDevices.rearIndex(appSettings.rearDeviceId))
                            color: Theme.highlightColor
                            font.pixelSize: Theme.fontSizeExtraSmall
                        }
                    }

                    IconButton {
                        width: Theme.iconSizeMedium
                        height: Theme.iconSizeMedium
                        icon.source: appSettings.audioEnabled
                                     ? "image://theme/icon-m-mic"
                                     : "image://theme/icon-m-mic-mute"
                        onClicked: {
                            playNgf("feedback_press")
                            appSettings.audioEnabled = !appSettings.audioEnabled
                        }
                    }
                }
            }

            Item {
                id: controlPanel
                x: page.isLandscape ? layout.width - width : 0
                y: page.isLandscape ? 0 : layout.height - height
                width: page.isLandscape ? layout.recColW : layout.width
                height: page.isLandscape ? layout.height : (recButton.height + Theme.paddingLarge)

                MouseArea {
                    id: recButton
                    width: Theme.iconSizeMedium * 1.2
                    height: Theme.iconSizeMedium * 1.2
                    anchors.centerIn: parent
                    onClicked: {
                        playNgf("feedback_press")
                        if (recorder.recording)
                            recorder.stopRecording()
                        else
                            tryStartRecording()
                    }

                    Rectangle {
                        anchors.fill: parent
                        radius: width / 2
                        color: "transparent"
                        border.width: 2
                        border.color: Theme.primaryColor
                    }

                    Rectangle {
                        anchors.centerIn: parent
                        width: recButton.width * 0.4
                        height: recButton.width * 0.4
                        radius: recorder.recording ? Theme.paddingSmall / 2 : width / 2
                        color: recorder.recording ? Theme.primaryColor : "#e53935"
                    }
                }
            }
        }
    }

    Connections {
        target: appSettings
        onCameraModeChanged: {
            if (!recorder.recording)
                applyCameraDevice()
        }
        onRearDeviceIdChanged: {
            if (!recorder.recording && appSettings.cameraMode === 0)
                applyCameraDevice()
        }
    }

    Timer {
        id: previewStartDelay
        interval: 200
        onTriggered: tryStartPreview()
    }

    function attachGallery() {
        if (pageStack.nextPage(page))
            return
        pageStack.pushAttached(Qt.resolvedUrl("GalleryPage.qml"))
        page.forwardNavigation = !recorder.recording
    }

    function refreshIncidentIcon() {
        if (warningIcon)
            warningIcon.requestPaint()
    }

    onStatusChanged: {
        if (status === PageStatus.Active) {
            if (!appSettings.recordingNoticeAccepted) {
                pageStack.push(Qt.resolvedUrl("RecordingNoticeDialog.qml"), {}, PageStackAction.Immediate)
                return
            }
            attachGallery()
            if (deviceStatus.heatCritical) {
                enforceHeatStop()
                return
            }
            if (deviceStatus.batteryCritical) {
                enforceBatteryStop()
                return
            }
            if (recorder.recording) {
                refreshIncidentIcon()
                return
            }
            applyCameraDevice()
            applyRecorderSettings()
            previewStartDelay.start()
        } else if (status === PageStatus.Inactive && !recorder.recording) {
            recorder.stopPreview()
        }
    }

    Connections {
        target: deviceStatus
        onHeatCriticalChanged: {
            if (deviceStatus.heatCritical)
                enforceHeatStop()
            else {
                if (page.hudNotice === qsTr("Phone too hot — camera stopped"))
                    page.hudNotice = ""
                tryResumePreview()
            }
        }
        onBatteryCriticalChanged: {
            if (deviceStatus.batteryCritical)
                enforceBatteryStop()
            else {
                if (page.hudNotice === qsTr("Battery empty — camera stopped"))
                    page.hudNotice = ""
                tryResumePreview()
            }
        }
        onStorageCriticalChanged: enforceStorageStop()
        onUpdated: {
            if (deviceStatus.heatCritical)
                enforceHeatStop()
            if (deviceStatus.batteryCritical)
                enforceBatteryStop()
            if (deviceStatus.storageCritical)
                enforceStorageStop()
        }
    }

    Connections {
        target: Qt.application
        onStateChanged: {
            if (Qt.application.state === Qt.ApplicationActive && recorder.recording)
                refreshIncidentIcon()
        }
    }
    Component.onDestruction: {
        if (!recorder.recording)
            recorder.stopPreview()
    }
}
