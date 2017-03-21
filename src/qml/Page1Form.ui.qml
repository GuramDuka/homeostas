import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.0
import com.homeostas.directorytracker 1.0

Item {
    id: item1
    property alias textField1: textField1
    property alias button1: button1
    property alias startTracker: startTracker
    property QDirectoryTracker qDirectoryTracker: QDirectoryTracker {
    }
    property alias stopTracker: stopTracker

    RowLayout {
        anchors.right: parent.right
        anchors.rightMargin: 175
        anchors.left: parent.left
        anchors.leftMargin: 0
        anchors.top: parent.top
        anchors.topMargin: 0

        TextField {
            id: textField1
            placeholderText: qsTr("Text Field")
        }

        Button {
            id: button1
            text: qsTr("Press Me")
        }
    }

    RowLayout {
        width: 100
        height: 100
        transformOrigin: Item.Center

        Button {
            id: startTracker
            text: qsTr("Запустить трекер")
            antialiasing: true
        }

        Button {
            id: stopTracker
            text: qsTr("Остановить трекер")
            antialiasing: true
        }
    }

    Connections {
        target: startTracker
        onClicked: print("clicked")
    }

    Connections {
        target: stopTracker
        onClicked: print("clicked")
    }
}
