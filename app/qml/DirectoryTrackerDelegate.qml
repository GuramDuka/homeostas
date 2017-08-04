import QtQuick 2.8
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2

ItemDelegate {
    id: delegate
    checkable: true

    contentItem: ColumnLayout {
        spacing: 4

        Label {
            text: name
            font.bold: true
            elide: Text.ElideRight
            Layout.fillWidth: true
        }

        GridLayout {
            id: grid
            visible: false

            columns: 3
            rowSpacing: 4
            columnSpacing: 4

            Label {
                text: qsTr("Key")
                Layout.leftMargin: 10
            }

            Label {
                text: ":"
            }

            Label {
                text: key
                elide: Text.ElideRight
                Layout.fillWidth: true
            }

            Label {
                text: qsTr("Path")
                Layout.leftMargin: 10
            }

            Label {
                text: ":"
            }

            Label {
                text: path
                //font.bold: true
                elide: Text.ElideRight
                Layout.fillWidth: true
            }

            Label {
                text: qsTr("Remote")
                Layout.leftMargin: 10
                visible: is_remote
            }

            Label {
                text: ":"
                visible: is_remote
            }

            Label {
                text: remote
                //font.bold: true
                elide: Text.ElideRight
                Layout.fillWidth: true
                visible: is_remote
            }
        }
    }

    states: [
        State {
            name: "expanded"
            when: delegate.checked

            PropertyChanges {
                target: grid
                visible: true
            }
        }
    ]
}
