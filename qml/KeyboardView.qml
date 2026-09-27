pragma ComponentBehavior: Bound

import QtQuick
import DrevoPowerConsole

// keyboard image with key highlights, see KeyboardModel.ViewMode
Item {
    id: root

    property KeyboardModel keyboard
    property int mode: KeyboardModel.LightStatic
    // color of the whole keyboard in LightStatic mode
    property color lightColor: highlightColor

    property int hoverIndex: -1
    property int selectedIndex: -1

    // key pressed in CustomKey mode
    signal keyPressed(int index)

    readonly property color highlightColor: Qt.rgba(1, 225 / 255, 0, 1)
    // key legends of unlit keys, independent of the theme
    readonly property color unlitColor: "#efefef"
    readonly property color frameColor: Qt.rgba(51 / 255, 51 / 255, 51 / 255, 1)

    width: 698
    height: 256

    onModeChanged: hoverIndex = -1

    // shines through the transparent key legends: the light color, or unlit keys
    Rectangle {
        x: 20
        y: 10
        width: root.width - 40
        height: root.height - 20
        color: root.mode === KeyboardModel.LightStatic ? root.lightColor : root.unlitColor
    }

    // key colors and hover, shine through the keyboard image
    Repeater {
        model: root.mode === KeyboardModel.LightStatic ? null : root.keyboard

        delegate: Rectangle {
            required property int index
            required property int keyX
            required property int keyY
            required property int keyWidth
            required property int keyHeight
            required property color keyColor

            readonly property bool customLight: root.mode === KeyboardModel.LightCustom

            visible: customLight ? keyColor.a > 0 : index === root.hoverIndex
            x: keyX + 1
            y: keyY
            width: keyWidth - 2
            height: keyHeight - 6
            color: customLight ? keyColor : root.highlightColor
        }
    }

    Image {
        source: DeviceManager.keyboardImage
    }

    // selected and checked keys, drawn over the keyboard image
    Repeater {
        model: root.mode === KeyboardModel.LightStatic ? null : root.keyboard

        delegate: Item {
            id: keyItem

            required property int index
            required property int keyX
            required property int keyY
            required property int keyWidth
            required property int keyHeight
            required property bool keyChecked

            readonly property bool selected: root.mode === KeyboardModel.CustomKey && index === root.selectedIndex

            // light selection
            Rectangle {
                visible: root.mode === KeyboardModel.LightCustom && keyItem.keyChecked
                x: keyItem.keyX - 1
                y: keyItem.keyY - 1
                width: keyItem.keyWidth + 2
                height: keyItem.keyHeight + 2
                color: "transparent"
                border.color: root.highlightColor
                border.width: 2
            }

            // assigned or selected key
            Rectangle {
                visible: root.mode === KeyboardModel.CustomKey && (keyItem.selected || keyItem.keyChecked)
                x: keyItem.keyX + 1
                y: keyItem.keyY
                width: keyItem.keyWidth - 2
                height: keyItem.keyHeight - 2
                color: keyItem.keyChecked ? Qt.rgba(1, 225 / 255, 0, 150 / 255) : Qt.rgba(1, 1, 1, 150 / 255)

                Rectangle {
                    x: 1
                    y: 2
                    width: keyItem.keyWidth - 4
                    height: keyItem.keyHeight - 4
                    color: "transparent"
                    border.color: root.frameColor
                    border.width: 1
                }
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        enabled: root.mode !== KeyboardModel.LightStatic
        hoverEnabled: true

        onPositionChanged: (mouse) => root.hoverIndex = root.keyboard.indexAt(mouse.x, mouse.y)
        onExited: root.hoverIndex = -1

        onPressed: (mouse) => {
            const index = root.keyboard.indexAt(mouse.x, mouse.y)
            root.hoverIndex = index
            if (index === -1)
                return

            root.selectedIndex = index
            if (root.mode === KeyboardModel.CustomKey)
                root.keyPressed(index)
            // clicking a checked key again unchecks it
            else if (root.keyboard.isChecked(index))
                root.keyboard.setCheck(index, false)
            // Shift or Ctrl adds to the checked keys
            else if (mouse.modifiers & (Qt.ShiftModifier | Qt.ControlModifier))
                root.keyboard.setCheck(index, true)
            else
                root.keyboard.checkOnly(index)
        }
    }
}
