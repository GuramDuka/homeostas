import QtQuick 2.5
import QtQuick.Controls 2.1

Dialog {
    id: dialog

    signal finished(string name, string path)
    title: qsTr("Add tracker")

    function createDirectoryTracker() {
        form.name.clear();
        form.path.clear();

        dialog.title = qsTr("Add tracker");
        dialog.open();
    }

    function editDirectoryTracker(tracker) {
        form.name.text = tracker.name;
        form.path.text = tracker.path;

        dialog.title = qsTr("Edit tracker");
        dialog.open();
    }

    width: parent.width - 4
    x: parent.width / 2 - width / 2
    y: parent.height / 2 - height / 2

    focus: true
    modal: true
    standardButtons: Dialog.Ok | Dialog.Cancel

    contentItem: DirectoryTrackerForm {
        id: form
    }

    onAccepted: finished(form.name.text, form.path.text)
}
