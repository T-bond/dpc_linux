pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

// two-level list of the functions that can be assigned to a key
Flickable {
    id: root

    // [{ text, icon, type, children: [{ text, value }] }]
    property var functions: []
    property font font

    // "parent" or "parent/child" index of the selected item
    property string selectedItem: ""

    signal functionPressed(int type, string text)
    signal subFunctionPressed(int type, int value, string text)

    readonly property int rowHeight: 29


    clip: true
    contentHeight: column.height
    boundsBehavior: Flickable.StopAtBounds
    ScrollBar.vertical: ScrollBar {}

    component TreeRow: Rectangle {
        id: row

        property string itemId
        property string text
        property url icon
        property int indent
        property bool expandable: false
        property bool expanded: false

        signal pressed()
        signal toggled()

        readonly property bool selected: root.selectedItem === itemId

        width: root.width
        height: root.rowHeight
        color: selected ? palette.highlight : "transparent"

        Text {
            visible: row.expandable
            x: row.indent
            anchors.verticalCenter: parent.verticalCenter
            text: row.expanded ? "▾" : "▸"
            color: row.selected ? palette.highlightedText : palette.mid
            font.pointSize: 10
        }

        Image {
            id: iconImage
            visible: row.icon.toString() !== ""
            x: row.indent + 17
            anchors.verticalCenter: parent.verticalCenter
            source: row.icon
        }

        Text {
            x: row.indent + 45
            anchors.verticalCenter: parent.verticalCenter
            text: row.text
            font: root.font
            color: row.selected ? palette.highlightedText : palette.text
        }

        MouseArea {
            anchors.fill: parent
            onPressed: (mouse) => {
                if (row.expandable && mouse.x < row.indent + 17)
                    row.toggled()
                else
                    row.pressed()
            }
            onDoubleClicked: if (row.expandable) row.toggled()
        }
    }

    Column {
        id: column
        width: root.width

        Repeater {
            model: root.functions

            delegate: Column {
                id: functionItem

                required property var modelData
                required property int index

                property bool expanded: false

                width: root.width

                TreeRow {
                    itemId: String(functionItem.index)
                    text: functionItem.modelData.text
                    icon: functionItem.modelData.icon
                    indent: 0
                    expandable: functionItem.modelData.children.length > 0
                    expanded: functionItem.expanded

                    onToggled: functionItem.expanded = !functionItem.expanded
                    onPressed: {
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
                        indent: 0

                        onPressed: {
                            root.selectedItem = itemId
                            root.subFunctionPressed(functionItem.modelData.type, modelData.value, modelData.text)
                        }
                    }
                }
            }
        }
    }
}
