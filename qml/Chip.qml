import QtQuick
import QtQuick.Controls

// small toggle button: filled with the accent color when checked
AbstractButton {
    id: root

    implicitWidth: Math.max(64, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: 34
    leftPadding: 12
    rightPadding: 12
    hoverEnabled: true
    opacity: enabled ? 1 : 0.45
    font.pointSize: 9.5

    ToolTip.visible: hovered && label.truncated
    ToolTip.text: text

    background: Rectangle {
        radius: Theme.smallRadius
        color: root.checked ? Theme.accent : root.hovered ? Theme.surfaceAlt : "transparent"
        border.color: root.checked ? Theme.accent : Theme.border
        Behavior on color { ColorAnimation { duration: 100 } }
    }

    contentItem: Label {
        id: label
        text: root.text
        font: root.font
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
        color: root.checked ? Theme.accentForeground : Theme.text
    }
}
