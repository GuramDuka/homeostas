import QtQuick 2.7
import QtQuick.Controls 2.1
import QtQuick.Window 2.2

Item {
    id    : root
    width : 128
    height: 128

    signal clicked()

    RoundButton {
        id: rb
        radius: 64
        anchors.rightMargin: 0
        anchors.leftMargin: 0
        anchors.bottomMargin: 0
        anchors.topMargin: 0
        anchors.fill: parent
        highlighted: true
        anchors.margins: 10

        Rectangle {
            id: vr
            width: 5
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 5
            anchors.top: parent.top
            anchors.topMargin: 5
            anchors.horizontalCenter: parent.horizontalCenter
            border.width: 0
        }

        Rectangle {
            id: hr
            height: 5
            anchors.right: parent.right
            anchors.rightMargin: 5
            anchors.left: parent.left
            anchors.leftMargin: 5
            anchors.verticalCenter: parent.verticalCenter
            border.width: 0
        }
    }

    Component.onCompleted: {
        //print(Screen.pixelDensity)
        width  = Screen.pixelDensity * 15
        height = Screen.pixelDensity * 15

        vr.anchors.topMargin = Screen.pixelDensity * 3
        vr.anchors.bottomMargin = Screen.pixelDensity * 3
        vr.width = Screen.pixelDensity * 1.9

        hr.anchors.leftMargin = Screen.pixelDensity * 3
        hr.anchors.rightMargin = Screen.pixelDensity * 3
        hr.height = Screen.pixelDensity * 1.75

        rb.radius = Math.max(width, height)

        // http://doc.qt.io/qt-5/qtqml-syntax-signals.html
        rb.clicked.connect(clicked);
    }
}
