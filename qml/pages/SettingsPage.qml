import QtQuick 2.0
import Sailfish.Silica 1.0
import ".."

Page {
    id: page
    allowedOrientations: Orientation.All
    Component.onCompleted: cameraDevices.refresh()

    CameraDevices { id: cameraDevices }

    function secondsToIndex(seconds) {
        switch (seconds) {
        case 30: return 0
        case 180: return 2
        case 300: return 3
        default: return 1
        }
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column
            width: page.width
            spacing: Theme.paddingMedium

            PageHeader { title: qsTr("Settings") }

            ComboBox {
                label: qsTr("Pre-incident recording")
                description: qsTr("Time kept before an incident is registered")
                currentIndex: page.secondsToIndex(appSettings.preIncidentSeconds)
                menu: ContextMenu {
                    MenuItem {
                        text: qsTr("30 seconds")
                        onClicked: appSettings.preIncidentSeconds = 30
                    }
                    MenuItem {
                        text: qsTr("1 minute")
                        onClicked: appSettings.preIncidentSeconds = 60
                    }
                    MenuItem {
                        text: qsTr("3 minutes")
                        onClicked: appSettings.preIncidentSeconds = 180
                    }
                    MenuItem {
                        text: qsTr("5 minutes")
                        onClicked: appSettings.preIncidentSeconds = 300
                    }
                }
            }

            ComboBox {
                label: qsTr("Post-incident recording")
                currentIndex: page.secondsToIndex(appSettings.postIncidentSeconds)
                description: qsTr("Extra time kept after an incident is registered")
                menu: ContextMenu {
                    MenuItem {
                        text: qsTr("30 seconds")
                        onClicked: appSettings.postIncidentSeconds = 30
                    }
                    MenuItem {
                        text: qsTr("1 minute")
                        onClicked: appSettings.postIncidentSeconds = 60
                    }
                    MenuItem {
                        text: qsTr("3 minutes")
                        onClicked: appSettings.postIncidentSeconds = 180
                    }
                    MenuItem {
                        text: qsTr("5 minutes")
                        onClicked: appSettings.postIncidentSeconds = 300
                    }
                }
            }

            ComboBox {
                label: qsTr("Video quality")
                currentIndex: appSettings.qualityIndex
                description: qsTr("Lower quality if recording fails or the phone gets hot.")
                menu: ContextMenu {
                    MenuItem {
                        text: qsTr("480p")
                        onClicked: appSettings.qualityIndex = 0
                    }
                    MenuItem {
                        text: qsTr("720p")
                        onClicked: appSettings.qualityIndex = 1
                    }
                    MenuItem {
                        text: qsTr("1080p")
                        onClicked: appSettings.qualityIndex = 2
                    }
                }
            }

            ComboBox {
                label: qsTr("Camera")
                description: qsTr("Front or rear camera for preview and recording.")
                currentIndex: appSettings.cameraMode
                menu: ContextMenu {
                    MenuItem {
                        text: qsTr("Rear")
                        onClicked: appSettings.cameraMode = 0
                    }
                    MenuItem {
                        text: qsTr("Front")
                        enabled: cameraDevices.frontCam
                        onClicked: appSettings.cameraMode = 1
                    }
                }
            }

            ComboBox {
                visible: cameraDevices.backList.length > 1
                label: qsTr("Rear lens")
                description: qsTr("Physical rear camera used for preview and recording.")
                currentIndex: cameraDevices.rearIndex(appSettings.rearDeviceId)
                value: cameraDevices.backLabel(cameraDevices.rearIndex(appSettings.rearDeviceId))
                menu: ContextMenu {
                    MenuItem {
                        visible: cameraDevices.backList.length > 0
                        text: cameraDevices.backList.length > 0 ? cameraDevices.backLabel(0) : ""
                        onClicked: if (cameraDevices.backList.length > 0)
                                       appSettings.rearDeviceId = cameraDevices.backList[0].deviceId
                    }
                    MenuItem {
                        visible: cameraDevices.backList.length > 1
                        text: cameraDevices.backList.length > 1 ? cameraDevices.backLabel(1) : ""
                        onClicked: if (cameraDevices.backList.length > 1)
                                       appSettings.rearDeviceId = cameraDevices.backList[1].deviceId
                    }
                    MenuItem {
                        visible: cameraDevices.backList.length > 2
                        text: cameraDevices.backList.length > 2 ? cameraDevices.backLabel(2) : ""
                        onClicked: if (cameraDevices.backList.length > 2)
                                       appSettings.rearDeviceId = cameraDevices.backList[2].deviceId
                    }
                    MenuItem {
                        visible: cameraDevices.backList.length > 3
                        text: cameraDevices.backList.length > 3 ? cameraDevices.backLabel(3) : ""
                        onClicked: if (cameraDevices.backList.length > 3)
                                       appSettings.rearDeviceId = cameraDevices.backList[3].deviceId
                    }
                }
            }

            ComboBox {
                label: qsTr("Speed")
                description: qsTr("Unit shown on the preview overlay.")
                currentIndex: appSettings.speedUnitKmh ? 0 : 1
                menu: ContextMenu {
                    MenuItem {
                        text: qsTr("km/h")
                        onClicked: appSettings.speedUnitKmh = true
                    }
                    MenuItem {
                        text: qsTr("mph")
                        onClicked: appSettings.speedUnitKmh = false
                    }
                }
            }

            TextSwitch {
                text: qsTr("Record audio")
                description: qsTr("May record voices nearby. Audio stays off until you enable it.")
                checked: appSettings.audioEnabled
                onClicked: appSettings.audioEnabled = checked
            }

            TextSwitch {
                text: qsTr("Keep screen on while recording")
                description: qsTr("The loop keeps running regardless of this option.")
                checked: appSettings.preventDisplayBlanking
                onClicked: appSettings.preventDisplayBlanking = checked
            }

            Slider {
                width: parent.width
                label: qsTr("Crash sensitivity")
                minimumValue: 1.2
                maximumValue: 6.0
                stepSize: 0.1
                value: appSettings.crashThresholdG
                valueText: value.toFixed(1) + " g"
                onReleased: appSettings.crashThresholdG = value
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryColor
                text: qsTr("Higher values ignore potholes. Too high, and a knock will not save an incident while the screen is off.")
            }

            TextSwitch {
                text: qsTr("Thermal protection")
                checked: appSettings.thermalProtectionEnabled
                onClicked: appSettings.thermalProtectionEnabled = checked
            }

            ComboBox {
                label: qsTr("Heat warning")
                enabled: appSettings.thermalProtectionEnabled
                opacity: enabled ? 1.0 : 0.5
                currentIndex: appSettings.heatWarnC === 37 ? 0
                              : (appSettings.heatWarnC === 43 ? 2 : 1)
                description: qsTr("Recording and preview always stop at 45 °C.")
                menu: ContextMenu {
                    MenuItem {
                        text: qsTr("37 °C")
                        onClicked: appSettings.heatWarnC = 37
                    }
                    MenuItem {
                        text: qsTr("40 °C")
                        onClicked: appSettings.heatWarnC = 40
                    }
                    MenuItem {
                        text: qsTr("43 °C")
                        onClicked: appSettings.heatWarnC = 43
                    }
                }
            }

            TextSwitch {
                text: qsTr("Storage protection")
                checked: appSettings.storageProtectionEnabled
                onClicked: appSettings.storageProtectionEnabled = checked
            }

            ComboBox {
                label: qsTr("Storage warning")
                enabled: appSettings.storageProtectionEnabled
                opacity: enabled ? 1.0 : 0.5
                currentIndex: appSettings.storageWarnPercent === 20 ? 0
                              : (appSettings.storageWarnPercent === 15 ? 1 : 2)
                description: qsTr("Recording always stops at 5% free.")
                menu: ContextMenu {
                    MenuItem {
                        text: "20%"
                        onClicked: appSettings.storageWarnPercent = 20
                    }
                    MenuItem {
                        text: "15%"
                        onClicked: appSettings.storageWarnPercent = 15
                    }
                    MenuItem {
                        text: "10%"
                        onClicked: appSettings.storageWarnPercent = 10
                    }
                }
            }

            TextSwitch {
                text: qsTr("Battery protection")
                checked: appSettings.batteryProtectionEnabled
                onClicked: appSettings.batteryProtectionEnabled = checked
            }

            ComboBox {
                label: qsTr("Battery warning")
                enabled: appSettings.batteryProtectionEnabled
                opacity: enabled ? 1.0 : 0.5
                currentIndex: appSettings.batteryWarnPercent === 10 ? 0
                              : (appSettings.batteryWarnPercent === 15 ? 1
                                 : (appSettings.batteryWarnPercent === 30 ? 3 : 2))
                description: qsTr("Recording and preview always stop at 5%.")
                menu: ContextMenu {
                    MenuItem {
                        text: "10%"
                        onClicked: appSettings.batteryWarnPercent = 10
                    }
                    MenuItem {
                        text: "15%"
                        onClicked: appSettings.batteryWarnPercent = 15
                    }
                    MenuItem {
                        text: "25%"
                        onClicked: appSettings.batteryWarnPercent = 25
                    }
                    MenuItem {
                        text: "30%"
                        onClicked: appSettings.batteryWarnPercent = 30
                    }
                }
            }
        }

        VerticalScrollDecorator {}
    }
}
