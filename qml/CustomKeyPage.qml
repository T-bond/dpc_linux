pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import DrevoPowerConsole

// function assignment of the keyboard keys
AssignmentPage {
    id: root

    title: qsTr("Key assignment")
    subtitle: qsTr("Give any key a new function: another key or shortcut, a mouse button, media control, or switch it off.")

    onAssignmentChanged: (keyValue, assigned) => keyboardModel.setKeyCheck(keyValue, assigned)

    KeyboardModel {
        id: keyboardModel
    }

    KeyboardStage {
        anchors.fill: parent
        anchors.bottomMargin: legend.height + 8
        keyboard: keyboardModel
        mode: KeyboardModel.CustomKey

        onKeyPressed: (index) => root.selectKey(keyboardModel.keyValue(index), keyboardModel.keyText(index))
    }

    // colors of the key highlights, see KeyboardView
    Row {
        id: legend
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 20

        Label {
            text: qsTr("Click a key to select it.")
            font.pointSize: 9
            color: Theme.textMuted
        }

        component LegendItem: Row {
            property color swatch
            property alias text: legendLabel.text
            spacing: 6

            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width: 12
                height: 12
                radius: 3
                color: parent.swatch
                border.color: Theme.border
            }
            Label {
                id: legendLabel
                font.pointSize: 9
                color: Theme.textMuted
            }
        }

        LegendItem {
            swatch: Qt.rgba(1, 225 / 255, 0, 150 / 255)
            text: qsTr("Custom function")
        }
        LegendItem {
            swatch: Qt.rgba(1, 1, 1, 150 / 255)
            text: qsTr("Selected")
        }
    }

    function loadAssignedKeys() {
        keyboardModel.setAllKeyCheck(false)
        for (const keyValue of root.assignedKeys())
            keyboardModel.setKeyCheck(keyValue, true)
    }

    onProfileChanged: loadAssignedKeys()
    Component.onCompleted: loadAssignedKeys()
}
