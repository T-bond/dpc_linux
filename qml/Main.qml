pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import DrevoPowerConsole

ApplicationWindow {
    id: window

    width: 1280
    height: 860
    // the pages are laid out to fit in this; see the minimum sizes of their panels
    minimumWidth: 1160
    minimumHeight: 720
    visible: true
    title: "DREVO Power Console – " + DeviceManager.deviceName
    color: Theme.background

    Material.theme: Theme.dark ? Material.Dark : Material.Light
    Material.accent: Theme.accent
    Material.primary: Theme.accent
    Material.foreground: Theme.text
    font.family: Theme.fontFamily

    // pages of the navigation rail: [{ text, icon }]
    readonly property var tabs: [
        { text: qsTr("Keys"), icon: "icon_menu_customkey" },
        { text: qsTr("Knob"), icon: "icon_menu_knob" },
        { text: qsTr("Lighting"), icon: "icon_menu_radi" },
        { text: qsTr("Settings"), icon: "icon_menu_set" },
    ]
    property int currentTab: 0

    // Exit from the tray menu: do not hide to the tray
    property bool quitting: false

    // the shown hardware profile: { name, profile, known, storedName }
    readonly property var hardwareProfile: DeviceManager.hardwareProfiles[DeviceManager.hardwareProfile] ?? ({})
    // what the keyboard has in a hardware profile without a current profile
    readonly property string storedKeysText: !hardwareProfile.known ? qsTr("The application does not know which keys the keyboard has stored in it.")
                                           : hardwareProfile.storedName !== "" ? qsTr("The keyboard keeps the keys of “%1” in it.").arg(hardwareProfile.storedName)
                                           : qsTr("The keyboard has its default keys in it.")

    function showWindow() {
        if (window.visibility === Window.Minimized)
            window.showNormal()
        else
            window.show()
        window.raise()
        window.requestActivate()
    }

    onClosing: (close) => {
        if (DeviceManager.closeToTray && SystemTray.available && !window.quitting) {
            close.accepted = false
            window.hide()
        } else {
            Qt.quit()
        }
    }

    // tray icon (see SystemTray.cpp)
    Connections {
        target: SystemTray
        function onShowWindowRequested() {
            window.showWindow()
        }
        function onQuitRequested() {
            window.quitting = true
            Qt.quit()
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // navigation rail: device, pages, profile and lights
        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: 220
            color: Theme.rail

            Rectangle {
                anchors.right: parent.right
                width: 1
                height: parent.height
                color: Theme.border
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 6

                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 4
                    spacing: 10

                    Image {
                        Layout.preferredWidth: 38
                        Layout.preferredHeight: 38
                        source: "qrc:/image/drevo-power-console.png"
                        sourceSize: Qt.size(76, 76)
                        smooth: true
                    }
                    ColumnLayout {
                        spacing: -2
                        Label {
                            text: "DREVO"
                            font.pointSize: 13
                            font.weight: Font.ExtraBold
                            font.letterSpacing: 2
                            color: Theme.text
                        }
                        Label {
                            text: "Power Console"
                            font.pointSize: 9
                            color: Theme.textMuted
                        }
                    }
                }

                // connection state
                Rectangle {
                    Layout.fillWidth: true
                    Layout.topMargin: 14
                    Layout.bottomMargin: 14
                    implicitHeight: deviceColumn.implicitHeight + 16
                    radius: Theme.smallRadius
                    color: Theme.surfaceAlt

                    ColumnLayout {
                        id: deviceColumn
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12
                        spacing: 4

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10

                            Rectangle {
                                Layout.alignment: Qt.AlignVCenter
                                Layout.minimumWidth: implicitWidth
                                implicitWidth: 8
                                implicitHeight: 8
                                radius: 4
                                color: DeviceManager.connected ? Theme.success
                                     : DeviceManager.receiverOnly ? Theme.accentText
                                     : Theme.danger
                            }
                            // takes the width the dot and the button leave; a long name wraps
                            ColumnLayout {
                                Layout.fillWidth: true
                                Layout.preferredWidth: 0
                                spacing: 0
                                Label {
                                    Layout.fillWidth: true
                                    text: DeviceManager.deviceName
                                    wrapMode: Text.WordWrap
                                    font.pointSize: 9.5
                                    font.weight: Font.DemiBold
                                    color: Theme.text
                                }
                                Label {
                                    Layout.fillWidth: true
                                    wrapMode: Text.WordWrap
                                    text: DeviceManager.connected ? qsTr("Connected")
                                        : DeviceManager.receiverOnly ? qsTr("Connected through 2.4G")
                                        : qsTr("Not connected")
                                    font.pointSize: 8.5
                                    color: Theme.textMuted
                                }
                            }
                            ToolButton {
                                Layout.alignment: Qt.AlignVCenter
                                Layout.minimumWidth: implicitWidth
                                Layout.rightMargin: -6
                                implicitWidth: 32
                                implicitHeight: 32
                                padding: 0
                                text: "↻"
                                font.pointSize: 12
                                onClicked: DeviceManager.refreshConnection()

                                ToolTip.visible: hovered
                                ToolTip.text: qsTr("Check the connection again")
                            }
                        }

                        // programming needs the USB cable
                        Label {
                            Layout.fillWidth: true
                            visible: DeviceManager.receiverOnly
                            text: qsTr("Only USB programming is supported. Connect the keyboard with its USB cable to change its settings.")
                            wrapMode: Text.WordWrap
                            font.pointSize: 8.5
                            color: Theme.accentText
                        }
                    }
                }

                Repeater {
                    model: window.tabs

                    delegate: NavButton {
                        required property var modelData
                        required property int index

                        Layout.fillWidth: true
                        text: modelData.text
                        iconName: modelData.icon
                        checked: window.currentTab === index
                        onClicked: window.currentTab = index
                    }
                }

                Item {
                    Layout.fillHeight: true
                }

                component SectionLabel: Label {
                    Layout.leftMargin: 4
                    font.pointSize: 8
                    font.weight: Font.DemiBold
                    font.letterSpacing: 1.2
                    color: Theme.textMuted
                }

                // hardware profile the application shows and writes to
                SectionLabel {
                    text: qsTr("HARDWARE PROFILE")
                }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    Repeater {
                        model: DeviceManager.hardwareProfiles

                        delegate: Chip {
                            required property var modelData
                            required property int index

                            Layout.fillWidth: true
                            Layout.preferredWidth: 1
                            text: modelData.name
                            checked: DeviceManager.hardwareProfile === index
                            onClicked: DeviceManager.hardwareProfile = index
                        }
                    }
                }
                Label {
                    Layout.fillWidth: true
                    Layout.leftMargin: 4
                    Layout.bottomMargin: 8
                    wrapMode: Text.WordWrap
                    text: qsTr("Use it on the keyboard with Fn+Ctrl+F%1.").arg(DeviceManager.hardwareProfile + 1)
                    font.pointSize: 8.5
                    color: Theme.textMuted
                }

                SectionLabel {
                    text: qsTr("PROFILE")
                }
                ProfileBar {
                    id: profileBar
                    Layout.fillWidth: true
                    dialogParent: Overlay.overlay
                }

                // lights off without changing the profile, like the tray menu
                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 4
                    Layout.topMargin: 6

                    Label {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        text: qsTr("Keyboard lights")
                        font.pointSize: 10
                        color: Theme.text
                    }
                    Switch {
                        checked: !DeviceManager.lightsOff
                        onToggled: DeviceManager.lightsOff = !checked
                    }
                }
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            // all pages are created at startup: they load their settings and send them to the keyboard
            StackLayout {
                anchors.fill: parent
                currentIndex: window.currentTab

                CustomKeyPage {}
                CustomKnobPage {}
                LightingPage {}
                SettingsPage {}
            }

            // the hardware profile has no current profile (empty, or it was moved or deleted):
            // nothing to show on the pages of the profile (the settings page has the application's too)
            Rectangle {
                anchors.fill: parent
                visible: DeviceManager.currentProfile <= 0 && window.currentTab < 3
                color: Theme.background

                // keeps the pages below from getting the mouse
                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                }

                Card {
                    anchors.centerIn: parent
                    width: Math.min(parent.width - 2 * Theme.pageMargin, 480)
                    title: qsTr("No profile on %1").arg(window.hardwareProfile.name ?? "")

                    Label {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        text: window.storedKeysText + " "
                              + (DeviceManager.hardwareProfileProfiles.length > 0
                                 ? qsTr("Select one of its profiles in the sidebar to write it, or create a new one.")
                                 : qsTr("Create a profile for it."))
                        font.pointSize: 10
                        color: Theme.textMuted
                    }
                    Button {
                        Layout.alignment: Qt.AlignRight
                        text: qsTr("New profile…")
                        highlighted: true
                        onClicked: profileBar.newProfile()
                    }
                }
            }
        }
    }

    MessageBox {
        id: databaseErrorDialog
        title: qsTr("Settings cannot be saved")
        text: DeviceManager.databaseError
        standardButtons: Dialog.Ok
    }

    Component.onCompleted: {
        if (DeviceManager.databaseError !== "")
            databaseErrorDialog.open()
    }
}
