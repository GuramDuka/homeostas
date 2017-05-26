import QtQuick 2.7
import QtQuick.Layouts 1.0
import QtQuick.Controls 2.1

GridLayout {
    id: grid
    property alias name: name
    property alias path: path
    property int minimumInputSize: 120
    property string placeholderText: qsTr("<enter>")

    width: parent.width - 4
    x: parent.width / 2 - width / 2
    y: parent.height / 2 - height / 2

    rows: 2
    columns: 2

    Label {
        text: qsTr("Name")
        Layout.alignment: Qt.AlignLeft | Qt.AlignBaseline
    }

    TextField {
        id: name
        focus: true
        Layout.fillWidth: true
        Layout.minimumWidth: grid.minimumInputSize
        Layout.alignment: Qt.AlignLeft | Qt.AlignBaseline
        placeholderText: grid.placeholderText
    }

    Label {
        text: qsTr("Path")
        Layout.alignment: Qt.AlignLeft | Qt.AlignBaseline
    }

    TextField {
        id: path
        Layout.fillWidth: true
        Layout.minimumWidth: grid.minimumInputSize
        Layout.alignment: Qt.AlignLeft | Qt.AlignBaseline
        placeholderText: grid.placeholderText
    }
}
