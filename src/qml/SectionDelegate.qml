import QtQuick 2.7
import QtQuick.Controls 2.1

ToolBar {
    id: background

    Label {
        id: label
        text: section
        anchors.fill: parent
        horizontalAlignment: Qt.AlignHCenter
        verticalAlignment: Qt.AlignVCenter
    }
}
