import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: page
    allowedOrientations: Orientation.All
    property bool selecting: false
    property var selectedPaths: []
    property bool returnAfterPlayer: false

    function deleteIncident(dirPath) {
        clipStorage.removeFile(dirPath)
    }

    function deleteIncidents(paths) {
        clipStorage.removeFiles(paths)
    }

    function deleteAllIncidents() {
        clipStorage.removeFiles(clipStorage.incidentFiles())
    }

    function isSelected(filePath) {
        return selectedPaths.indexOf(filePath) >= 0
    }

    function toggleSelected(filePath) {
        var next = selectedPaths.slice()
        var i = next.indexOf(filePath)
        if (i >= 0)
            next.splice(i, 1)
        else
            next.push(filePath)
        selectedPaths = next
    }

    function clearSelection() {
        selecting = false
        selectedPaths = []
    }

    SilicaListView {
        id: listView
        anchors.fill: parent
        model: clipModel
        header: PageHeader {
            title: page.selecting ? qsTr("Select") : qsTr("Gallery")
            extraContent.children: [
                IconButton {
                    visible: page.selecting
                    anchors.centerIn: parent
                    icon.source: "image://theme/icon-m-cancel"
                    onClicked: page.clearSelection()
                }
            ]
        }

        PullDownMenu {
            MenuItem {
                text: qsTr("Delete")
                visible: page.selecting
                enabled: page.selectedPaths.length > 0
                onClicked: {
                    var paths = page.selectedPaths.slice()
                    page.deleteIncidents(paths)
                    page.clearSelection()
                }
            }
            MenuItem {
                text: qsTr("Select")
                visible: !page.selecting
                enabled: listView.count > 0
                onClicked: {
                    page.selecting = true
                    page.selectedPaths = []
                }
            }
            MenuItem {
                text: qsTr("Delete all")
                visible: !page.selecting
                enabled: listView.count > 0
                onClicked: pageStack.animatorPush(deleteAllDialog)
            }
        }

        ViewPlaceholder {
            enabled: listView.count === 0
            text: qsTr("No incidents yet")
            hintText: qsTr("Crash and manual saves appear here.")
        }

        delegate: ListItem {
            id: delegate
            width: listView.width
            contentHeight: Theme.itemSizeMedium
            property string incidentDir: path
            highlighted: down || (page.selecting && page.isSelected(delegate.incidentDir))
            showMenuOnPressAndHold: !page.selecting
            menu: page.selecting ? null : contextMenu

            ContextMenu {
                id: contextMenu
                MenuItem {
                    text: qsTr("Delete")
                    onClicked: {
                        var dirPath = delegate.incidentDir
                        var deleteIncident = page.deleteIncident
                        delegate.remorseAction(qsTr("Deleting"), function() {
                            deleteIncident(dirPath)
                        })
                    }
                }
            }

            onClicked: {
                if (page.selecting)
                    page.toggleSelected(delegate.incidentDir)
                else {
                    page.returnAfterPlayer = true
                    pageStack.animatorPush(Qt.resolvedUrl("PlayerPage.qml"), {
                        clipPath: delegate.incidentDir,
                        clipPaths: parts
                    })
                }
            }

            Row {
                anchors.verticalCenter: parent.verticalCenter
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                spacing: Theme.paddingMedium

                Icon {
                    visible: page.selecting
                    anchors.verticalCenter: parent.verticalCenter
                    source: page.isSelected(delegate.incidentDir)
                            ? "image://theme/icon-m-acknowledge"
                            : "image://theme/icon-m-tabs"
                    color: page.isSelected(delegate.incidentDir) ? Theme.highlightColor : Theme.secondaryColor
                }

                Column {
                    width: parent.width - (page.selecting ? parent.spacing + Theme.iconSizeMedium : 0)
                    Label {
                        text: title
                        color: delegate.highlighted ? Theme.highlightColor : Theme.primaryColor
                        truncationMode: TruncationMode.Fade
                        width: parent.width
                    }
                    Label {
                        text: subtitle
                        color: Theme.highlightColor
                        font.pixelSize: Theme.fontSizeExtraSmall
                    }
                }
            }
        }

        VerticalScrollDecorator {}
    }

    Component {
        id: deleteAllDialog
        Dialog {
            Column {
                width: parent.width
                DialogHeader {
                    acceptText: qsTr("Delete")
                    title: qsTr("Delete all")
                }
                Label {
                    x: Theme.horizontalPageMargin
                    width: parent.width - 2 * Theme.horizontalPageMargin
                    wrapMode: Text.Wrap
                    color: Theme.highlightColor
                    text: qsTr("Delete all incidents?")
                }
            }
            onAccepted: page.deleteAllIncidents()
        }
    }

    Connections {
        target: pageStack
        onBusyChanged: {
            if (pageStack.busy || !page.returnAfterPlayer)
                return
            if (pageStack.currentPage === page) {
                page.returnAfterPlayer = false
                return
            }
            if (pageStack.depth === 1 && !recorder.recording) {
                page.returnAfterPlayer = false
                pageStack.navigateForward(PageStackAction.Immediate)
            }
        }
    }

    Component.onCompleted: clipModel.refresh()
}
