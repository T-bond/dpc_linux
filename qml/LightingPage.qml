pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import DrevoPowerConsole

// backlight effects
Control {
    id: root

    padding: Theme.pageMargin
    leftPadding: Math.max(Theme.pageMargin, (width - Theme.maximumContentWidth) / 2)
    rightPadding: leftPadding
    font.family: Theme.fontFamily

    KeyboardModel {
        id: keyboardModel
    }

    Lighting {
        id: lighting
        keyboard: keyboardModel
    }

    // setting name and its control
    component SettingRow: RowLayout {
        property alias text: settingLabel.text
        // value shown on the right of the name
        property alias valueText: valueLabel.text

        Layout.fillWidth: true
        spacing: 8

        Label {
            id: settingLabel
            Layout.fillWidth: true
            Layout.minimumWidth: implicitWidth > 0 ? Math.min(implicitWidth, 110) : 0
            wrapMode: Text.WordWrap
            font.pointSize: 10
            color: Theme.text
        }
        Label {
            id: valueLabel
            font.pointSize: 9.5
            color: Theme.textMuted
        }
    }

    // group of keys or side LEDs checked together in the custom mode
    component KeyGroupButton: Chip {
        // key values checked with the button
        property var keyValues: []

        checkable: true
        Layout.fillWidth: true
        onClicked: {
            for (const keyValue of keyValues)
                keyboardModel.setKeyCheck(keyValue, checked)
        }
    }

    contentItem: ColumnLayout {
        spacing: Theme.spacing

        PageHeader {
            Layout.fillWidth: true
            title: qsTr("Lighting")
            subtitle: qsTr("Radi-RGB backlight: choose an effect and tune its brightness, speed and color.")
        }

        Card {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: Theme.minimumStageHeight
            Layout.maximumHeight: Theme.maximumStageHeight
            fillContent: true
            title: qsTr("Preview")
            subtitle: lighting.customMode ? qsTr("Click keys and side LEDs to select them, Shift or Ctrl + click adds to the selection.")
                                          : ""

            KeyboardStage {
                Layout.fillWidth: true
                Layout.fillHeight: true
                keyboard: keyboardModel
                mode: lighting.customMode ? KeyboardModel.LightCustom : KeyboardModel.LightStatic
                lightColor: lighting.lightColor
            }
        }

        RowLayout {
            Layout.fillWidth: true
            // never shorter than its content, grows once the preview card is at its maximum
            Layout.fillHeight: true
            Layout.minimumHeight: 290
            Layout.preferredHeight: 290
            spacing: Theme.spacing

            Card {
                // fillWidth: only filling items shrink to their minimum when the window is narrow
                Layout.fillWidth: true
                Layout.preferredWidth: lighting.customMode ? 330 : 380
                Layout.minimumWidth: 300
                Layout.maximumWidth: 440
                Layout.fillHeight: true
                fillContent: true
                title: qsTr("Effect")

                GridView {
                    id: effectGrid
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds
                    cellWidth: width / 2
                    cellHeight: 44
                    model: lighting.modes
                    currentIndex: lighting.modeIndex
                    ScrollBar.vertical: ScrollBar {}

                    delegate: ItemDelegate {
                        id: modeItem

                        required property var modelData
                        required property int index

                        readonly property bool current: GridView.isCurrentItem

                        width: effectGrid.cellWidth - 6
                        height: effectGrid.cellHeight - 6
                        padding: 0
                        hoverEnabled: true
                        onClicked: lighting.selectModeIndex(index)

                        ToolTip.visible: hovered && modeLabel.truncated
                        ToolTip.text: modelData.text

                        background: Rectangle {
                            radius: Theme.smallRadius
                            color: modeItem.current ? Theme.accentSoft : modeItem.hovered ? Theme.surfaceAlt : "transparent"
                            border.color: modeItem.current ? Theme.accent : "transparent"
                        }

                        contentItem: RowLayout {
                            spacing: 10

                            TintedIcon {
                                Layout.leftMargin: 10
                                Layout.preferredWidth: 20
                                Layout.preferredHeight: 20
                                icon: modeItem.modelData.icon
                                color: modeItem.current ? Theme.accentText : Theme.textMuted
                            }
                            Label {
                                id: modeLabel
                                Layout.fillWidth: true
                                text: modeItem.modelData.text
                                elide: Text.ElideRight
                                font.pointSize: 10
                                font.weight: modeItem.current ? Font.DemiBold : Font.Normal
                                color: Theme.text
                            }
                        }
                    }
                }
            }

            Card {
                Layout.fillWidth: true
                Layout.preferredWidth: 420
                Layout.minimumWidth: 250
                Layout.fillHeight: true
                title: qsTr("Settings")

                SettingRow {
                    text: qsTr("Brightness")
                    valueText: Math.round(brightnessSlider.value / 15 * 100) + " %"
                }
                Slider {
                    id: brightnessSlider
                    Layout.fillWidth: true
                    from: 0
                    to: 15
                    stepSize: 1
                    snapMode: Slider.SnapAlways
                    value: lighting.brightness
                    onMoved: lighting.setBrightness(value)
                }

                SettingRow {
                    text: qsTr("Speed")
                    valueText: lighting.speedEnabled ? (speedSlider.value + 1) + " / 5" : qsTr("not used by this effect")
                    opacity: lighting.speedEnabled ? 1 : 0.6
                }
                Slider {
                    id: speedSlider
                    Layout.fillWidth: true
                    from: 0
                    to: 4
                    stepSize: 1
                    snapMode: Slider.SnapAlways
                    enabled: lighting.speedEnabled
                    value: lighting.speed
                    onMoved: lighting.setSpeed(value)
                }

                // rainbow direction, in place of the color settings (the rainbow has no color)
                SettingRow {
                    visible: lighting.directionVisible
                    text: qsTr("Direction")

                    ComboBox {
                        id: directionCombo
                        Layout.preferredWidth: 200
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
                }

                // effect color, or the color of the checked keys in the custom mode
                SettingRow {
                    visible: lighting.colorVisible
                    text: lighting.customMode ? qsTr("Color of the selected keys") : qsTr("Custom color")

                    // the custom mode always uses colors, so it has no switch
                    Switch {
                        visible: lighting.customColorVisible
                        checked: lighting.customColor
                        onToggled: lighting.setCustomColor(checked)
                    }

                    // always clickable: picking a color switches custom color on
                    AbstractButton {
                        id: colorButton
                        implicitWidth: 64
                        implicitHeight: 32
                        hoverEnabled: true
                        onClicked: {
                            colorDialog.selectedColor = lighting.color
                            colorDialog.open()
                        }

                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Choose a color")

                        background: Rectangle {
                            radius: Theme.smallRadius
                            color: lighting.color
                            opacity: lighting.colorEnabled ? 1 : 0.45
                            border.width: colorButton.hovered ? 2 : 1
                            border.color: colorButton.hovered ? Theme.text : Theme.border
                        }
                    }
                }

                Label {
                    visible: lighting.colorVisible && lighting.customColorVisible && !lighting.customColor
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    text: qsTr("Off: the effect uses the keyboard's own colors.")
                    font.pointSize: 9
                    color: Theme.textMuted
                }
            }

            // key selection of the custom mode
            Card {
                visible: lighting.customMode
                Layout.fillWidth: true
                Layout.preferredWidth: 300
                Layout.minimumWidth: 290
                Layout.maximumWidth: 380
                Layout.fillHeight: true
                title: qsTr("Selection")

                actions: Button {
                    text: qsTr("Reset all LEDs")
                    flat: true
                    font.pointSize: 9.5
                    onClicked: lighting.resetAllLeds()
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: 2
                    rowSpacing: 6
                    columnSpacing: 6

                    KeyGroupButton {
                        id: allKeys
                        Layout.columnSpan: 2
                        text: qsTr("Select all")
                        onClicked: {
                            keyboardModel.setAllKeyCheck(checked)
                            for (const button of [topLed, leftLed, rightLed, bottomLed, wasdKeys, arrowKeys, numberKeys, functionKeys])
                                button.checked = checked
                        }
                    }
                    KeyGroupButton {
                        id: topLed
                        text: qsTr("Top side LED")
                        keyValues: keyboardModel.sideLedValues(601)
                    }
                    KeyGroupButton {
                        id: bottomLed
                        text: qsTr("Bottom side LED")
                        keyValues: keyboardModel.sideLedValues(603)
                    }
                    KeyGroupButton {
                        id: leftLed
                        text: qsTr("Left side LED")
                        keyValues: keyboardModel.sideLedValues(600)
                    }
                    KeyGroupButton {
                        id: rightLed
                        text: qsTr("Right side LED")
                        keyValues: keyboardModel.sideLedValues(602)
                    }
                    KeyGroupButton {
                        id: wasdKeys
                        text: "WASD"
                        keyValues: [0x1A, 0x04, 0x16, 0x07]
                    }
                    KeyGroupButton {
                        id: arrowKeys
                        text: qsTr("Arrows")
                        keyValues: [0x50, 0x4F, 0x52, 0x51]
                    }
                    KeyGroupButton {
                        id: numberKeys
                        text: qsTr("Number row")
                        keyValues: [0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27]
                    }
                    KeyGroupButton {
                        id: functionKeys
                        text: qsTr("Functions")
                        keyValues: [0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45]
                    }
                }
            }
        }
    }

    ColorDialog {
        id: colorDialog
        title: qsTr("Color Select")
        onAccepted: lighting.setColor(selectedColor)
    }
}
