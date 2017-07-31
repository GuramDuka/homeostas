import QtQuick 2.5
import QtQuick.Controls 2.1
import QtQuick.Window 2.0

ApplicationWindow {
    id: window
    visible: true
    width: 480
    height: 800
    title: "homeostas"

    property int currentDirectoryTracker: -1

    Label {
        id: digitalClock
        text: "00:00"
        z: 100
        smooth: true
        antialiasing: true
        font.family: "Digital-7 Mono"
        font.pointSize: 35
        verticalAlignment: Text.AlignVCenter
        anchors.top: parent.top
        anchors.topMargin: 0
        anchors.right: parent.right
        anchors.rightMargin: 0
        horizontalAlignment: Text.AlignRight
    }

    Timer {
        id: digitalClockTimer
        triggeredOnStart: true
        interval: 1000
        running: true
        repeat: true
        onTriggered: digitalClock.text = Qt.formatDateTime(new Date(), "hh:mm:ss")
    }

    DirectoryTrackerDialog {
        id: directoryTrackerDialog
        onFinished: {
            if( currentDirectoryTracker === -1 )
                directoriesTrackersView.model.append(name, path)
            else
                directoriesTrackersView.model.set(currentDirectoryTracker, name, path)
        }
    }

    Menu {
        id: directoryTrackerMenu
        x: parent.width / 2 - width / 2
        y: parent.height / 2 - height / 2
        modal: true

        Label {
            padding: 10
            font.bold: true
            width: parent.width
            horizontalAlignment: Qt.AlignHCenter
            text: currentDirectoryTracker >= 0 ? directoriesTrackersView.model.get(currentDirectoryTracker).name : ""
        }

        MenuItem {
            text: qsTr("Edit...")
            width: parent.width
            onTriggered: directoryTrackerDialog.editDirectoryTracker(directoriesTrackersView.model.get(currentDirectoryTracker))
        }

        MenuItem {
            text: qsTr("Remove")
            width: parent.width
            onTriggered: directoriesTrackersView.model.remove(currentDirectoryTracker)
        }
    }

    DirectoriesTrackersView {
        id: directoriesTrackersView
        anchors.right: parent.right
        anchors.rightMargin: 0
        anchors.left: parent.left
        anchors.leftMargin: 0
        anchors.topMargin: 0
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 0
        anchors.top: parent.top
        onPressAndHold: {
            currentDirectoryTracker = index
            directoryTrackerMenu.open()
        }
    }

    PlusButton {
        id: createNewDirectoryTracker
        width: 48
        height: 48
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 10
        anchors.right: parent.right
        anchors.rightMargin: 10
    }

    Connections {
        target: createNewDirectoryTracker
        onClicked: {
            currentDirectoryTracker = -1
            directoryTrackerDialog.createDirectoryTracker()
        }
    }
}
