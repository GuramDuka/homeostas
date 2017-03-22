import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.0
import com.homeostas.directorytracker 1.0

ApplicationWindow {
    id: applicationWindow
    visible: true
    width: 480
    height: 800
    title: qsTr("homeostas")

    property QDirectoryTracker qDirectoryTracker: QDirectoryTracker {
        directoryUserDefinedName: "hiew"
        directoryPathName: "c:\\hiew"
    }

    Button {
        id: startTracker
        text: qsTr("Запустить трекер")
        anchors.left: parent.left
        anchors.leftMargin: 0
        anchors.top: parent.top
        anchors.topMargin: 0
    }

    Button {
        id: stopTracker
        text: qsTr("Остановить трекер")
        anchors.left: startTracker.right
        anchors.leftMargin: 2
        transformOrigin: Item.Center
    }

    Label {
        id: digitalClock
        text: "00:00"
        smooth: true
        antialiasing: true
        font.family: "Digital-7"
        font.pointSize: 35
        verticalAlignment: Text.AlignVCenter
        anchors.top: parent.top
        anchors.topMargin: 0
        anchors.right: parent.right
        anchors.rightMargin: 0
        horizontalAlignment: Text.AlignRight
    }

    Connections {
        target: startTracker
        onClicked: qDirectoryTracker.startTracker()
    }

    Connections {
        target: stopTracker
        onClicked: qDirectoryTracker.stopTracker()
    }

    Timer {
        triggeredOnStart: true
        interval: 1000
        running: true
        repeat: true
        onTriggered: digitalClock.text = Qt.formatDateTime(new Date(), "hh:mm:ss")
    }
}
