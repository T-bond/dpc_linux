pragma ComponentBehavior: Bound

import QtQuick
import DrevoPowerConsole

// function assignment of the keyboard keys
AssignmentPage {
    id: root

    caption: qsTr("Customization")

    onAssignmentChanged: (keyValue, assigned) => keyboardModel.setKeyCheck(keyValue, assigned)

    KeyboardModel {
        id: keyboardModel
    }

    KeyboardView {
        x: 340
        y: 160
        keyboard: keyboardModel
        mode: KeyboardModel.CustomKey

        onKeyPressed: (index) => root.selectKey(keyboardModel.keyValue(index), keyboardModel.keyText(index))
    }

    function loadAssignedKeys() {
        keyboardModel.setAllKeyCheck(false)
        for (const keyValue of root.assignedKeys())
            keyboardModel.setKeyCheck(keyValue, true)
    }

    onProfileChanged: loadAssignedKeys()
    Component.onCompleted: loadAssignedKeys()
}
