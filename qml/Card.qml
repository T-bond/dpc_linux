pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// rounded panel with an optional title row; children are laid out in a column below the title
Pane {
    id: root

    property string title
    property string subtitle
    // the content fills the card height; otherwise it stays at the top
    property bool fillContent: false
    // items on the right of the title
    property alias actions: actionRow.data
    default property alias content: body.data

    padding: 20
    topPadding: title === "" ? 20 : 16

    background: Rectangle {
        color: Theme.surface
        radius: Theme.radius
        border.color: Theme.border
    }

    contentItem: ColumnLayout {
        spacing: 14

        RowLayout {
            visible: root.title !== ""
            Layout.fillWidth: true
            spacing: 12

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                Label {
                    Layout.fillWidth: true
                    text: root.title
                    font.pointSize: 12
                    font.weight: Font.DemiBold
                    color: Theme.text
                }
                Label {
                    visible: root.subtitle !== ""
                    Layout.fillWidth: true
                    text: root.subtitle
                    wrapMode: Text.WordWrap
                    font.pointSize: 9
                    color: Theme.textMuted
                }
            }

            RowLayout {
                id: actionRow
                spacing: 8
            }
        }

        ColumnLayout {
            id: body
            visible: children.length > 0
            Layout.fillWidth: true
            Layout.fillHeight: root.fillContent
            spacing: 12
        }

        Item {
            visible: !root.fillContent && body.visible
            Layout.fillHeight: true
        }
    }
}
