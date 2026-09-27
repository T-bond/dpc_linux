import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// page entry of the navigation rail
AbstractButton {
    id: root

    // tab sprite in image/button/
    property string iconName

    checkable: true
    autoExclusive: true
    implicitHeight: 42
    hoverEnabled: true
    focusPolicy: Qt.TabFocus

    background: Rectangle {
        radius: Theme.smallRadius
        color: root.checked ? Theme.accentSoft
             : root.hovered ? Theme.surfaceAlt : "transparent"

        // selection marker
        Rectangle {
            visible: root.checked
            x: 0
            anchors.verticalCenter: parent.verticalCenter
            width: 3
            height: 18
            radius: 2
            color: Theme.accent
        }
        Behavior on color { ColorAnimation { duration: 120 } }
    }

    contentItem: RowLayout {
        spacing: 10

        TintedIcon {
            Layout.leftMargin: 6
            Layout.preferredWidth: 30
            Layout.preferredHeight: 30
            sourceSize: Qt.size(30, 30)
            icon: "image/button/" + root.iconName + ".png"
            color: root.checked ? Theme.accentText : Theme.textMuted
        }
        Label {
            Layout.fillWidth: true
            text: root.text
            elide: Text.ElideRight
            font.pointSize: 10.5
            font.weight: root.checked ? Font.DemiBold : Font.Normal
            color: root.checked ? Theme.text : Theme.textMuted
        }
    }
}
