import QtQuick 2.5
import QtQuick.Controls 2.1
//import Backend 1.0

ListView {
    id: listView

    signal pressAndHold(int index)

    width: 320
    height: 480

    focus: true
    boundsBehavior: Flickable.StopAtBounds

    section.property: "name"
    section.criteria: ViewSection.FirstCharacter
    section.delegate: SectionDelegate {
        width: listView.width
    }

    delegate: DirectoryTrackerDelegate {
        id: delegate
        width: listView.width

        Connections {
            target: delegate
            onPressAndHold: listView.pressAndHold(index)
        }
    }

    model: directoriesTrackersModel

    ScrollBar.vertical: ScrollBar { }
}
