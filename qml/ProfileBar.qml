pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import DrevoPowerConsole

// profile selection and management
Item {
    id: root

    // item the dialogs are centered on
    property Item dialogParent: parent

    // "Profile <n>" that is not used yet
    function unusedName() {
        const names = DeviceManager.profiles.map(profile => profile.name)
        let n = DeviceManager.profiles.length + 1
        while (names.includes(qsTr("Profile %1").arg(n)))
            n++
        return qsTr("Profile %1").arg(n)
    }

    // "<name> (copy)", "<name> (copy 2)", ... that is not used yet;
    // a copy of a copy is numbered instead of getting a second suffix
    function copyName(name) {
        const escape = text => text.replace(/[.*+?^${}()|[\]\\]/g, "\\$&")
        const pattern = template => new RegExp("^" + escape(template).replace("%1", "(.+)").replace("%2", "\\d+") + "$")
        for (const template of [qsTr("%1 (copy)"), qsTr("%1 (copy %2)")]) {
            const match = pattern(template).exec(name)
            if (match) {
                name = match[1]
                break
            }
        }

        const names = DeviceManager.profiles.map(profile => profile.name)
        let copy = qsTr("%1 (copy)").arg(name)
        for (let n = 2; names.includes(copy); n++)
            copy = qsTr("%1 (copy %2)").arg(name).arg(n)
        return copy
    }

    readonly property bool hasProfile: DeviceManager.currentProfile > 0

    // new profile in the shown hardware profile
    function newProfile() {
        nameDialog.openFor("new", root.unusedName())
    }

    readonly property string currentName: {
        for (const profile of DeviceManager.profiles) {
            if (profile.id === DeviceManager.currentProfile)
                return profile.name
        }
        return ""
    }

    implicitWidth: row.implicitWidth
    implicitHeight: row.implicitHeight

    RowLayout {
        id: row
        anchors.fill: parent
        spacing: 4

        ComboBox {
            id: profileCombo
            Layout.fillWidth: true
            Layout.minimumWidth: 80
            // the profiles of the shown hardware profile
            model: DeviceManager.hardwareProfileProfiles
            textRole: "name"
            valueRole: "id"
            displayText: currentIndex < 0 ? qsTr("No profile") : currentText
            onActivated: DeviceManager.currentProfile = currentValue

            function showCurrentProfile() {
                currentIndex = indexOfValue(DeviceManager.currentProfile)
            }

            // the model is replaced when profiles change
            onModelChanged: showCurrentProfile()
            Component.onCompleted: showCurrentProfile()
            Connections {
                target: DeviceManager
                function onCurrentProfileChanged() {
                    profileCombo.showCurrentProfile()
                }
            }
        }

        ToolButton {
            id: menuButton
            text: "⋯"
            font.pointSize: 14
            onClicked: profileMenu.open()

            ToolTip.visible: hovered
            ToolTip.text: qsTr("Manage profiles")

            Menu {
                id: profileMenu
                y: menuButton.height

                MenuItem {
                    text: qsTr("New profile…")
                    onTriggered: root.newProfile()
                }
                MenuItem {
                    text: qsTr("Duplicate…")
                    enabled: root.hasProfile
                    onTriggered: nameDialog.openFor("duplicate", root.copyName(root.currentName))
                }
                MenuItem {
                    text: qsTr("Rename…")
                    enabled: root.hasProfile
                    onTriggered: nameDialog.openFor("rename", root.currentName)
                }
                // writes it to the other hardware profile, the current one keeps its keys
                Menu {
                    title: qsTr("Move to")
                    enabled: root.hasProfile

                    Repeater {
                        model: DeviceManager.hardwareProfiles

                        delegate: MenuItem {
                            required property var modelData
                            required property int index

                            text: modelData.name
                            enabled: index !== DeviceManager.hardwareProfile
                            onTriggered: DeviceManager.moveProfile(DeviceManager.currentProfile, index)
                        }
                    }
                }
                MenuSeparator {}
                MenuItem {
                    text: qsTr("Delete")
                    enabled: root.hasProfile && DeviceManager.profiles.length > 1
                    onTriggered: deleteDialog.open()
                }
            }
        }
    }

    // name of a new or renamed profile
    Dialog {
        id: nameDialog

        // "new", "duplicate" (the current profile) or "rename" (the current profile)
        property string action: "new"

        function openFor(action, name) {
            nameDialog.action = action
            nameField.text = name
            open()
            nameField.selectAll()
            nameField.forceActiveFocus()
        }

        function accept_() {
            const name = nameField.text.trim()
            if (name === "")
                return
            if (action === "new")
                DeviceManager.addProfile(name)
            else if (action === "duplicate")
                DeviceManager.duplicateProfile(DeviceManager.currentProfile, name)
            else
                DeviceManager.renameProfile(DeviceManager.currentProfile, name)
            close()
        }

        parent: root.dialogParent
        anchors.centerIn: parent
        modal: true
        title: action === "new" ? qsTr("New profile")
             : action === "duplicate" ? qsTr("Duplicate profile")
             : qsTr("Rename profile")
        width: 320

        contentItem: TextField {
            id: nameField
            placeholderText: qsTr("Profile name")
            onAccepted: nameDialog.accept_()
        }

        footer: DialogButtonBox {
            Button {
                text: nameDialog.action === "rename" ? qsTr("Rename") : qsTr("Create")
                highlighted: true
                enabled: nameField.text.trim() !== ""
                DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            }
            Button {
                text: qsTr("Cancel")
                DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
            }
            onAccepted: nameDialog.accept_()
            onRejected: nameDialog.close()
        }
    }

    MessageBox {
        id: deleteDialog
        title: qsTr("Delete profile")
        text: qsTr("Delete the profile “%1”?").arg(root.currentName)
        informativeText: qsTr("Its key assignments, lighting and settings are removed.")
        destructive: true
        onAccepted: DeviceManager.removeProfile(DeviceManager.currentProfile)
    }
}
