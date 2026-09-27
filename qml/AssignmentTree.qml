pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Shapes

// two-level list of the functions that can be assigned to a key
Flickable {
    id: root

    // [{ text, icon, type, children: [{ text, value }] }]
    property var functions: []

    // "parent" or "parent/child" index of the selected item
    property string selectedItem: ""

    signal functionPressed(int type, string text)
    signal subFunctionPressed(int type, int value, string text)

    readonly property int rowHeight: 34
    // rows always leave room for the scroll bar, so they keep their width when groups open and close
    readonly property real rowWidth: root.width - scrollBar.width - 4

    clip: true
    contentHeight: column.height
    boundsBehavior: Flickable.StopAtBounds
    ScrollBar.vertical: ScrollBar {
        id: scrollBar
    }

    component TreeRow: ItemDelegate {
        id: row

        property string itemId
        property url iconSource
        property bool child: false
        property bool expandable: false
        property bool expanded: false

        readonly property bool selected: root.selectedItem === itemId

        width: root.rowWidth
        height: root.rowHeight
        padding: 0
        hoverEnabled: true

        background: Rectangle {
            radius: Theme.smallRadius
            color: row.selected ? Theme.accentSoft : row.hovered ? Theme.surfaceAlt : "transparent"
        }

        contentItem: RowLayout {
            spacing: 10

            Item {
                Layout.leftMargin: row.child ? 34 : 6
                Layout.preferredWidth: 18
                Layout.preferredHeight: 18

                TintedIcon {
                    anchors.fill: parent
                    visible: !row.child
                    icon: row.iconSource
                    color: row.selected ? Theme.accentText : Theme.textMuted
                }
                // dot of the second level
                Rectangle {
                    visible: row.child
                    anchors.centerIn: parent
                    width: 5
                    height: 5
                    radius: 3
                    color: row.selected ? Theme.accentText : Theme.border
                }
            }

            Label {
                Layout.fillWidth: true
                text: row.text
                elide: Text.ElideRight
                font.pointSize: 10.5
                font.weight: row.selected ? Font.DemiBold : Font.Normal
                color: Theme.text
            }

            // chevron of a group, centered in its square so it turns in place
            Shape {
                visible: row.expandable
                Layout.alignment: Qt.AlignVCenter
                Layout.rightMargin: 8
                Layout.preferredWidth: 12
                Layout.preferredHeight: 12
                preferredRendererType: Shape.CurveRenderer
                rotation: row.expanded ? 90 : 0
                Behavior on rotation { NumberAnimation { duration: 120 } }

                ShapePath {
                    strokeColor: Theme.textMuted
                    strokeWidth: 1.6
                    fillColor: "transparent"
                    capStyle: ShapePath.RoundCap
                    joinStyle: ShapePath.RoundJoin
                    startX: 4.5; startY: 2.5
                    PathLine { x: 8; y: 6 }
                    PathLine { x: 4.5; y: 9.5 }
                }
            }
        }
    }

    Column {
        id: column
        width: root.rowWidth
        spacing: 2

        Repeater {
            model: root.functions

            delegate: Column {
                id: functionItem

                required property var modelData
                required property int index

                property bool expanded: false

                width: root.rowWidth
                spacing: 2

                TreeRow {
                    itemId: String(functionItem.index)
                    text: functionItem.modelData.text
                    iconSource: functionItem.modelData.icon
                    expandable: functionItem.modelData.children.length > 0
                    expanded: functionItem.expanded

                    // groups open and close, the others are functions
                    onClicked: {
                        if (expandable) {
                            functionItem.expanded = !functionItem.expanded
                            return
                        }
                        root.selectedItem = itemId
                        root.functionPressed(functionItem.modelData.type, functionItem.modelData.text)
                    }
                }

                Repeater {
                    model: functionItem.expanded ? functionItem.modelData.children : []

                    delegate: TreeRow {
                        required property var modelData
                        required property int index

                        itemId: functionItem.index + "/" + index
                        text: modelData.text
                        child: true

                        onClicked: {
                            root.selectedItem = itemId
                            root.subFunctionPressed(functionItem.modelData.type, modelData.value, modelData.text)
                        }
                    }
                }
            }
        }
    }
}
