pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import DrevoPowerConsole

// function assignment for keys or knob actions; the page content selects the key
Control {
    id: root

    property alias caption: captionLabel.text
    property alias knob: assignment.knob
    default property alias content: contentArea.data

    // selected key
    property int currentKeyValue: 0
    property string currentKeyName: ""

    // function selected in the tree
    property int macroType: 0
    property int macroValue: 0
    property string macroDesc: ""

    // assignment of a key was saved: assigned = key has a custom function
    signal assignmentChanged(int keyValue, bool assigned)

    // show the assignment of a key
    function selectKey(keyValue, keyName) {
        currentKeyValue = keyValue
        currentKeyName = keyName
        const info = assignment.describe(keyValue, keyName)
        keyNameLabel.text = keyName
        keyFunctionLabel.text = info.function
        keyMacroLabel.text = info.macro
    }

    function assignedKeys() {
        return assignment.assignedKeys()
    }

    function applyResult(result) {
        if (result === KeyAssignment.NoKeySelected)
            noKeyDialog.open()
        else if (result === KeyAssignment.Assigned)
            root.assignmentChanged(currentKeyValue, true)
        else if (result === KeyAssignment.Unassigned)
            root.assignmentChanged(currentKeyValue, false)
    }

    width: 1135
    height: 650
    padding: 0
    font.family: "Open Sans"
    font.pointSize: 12

    KeyAssignment {
        id: assignment
    }

    // assignments of another profile
    signal profileChanged()

    Connections {
        target: DeviceManager
        function onCurrentProfileChanged() {
            if (root.currentKeyValue > 0)
                root.selectKey(root.currentKeyValue, root.currentKeyName)
            root.profileChanged()
        }
    }


    Label {
        id: captionLabel
        x: 5
        y: 5
        font.pointSize: 16
        font.bold: true
    }

    Label {
        x: 5
        y: 45
        text: qsTr("Key Assignment")
        font.pointSize: 14
    }

    AssignmentTree {
        x: 5
        y: 70
        width: 252
        height: 480
        font.family: "Open Sans"
        font.pointSize: 14
        functions: assignment.functionTree

        onFunctionPressed: (type, text) => {
            root.macroType = type
            root.macroValue = 0
            root.macroDesc = text
            comboKeyFrame.visible = type === KeyAssignment.KeyboardFunction
        }
        onSubFunctionPressed: (type, value, text) => {
            root.macroType = type
            root.macroValue = value
            root.macroDesc = text
        }
    }

    // combo key selection of "Keyboard function"
    Item {
        id: comboKeyFrame
        x: 0
        y: 550
        width: 260
        height: 70
        visible: false

        Label {
            x: 5
            y: 5
            width: 80
            height: 25
            verticalAlignment: Text.AlignVCenter
            text: qsTr("combox key")
            font.pointSize: 10
        }
        ComboBox {
            id: sysKey1Combo
            x: 90
            y: 5
            width: 80
            height: 25
            font.pointSize: 10
            model: assignment.sysKeys
            textRole: "text"
        }
        ComboBox {
            id: sysKey2Combo
            x: 175
            y: 5
            width: 80
            height: 25
            font.pointSize: 10
            model: assignment.sysKeys
            textRole: "text"
        }
        Label {
            x: 5
            y: 40
            width: 80
            height: 25
            verticalAlignment: Text.AlignVCenter
            text: qsTr("new key")
            font.pointSize: 10
        }
        ComboBox {
            id: newKeyCombo
            x: 90
            y: 40
            width: 165
            height: 25
            font.pointSize: 10
            model: assignment.allKeys
            textRole: "text"
        }
    }

    Button {
        x: 60
        y: 620
        width: 90
        height: 25
        text: qsTr("Cancel")
        onClicked: root.applyResult(assignment.restoreDefault(root.currentKeyValue))
    }

    Button {
        x: 167
        y: 620
        width: 90
        height: 25
        text: qsTr("Save")
        onClicked: root.applyResult(assignment.save(root.currentKeyValue, root.macroType, root.macroValue, root.macroDesc,
                                                    newKeyCombo.currentIndex, sysKey1Combo.currentIndex,
                                                    sysKey2Combo.currentIndex))
    }

    // current assignment of the selected key
    Rectangle {
        x: 589
        y: 50
        width: 200
        height: 100
        color: palette.base
        border.color: palette.mid

        Label {
            id: keyNameLabel
            x: 10
            y: 10
            width: 180
            height: 22
            horizontalAlignment: Text.AlignHCenter
            font.pointSize: 14
            color: palette.text
        }
        Label {
            id: keyFunctionLabel
            x: 10
            y: 40
            width: 180
            height: 22
            horizontalAlignment: Text.AlignHCenter
            font.pointSize: 14
            color: palette.text
        }
        Label {
            id: keyMacroLabel
            x: 10
            y: 70
            width: 180
            height: 22
            horizontalAlignment: Text.AlignHCenter
            font.pointSize: 14
            color: palette.text
        }
    }

    Item {
        id: contentArea
        anchors.fill: parent
    }

    MessageDialog {
        id: noKeyDialog
        title: qsTr("Key Assignment")
        text: qsTr("Select keyboard button")
        buttons: MessageDialog.Ok
    }
}
