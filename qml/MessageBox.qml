import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

// modal message in the window. Used instead of MessageDialog: a native one closed with the title
// bar button reports no result, stays "visible" and cannot be opened again.
Dialog {
    id: root

    property string text
    property string informativeText
    // the confirmation button turns red: the action cannot be undone
    property bool destructive: false

    parent: Overlay.overlay
    anchors.centerIn: parent
    modal: true
    width: Math.min(420, parent ? parent.width - 32 : 420)
    standardButtons: Dialog.Yes | Dialog.No

    contentItem: ColumnLayout {
        spacing: 8

        Label {
            Layout.fillWidth: true
            text: root.text
            wrapMode: Text.WordWrap
            color: Theme.text
        }
        Label {
            Layout.fillWidth: true
            visible: text !== ""
            text: root.informativeText
            wrapMode: Text.WordWrap
            color: Theme.textMuted
        }
    }

    Component.onCompleted: {
        const yes = standardButton(Dialog.Yes)
        if (yes && destructive)
            yes.Material.foreground = Theme.danger
    }
}
