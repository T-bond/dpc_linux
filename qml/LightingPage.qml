pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import DrevoPowerConsole

// backlight effects
Control {
    id: root

    width: 1135
    height: 650
    padding: 0


    KeyboardModel {
        id: keyboardModel
    }

    Lighting {
        id: lighting
        keyboard: keyboardModel
    }

    Label {
        x: 5
        y: 5
        text: "Radi-RGB"
        font.family: "Open Sans"
        font.pointSize: 16
        font.bold: true
    }

    Label {
        x: 5
        y: 45
        text: qsTr("Color Effect")
        font.family: "Open Sans"
        font.pointSize: 14
    }

    ListView {
        x: 5
        y: 70
        width: 252
        height: 570
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        model: lighting.modes
        currentIndex: lighting.modeIndex

        delegate: Rectangle {
            id: modeItem

            required property var modelData
            required property int index

            readonly property bool current: ListView.isCurrentItem

            width: ListView.view.width
            height: 29
            color: current ? palette.highlight : "transparent"

            Image {
                x: 2
                anchors.verticalCenter: parent.verticalCenter
                source: modeItem.modelData.icon
            }

            Text {
                x: 30
                anchors.verticalCenter: parent.verticalCenter
                text: modeItem.modelData.text
                font.family: "Open Sans"
                font.pointSize: 14
                color: modeItem.current ? palette.highlightedText : palette.text
            }

            MouseArea {
                anchors.fill: parent
                onClicked: lighting.selectModeIndex(modeItem.index)
            }
        }
    }

    KeyboardView {
        x: 340
        y: 160
        keyboard: keyboardModel
        mode: lighting.customMode ? KeyboardModel.LightCustom : KeyboardModel.LightStatic
        lightColor: lighting.lightColor
    }

    Rectangle {
        x: 275
        y: 435
        width: 840
        height: 211
        color: palette.base

        Label {
            x: 10
            y: 10
            text: qsTr("Brightness")
            font.family: "Open Sans"
            font.pointSize: 14
            color: palette.text
        }
        Slider {
            x: 10
            y: 40
            width: 200
            height: 16
            from: 0
            to: 15
            stepSize: 1
            snapMode: Slider.SnapAlways
            value: lighting.brightness
            onMoved: lighting.setBrightness(value)
        }

        Label {
            x: 10
            y: 80
            text: qsTr("Speed")
            font.family: "Open Sans"
            font.pointSize: 14
            color: palette.text
        }
        Slider {
            x: 10
            y: 110
            width: 200
            height: 16
            from: 0
            to: 4
            stepSize: 1
            snapMode: Slider.SnapAlways
            enabled: lighting.speedEnabled
            value: lighting.speed
            onMoved: lighting.setSpeed(value)
        }

        // rainbow direction, in place of the color settings (the rainbow has no color)
        Label {
            x: 10
            y: 140
            visible: lighting.directionVisible
            text: qsTr("Direction")
            color: palette.text
        }
        ComboBox {
            id: directionCombo
            x: 10
            y: 165
            width: 200
            height: 25
            visible: lighting.directionVisible
            model: lighting.directions
            textRole: "text"
            valueRole: "value"
            onActivated: lighting.setDirection(currentValue)
            Component.onCompleted: currentIndex = indexOfValue(lighting.direction)

            // selecting an item breaks a binding, so follow the setting explicitly
            Connections {
                target: lighting
                function onSettingsChanged() {
                    directionCombo.currentIndex = directionCombo.indexOfValue(lighting.direction)
                }
            }
        }

        // custom color switch, its label toggles it too
        CheckBox {
            x: 10
            y: 140
            height: 23
            padding: 0
            spacing: 6
            visible: lighting.customColorVisible
            text: qsTr("Custom color")
            checked: lighting.customColor
            onClicked: lighting.setCustomColor(checked)
        }
        // the custom mode always uses colors, so it has no switch
        Label {
            x: 10
            y: 140
            visible: lighting.colorVisible && !lighting.customColorVisible
            text: qsTr("Custom color")
            color: palette.text
        }
        Button {
            id: colorButton
            x: 10
            y: 170
            width: 60
            height: 25
            visible: lighting.colorVisible
            // always clickable: picking a color switches custom color on
            background: Rectangle {
                color: lighting.color
                border.color: palette.mid
                opacity: lighting.colorEnabled ? 1 : 0.5
            }
            onClicked: {
                colorDialog.selectedColor = lighting.color
                colorDialog.open()
            }
        }

        // key selection of the custom mode
        Item {
            x: 420
            y: 0
            width: 391
            height: 211
            visible: lighting.customMode

            component KeyCheckBox: CheckBox {
                // key values checked with the checkbox
                property var keyValues: []

                font.family: "Open Sans"
                font.pointSize: 12
                onClicked: {
                    for (const keyValue of keyValues)
                        keyboardModel.setKeyCheck(keyValue, checked)
                }
            }

            KeyCheckBox {
                x: 20
                y: 20
                text: qsTr("Select all")
                onClicked: {
                    keyboardModel.setAllKeyCheck(checked)
                    for (const box of [topLed, leftLed, rightLed, bottomLed, wasdKeys, arrowKeys, numberKeys, functionKeys])
                        box.checked = checked
                }
            }
            KeyCheckBox {
                id: topLed
                x: 20
                y: 50
                text: qsTr("Top side LED")
                keyValues: [601]
            }
            KeyCheckBox {
                id: rightLed
                x: 20
                y: 80
                text: qsTr("Right side LED")
                keyValues: [602]
            }
            KeyCheckBox {
                id: wasdKeys
                x: 20
                y: 110
                text: "WASD"
                keyValues: [0x1A, 0x04, 0x16, 0x07]
            }
            KeyCheckBox {
                id: numberKeys
                x: 20
                y: 140
                text: qsTr("Number row")
                keyValues: [0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27]
            }
            KeyCheckBox {
                id: leftLed
                x: 190
                y: 50
                text: qsTr("Left side LED")
                keyValues: [600]
            }
            KeyCheckBox {
                id: bottomLed
                x: 190
                y: 80
                text: qsTr("Bottom side LED")
                keyValues: [603]
            }
            KeyCheckBox {
                id: arrowKeys
                x: 190
                y: 110
                text: qsTr("Arrows")
                keyValues: [0x50, 0x4F, 0x52, 0x51]
            }
            KeyCheckBox {
                id: functionKeys
                x: 190
                y: 140
                text: qsTr("Functions")
                keyValues: [0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45]
            }

            Button {
                x: 20
                y: 170
                // grows for longer translations
                width: Math.max(120, implicitWidth)
                height: 25
                text: qsTr("Reset all LEDs")
                font.family: "Open Sans"
                font.pointSize: 12
                onClicked: lighting.resetAllLeds()
            }
        }
    }

    ColorDialog {
        id: colorDialog
        title: qsTr("Color Select")
        onAccepted: lighting.setColor(selectedColor)
    }
}
