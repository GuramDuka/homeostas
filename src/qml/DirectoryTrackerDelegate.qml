import QtQuick 2.7
import QtQuick.Layouts 1.0
import QtQuick.Controls 2.1

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

            columns: 2
            rowSpacing: 4
            columnSpacing: 4

            Label {
                text: qsTr("Path:")
                Layout.leftMargin: 10
            }

            Label {
                text: path
                font.bold: true
                elide: Text.ElideRight
                Layout.fillWidth: true
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
