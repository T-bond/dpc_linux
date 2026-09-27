pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import DrevoPowerConsole

// function assignment of the Genius-Knob actions
AssignmentPage {
    id: root

    title: "Genius-Knob"
    subtitle: qsTr("Choose what clicking and turning the knob on the side of the keyboard does.")
    contentTitle: qsTr("Knob actions")
    contentSubtitle: qsTr("Select an action to change its function.")
    knob: true

    readonly property var knobActions: [
        { keyValue: 500, text: qsTr("Single-Click"), symbol: "●" },
        { keyValue: 501, text: qsTr("Double-Click"), symbol: "●●" },
        { keyValue: 502, text: qsTr("Forward"), symbol: "↻" },
        { keyValue: 503, text: qsTr("Backward"), symbol: "↺" },
    ]

    // key values of the actions with a custom function
    property var assigned: []

    function loadAssignedKeys() {
        const keys = root.assignedKeys()
        assigned = knobActions.map(action => action.keyValue).filter(keyValue => keys.includes(keyValue))
    }

    onAssignmentChanged: loadAssignedKeys()
    onProfileChanged: loadAssignedKeys()
    Component.onCompleted: loadAssignedKeys()

    RowLayout {
        anchors.fill: parent
        spacing: Theme.spacing

        Image {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredWidth: 3
            source: "qrc:/image/icon/icon_knob.png"
            fillMode: Image.PreserveAspectFit
            mipmap: true
        }

        GridLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredWidth: 2
            columns: 2
            rowSpacing: 12
            columnSpacing: 12

            Repeater {
                model: root.knobActions

                delegate: AbstractButton {
                    id: tile

                    required property var modelData

                    readonly property bool selected: root.currentKeyValue === modelData.keyValue
                    readonly property bool custom: root.assigned.includes(modelData.keyValue)
                    readonly property var info: root.describe(modelData.keyValue, modelData.text)

                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.maximumHeight: 96
                    hoverEnabled: true
                    padding: 12

                    onClicked: root.selectKey(modelData.keyValue, modelData.text)

                    background: Rectangle {
                        radius: Theme.radius
                        color: tile.selected ? Theme.accentSoft : tile.hovered ? Theme.surfaceAlt : "transparent"
                        border.width: tile.selected ? 2 : 1
                        border.color: tile.selected ? Theme.accent : Theme.border
                    }

                    contentItem: RowLayout {
                        spacing: 12

                        Label {
                            Layout.preferredWidth: 28
                            horizontalAlignment: Text.AlignHCenter
                            text: tile.modelData.symbol
                            font.pointSize: 13
                            color: tile.selected ? Theme.accentText : Theme.textMuted
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 0

                            Label {
                                Layout.fillWidth: true
                                text: tile.modelData.text
                                elide: Text.ElideRight
                                font.pointSize: 11
                                font.weight: Font.DemiBold
                                color: Theme.text
                            }
                            Label {
                                Layout.fillWidth: true
                                text: tile.custom ? tile.info.macro : qsTr("Default")
                                elide: Text.ElideRight
                                font.pointSize: 9.5
                                color: tile.custom ? Theme.accentText : Theme.textMuted
                            }
                        }
                        // has a custom function
                        Rectangle {
                            visible: tile.custom
                            Layout.alignment: Qt.AlignTop
                            implicitWidth: 8
                            implicitHeight: 8
                            radius: 4
                            color: Theme.accent
                        }
                    }
                }
            }
        }
    }
}
