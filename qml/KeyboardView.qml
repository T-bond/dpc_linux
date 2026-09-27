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
    // side LEDs without a color
    readonly property color unlitLedColor: "#5a5a5a"
    // room for the side LEDs drawn around the image in LightCustom mode
    readonly property int barMargin: 16

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

        delegate: KeyShape {
            required property int index
            required property int keyX
            required property int keyY
            required property int keyWidth
            required property int keyHeight
            required property rect keyLowerRect
            required property color keyColor
            required property bool keySideLed

            readonly property bool customLight: root.mode === KeyboardModel.LightCustom

            // side LEDs are drawn over the image, see below
            visible: !keySideLed && (customLight ? keyColor.a > 0 : index === root.hoverIndex)
            keyRect: Qt.rect(keyX, keyY, keyWidth, keyHeight)
            lowerRect: keyLowerRect
            leftInset: 1
            rightInset: 1
            bottomInset: 6
            color: customLight ? keyColor : root.highlightColor
        }
    }

    Image {
        source: DeviceManager.keyboardImage
    }

    // side LEDs around the keyboard
    Repeater {
        model: root.mode === KeyboardModel.LightCustom ? root.keyboard : null

        delegate: Rectangle {
            required property int keyX
            required property int keyY
            required property int keyWidth
            required property int keyHeight
            required property color keyColor
            required property bool keySideLed

            visible: keySideLed
            x: keyX
            y: keyY
            width: keyWidth
            height: keyHeight
            color: root.frameColor

            Rectangle {
                x: 1
                y: 1
                width: parent.width - 2
                height: parent.height - 2
                radius: 1
                color: parent.keyColor.a > 0 ? parent.keyColor : root.unlitLedColor
            }
        }
    }

    // selected and checked keys and side LEDs, drawn over the keyboard image
    Repeater {
        model: root.mode === KeyboardModel.LightStatic ? null : root.keyboard

        delegate: Item {
            id: keyItem

            required property int index
            required property int keyX
            required property int keyY
            required property int keyWidth
            required property int keyHeight
            required property rect keyLowerRect
            required property bool keyChecked

            readonly property bool selected: root.mode === KeyboardModel.CustomKey && index === root.selectedIndex
            readonly property rect keyRect: Qt.rect(keyX, keyY, keyWidth, keyHeight)

            // light selection
            KeyShape {
                visible: root.mode === KeyboardModel.LightCustom && keyItem.keyChecked
                keyRect: keyItem.keyRect
                lowerRect: keyItem.keyLowerRect
                leftInset: -1
                topInset: -1
                rightInset: -1
                bottomInset: -1
                borderColor: root.highlightColor
                borderWidth: 2
            }

            // assigned or selected key
            KeyShape {
                visible: root.mode === KeyboardModel.CustomKey && (keyItem.selected || keyItem.keyChecked)
                keyRect: keyItem.keyRect
                lowerRect: keyItem.keyLowerRect
                leftInset: 1
                rightInset: 1
                bottomInset: 2
                color: keyItem.keyChecked ? Qt.rgba(1, 225 / 255, 0, 150 / 255) : Qt.rgba(1, 1, 1, 150 / 255)
            }
            KeyShape {
                visible: root.mode === KeyboardModel.CustomKey && (keyItem.selected || keyItem.keyChecked)
                keyRect: keyItem.keyRect
                lowerRect: keyItem.keyLowerRect
                leftInset: 2
                topInset: 2
                rightInset: 2
                bottomInset: 2
                borderColor: root.frameColor
                borderWidth: 1
            }
        }
    }

    // rubber band of a drag selection in LightCustom mode
    Rectangle {
        id: band

        property point start
        property point end

        visible: mouseArea.dragging
        x: Math.min(start.x, end.x)
        y: Math.min(start.y, end.y)
        width: Math.abs(end.x - start.x)
        height: Math.abs(end.y - start.y)
        color: Qt.alpha(root.highlightColor, 0.18)
        border.color: root.highlightColor
        border.width: 1
    }

    MouseArea {
        id: mouseArea

        // a press in LightCustom mode: the key under it, and the keys checked before with Shift or Ctrl
        property int pressIndex: -1
        property bool merge: false
        property var baseSelection: []
        // the press moved far enough to be a drag selection
        property bool dragging: false

        // includes the side LEDs around the image
        anchors.fill: parent
        anchors.margins: -root.barMargin
        enabled: root.mode !== KeyboardModel.LightStatic
        hoverEnabled: true
        // a drag selects keys, it does not scroll the page
        preventStealing: true

        // position in the coordinates of the keyboard image
        function imagePos(mouse) {
            return mouseArea.mapToItem(root, mouse.x, mouse.y)
        }
        // key or side LED under the mouse
        function indexAt(mouse) {
            const pos = imagePos(mouse)
            return root.keyboard.indexAt(pos.x, pos.y, root.mode === KeyboardModel.LightCustom)
        }

        onPositionChanged: (mouse) => {
            root.hoverIndex = indexAt(mouse)
            if (!pressed || root.mode !== KeyboardModel.LightCustom)
                return

            band.end = imagePos(mouse)
            if (!dragging && Math.hypot(band.end.x - band.start.x, band.end.y - band.start.y) >= Application.styleHints.startDragDistance)
                dragging = true
            // the keys the band touches, added to the previous selection with Shift or Ctrl
            if (dragging)
                root.keyboard.checkInRect(Qt.rect(band.x, band.y, band.width, band.height), baseSelection, true)
        }
        onExited: root.hoverIndex = -1

        onPressed: (mouse) => {
            const index = indexAt(mouse)
            root.hoverIndex = index

            if (root.mode === KeyboardModel.CustomKey) {
                if (index !== -1) {
                    root.selectedIndex = index
                    root.keyPressed(index)
                }
                return
            }

            pressIndex = index
            merge = (mouse.modifiers & (Qt.ShiftModifier | Qt.ControlModifier)) !== 0
            baseSelection = merge ? root.keyboard.checkedIndexes() : []
            band.start = imagePos(mouse)
            band.end = band.start
            dragging = false
        }

        // a click without dragging in LightCustom mode
        onReleased: {
            if (root.mode !== KeyboardModel.LightCustom)
                return
            if (dragging) {
                dragging = false
                return
            }

            const index = pressIndex
            // an empty place clears the selection, unless Shift or Ctrl is held
            if (index === -1) {
                if (!merge)
                    root.keyboard.setAllKeyCheck(false)
                return
            }

            root.selectedIndex = index
            // clicking a checked key again unchecks it
            if (root.keyboard.isChecked(index))
                root.keyboard.setCheck(index, false)
            // Shift or Ctrl adds to the checked keys
            else if (merge)
                root.keyboard.setCheck(index, true)
            else
                root.keyboard.checkOnly(index)
        }
        onCanceled: dragging = false
    }
}
