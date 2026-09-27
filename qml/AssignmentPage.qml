pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import DrevoPowerConsole

// function assignment for keys or knob actions; the page content selects the key
Control {
    id: root

    property string title
    property string subtitle
    // title of the card around the content
    property string contentTitle
    property string contentSubtitle
    property alias knob: assignment.knob
    // key selection, fills the content card
    default property alias content: contentArea.data

    // selected key, 0 for none
    property int currentKeyValue: 0
    property string currentKeyName: ""
    // assignment of the selected key: { function, macro }
    property var currentInfo: ({ function: "", macro: "" })

    // function selected in the tree
    property int macroType: 0
    property int macroValue: 0
    property string macroDesc: ""
    readonly property bool functionSelected: tree.selectedItem !== ""

    // changes when stored assignments change, for bindings that describe keys
    property int revision: 0

    // result of the last Assign or Restore, shown below the buttons
    property string statusText: ""

    // assignment of a key was saved: assigned = key has a custom function
    signal assignmentChanged(int keyValue, bool assigned)
    // assignments of another profile
    signal profileChanged()

    // show the assignment of a key
    function selectKey(keyValue, keyName) {
        currentKeyValue = keyValue
        currentKeyName = keyName
        currentInfo = assignment.describe(keyValue, keyName)
        statusText = ""
    }

    function assignedKeys() {
        return assignment.assignedKeys()
    }

    // current assignment of any key: { function, macro }
    function describe(keyValue, keyName) {
        void root.revision
        return assignment.describe(keyValue, keyName)
    }

    function applyResult(result) {
        if (result === KeyAssignment.NotSaved) {
            statusText = qsTr("Choose the key to send.")
            return
        }
        if (result === KeyAssignment.Assigned)
            root.assignmentChanged(currentKeyValue, true)
        else if (result === KeyAssignment.Unassigned)
            root.assignmentChanged(currentKeyValue, false)
        revision++
        selectKey(currentKeyValue, currentKeyName)
        statusText = result === KeyAssignment.Assigned ? qsTr("Assigned.") : qsTr("Default function restored.")
    }

    padding: Theme.pageMargin
    leftPadding: Math.max(Theme.pageMargin, (width - Theme.maximumContentWidth) / 2)
    rightPadding: leftPadding
    font.family: Theme.fontFamily

    KeyAssignment {
        id: assignment
    }

    Connections {
        target: DeviceManager
        function onCurrentProfileChanged() {
            root.revision++
            if (root.currentKeyValue > 0)
                root.selectKey(root.currentKeyValue, root.currentKeyName)
            root.profileChanged()
        }
    }

    contentItem: ColumnLayout {
        spacing: Theme.spacing

        PageHeader {
            Layout.fillWidth: true
            title: root.title
            subtitle: root.subtitle
        }

        Card {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: Theme.minimumStageHeight
            Layout.maximumHeight: Theme.maximumStageHeight
            fillContent: true
            title: root.contentTitle
            subtitle: root.contentSubtitle

            Item {
                id: contentArea
                Layout.fillWidth: true
                Layout.fillHeight: true
            }
        }

        RowLayout {
            Layout.fillWidth: true
            // never shorter than its content, grows (longer list) once the keyboard card is at its maximum
            Layout.fillHeight: true
            Layout.minimumHeight: 300
            Layout.preferredHeight: 300
            spacing: Theme.spacing

            Card {
                Layout.fillWidth: true
                Layout.preferredWidth: 600
                Layout.minimumWidth: 360
                Layout.fillHeight: true
                fillContent: true
                title: qsTr("Function")
                subtitle: qsTr("What the selected key does. Open a group to see its functions.")

                AssignmentTree {
                    id: tree
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    functions: assignment.functionTree

                    onFunctionPressed: (type, text) => {
                        root.macroType = type
                        root.macroValue = 0
                        root.macroDesc = text
                        root.statusText = ""
                    }
                    onSubFunctionPressed: (type, value, text) => {
                        root.macroType = type
                        root.macroValue = value
                        root.macroDesc = text
                        root.statusText = ""
                    }
                }
            }

            Card {
                Layout.fillWidth: true
                Layout.preferredWidth: 400
                Layout.minimumWidth: 340
                Layout.maximumWidth: 480
                Layout.fillHeight: true
                fillContent: true
                title: root.knob ? qsTr("Selected action") : qsTr("Selected key")

                // nothing selected yet
                ColumnLayout {
                    visible: root.currentKeyValue <= 0
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    Label {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        wrapMode: Text.WordWrap
                        text: root.knob ? qsTr("Select a knob action above.") : qsTr("Click a key on the keyboard above.")
                        font.pointSize: 10.5
                        color: Theme.textMuted
                    }
                }

                ColumnLayout {
                    visible: root.currentKeyValue > 0
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 10

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12

                        // key cap
                        Rectangle {
                            Layout.preferredHeight: 44
                            Layout.preferredWidth: Math.max(44, keyCapLabel.implicitWidth + 24)
                            Layout.maximumWidth: 180
                            radius: Theme.smallRadius
                            color: Theme.surfaceAlt
                            border.color: Theme.border

                            Label {
                                id: keyCapLabel
                                anchors.fill: parent
                                anchors.margins: 6
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                                text: root.currentKeyName
                                font.pointSize: 11
                                font.weight: Font.Bold
                                color: Theme.text
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 0
                            Label {
                                text: root.currentInfo.function
                                font.pointSize: 8.5
                                color: Theme.textMuted
                            }
                            Label {
                                Layout.fillWidth: true
                                text: root.currentInfo.macro
                                elide: Text.ElideRight
                                font.pointSize: 11
                                font.weight: Font.DemiBold
                                color: Theme.text
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: 1
                        color: Theme.border
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Label {
                            text: qsTr("New function")
                            font.pointSize: 9.5
                            color: Theme.textMuted
                        }
                        Label {
                            Layout.fillWidth: true
                            horizontalAlignment: Text.AlignRight
                            elide: Text.ElideRight
                            text: root.functionSelected ? root.macroDesc : qsTr("Choose one in the list")
                            font.pointSize: 10
                            font.weight: root.functionSelected ? Font.DemiBold : Font.Normal
                            color: root.functionSelected ? Theme.accentText : Theme.textMuted
                        }
                    }

                    // combo key of "Keyboard function"
                    GridLayout {
                        visible: root.functionSelected && root.macroType === KeyAssignment.KeyboardFunction
                        Layout.fillWidth: true
                        columns: 3
                        columnSpacing: 8
                        rowSpacing: 6

                        Label {
                            text: qsTr("Modifiers")
                            font.pointSize: 9.5
                            color: Theme.textMuted
                        }
                        ComboBox {
                            id: sysKey1Combo
                            Layout.fillWidth: true
                            model: assignment.sysKeys
                            textRole: "text"
                        }
                        ComboBox {
                            id: sysKey2Combo
                            Layout.fillWidth: true
                            model: assignment.sysKeys
                            textRole: "text"
                        }
                        Label {
                            text: qsTr("Key")
                            font.pointSize: 9.5
                            color: Theme.textMuted
                        }
                        ComboBox {
                            id: newKeyCombo
                            Layout.fillWidth: true
                            Layout.columnSpan: 2
                            model: assignment.allKeys
                            textRole: "text"
                        }
                    }

                    Item {
                        Layout.fillHeight: true
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Label {
                            Layout.fillWidth: true
                            text: root.statusText
                            elide: Text.ElideRight
                            font.pointSize: 9.5
                            color: Theme.textMuted
                        }
                        Button {
                            text: qsTr("Restore default")
                            flat: true
                            onClicked: root.applyResult(assignment.restoreDefault(root.currentKeyValue))
                        }
                        Button {
                            text: qsTr("Assign")
                            highlighted: true
                            enabled: root.functionSelected
                            onClicked: root.applyResult(assignment.save(root.currentKeyValue, root.macroType, root.macroValue, root.macroDesc,
                                                                        newKeyCombo.currentIndex, sysKey1Combo.currentIndex,
                                                                        sysKey2Combo.currentIndex))
                        }
                    }
                }
            }
        }
    }
}
