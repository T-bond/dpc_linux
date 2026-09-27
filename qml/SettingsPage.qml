pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import DrevoPowerConsole

// report rate, sleep times, layout and application settings
Control {
    id: root

    padding: 0
    font.family: Theme.fontFamily

    KeyboardSettings {
        id: settings
    }

    KeyboardModel {
        id: keyboardModel
    }

    // name and description on the left, control on the right
    component OptionRow: RowLayout {
        property alias text: optionLabel.text
        property alias description: descriptionLabel.text

        Layout.fillWidth: true
        spacing: 16

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 0
            Label {
                id: optionLabel
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                font.pointSize: 10
                color: Theme.text
            }
            Label {
                id: descriptionLabel
                visible: text !== ""
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                font.pointSize: 9
                color: Theme.textMuted
            }
        }
    }

    // sleep time in s, 0 = never; the switch turns on the default time
    component SleepTime: OptionRow {
        id: sleepTime

        property int value
        property int defaultValue
        signal edited(int value)

        SpinBox {
            enabled: sleepTime.value !== 0
            opacity: enabled ? 1 : 0.5
            from: 1
            to: 9999
            editable: true
            value: sleepTime.value !== 0 ? sleepTime.value : sleepTime.defaultValue
            textFromValue: (value, locale) => value + " s"
            valueFromText: (text, locale) => parseInt(text) || sleepTime.defaultValue
            onValueModified: sleepTime.edited(value)
        }
        Switch {
            checked: sleepTime.value !== 0
            onToggled: sleepTime.edited(checked ? sleepTime.defaultValue : 0)
        }
    }

    ScrollView {
        id: scrollView
        anchors.fill: parent
        contentWidth: availableWidth

        ColumnLayout {
            width: Math.min(scrollView.availableWidth - 2 * Theme.pageMargin, Theme.maximumContentWidth)
            x: (scrollView.availableWidth - width) / 2
            spacing: Theme.spacing

            Item {
                Layout.preferredHeight: Theme.pageMargin - Theme.spacing
            }

            PageHeader {
                Layout.fillWidth: true
                title: qsTr("Settings")
                subtitle: qsTr("Keyboard options of the current profile, and how the program behaves.")
            }

            GridLayout {
                Layout.fillWidth: true
                columns: 2
                columnSpacing: Theme.spacing
                rowSpacing: Theme.spacing

                Card {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    Layout.fillHeight: true
                    // settings of the current profile
                    enabled: DeviceManager.currentProfile > 0
                    title: qsTr("Report Rate")
                    subtitle: qsTr("How often the keyboard reports key presses to the computer.")

                    // report interval in ms
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        Repeater {
                            model: [8, 4, 2, 1]

                            delegate: Chip {
                                required property int modelData

                                Layout.fillWidth: true
                                Layout.preferredWidth: 1
                                text: (1000 / modelData) + " Hz"
                                checked: settings.reportRate === modelData
                                onClicked: settings.reportRate = modelData

                                ToolTip.visible: hovered
                                ToolTip.text: qsTr("%1 ms").arg(modelData)
                            }
                        }
                    }
                }

                Card {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    Layout.fillHeight: true
                    // settings of the current profile
                    enabled: DeviceManager.currentProfile > 0
                    title: qsTr("Set sleep mode")
                    subtitle: qsTr("Time without typing before the keyboard goes to sleep.")

                    SleepTime {
                        text: qsTr("USB connection mode")
                        value: settings.usbSleep
                        defaultValue: 300
                        onEdited: (value) => settings.usbSleep = value
                    }
                    SleepTime {
                        text: qsTr("Backlighting")
                        value: settings.backlightSleep
                        defaultValue: 120
                        onEdited: (value) => settings.backlightSleep = value
                    }
                    SleepTime {
                        text: "2.4G/Bluetooth"
                        value: settings.wirelessSleep
                        defaultValue: 180
                        onEdited: (value) => settings.wirelessSleep = value
                    }
                }

                Card {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    Layout.fillHeight: true
                    title: qsTr("Keyboard layout")
                    subtitle: regionCombo.visible ? qsTr("The keyboard does not report its regional variant, choose it here.")
                                                  : ""

                    // regional variant of the layout: the keyboard does not report it
                    ComboBox {
                        id: regionCombo
                        visible: count > 0
                        Layout.fillWidth: true
                        model: DeviceManager.keyboardRegions
                        textRole: "text"
                        valueRole: "value"
                        onActivated: DeviceManager.keyboardRegion = currentValue
                        Component.onCompleted: currentIndex = Math.max(0, indexOfValue(DeviceManager.keyboardRegion))
                    }

                    KeyboardStage {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 180
                        keyboard: keyboardModel
                        maximumScale: 1
                    }
                }

                Card {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    Layout.fillHeight: true
                    title: qsTr("Application")

                    OptionRow {
                        text: qsTr("Appearance")

                        ComboBox {
                            Layout.preferredWidth: 160
                            model: [qsTr("System"), qsTr("Light"), qsTr("Dark")]
                            currentIndex: Theme.appearance
                            onActivated: (index) => Theme.appearance = index
                        }
                    }

                    OptionRow {
                        text: qsTr("Close to the system tray")
                        description: SystemTray.available ? qsTr("Closing the window keeps the program running in the tray.")
                                                          : qsTr("The desktop does not show tray icons.")

                        Switch {
                            enabled: SystemTray.available
                            checked: DeviceManager.closeToTray
                            onToggled: DeviceManager.closeToTray = checked
                        }
                    }

                    Item {
                        Layout.fillHeight: true
                    }
                }
            }

            Card {
                Layout.fillWidth: true
                title: qsTr("Restore Keyboard")
                subtitle: qsTr("Reset the keyboard to its factory settings.")

                actions: Button {
                    text: qsTr("Restore…")
                    Material.foreground: Theme.danger
                    onClicked: restoreDialog.open()
                }
            }

            Item {
                Layout.preferredHeight: Theme.pageMargin - Theme.spacing
            }
        }
    }

    MessageBox {
        id: restoreDialog
        title: qsTr("Restore Keyboard")
        text: qsTr("Reset the keyboard to its factory settings?")
        destructive: true
        onAccepted: settings.resetKeyboard()
    }
}
