pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import DrevoPowerConsole

ApplicationWindow {
    id: window

    width: 1180
    height: 690
    minimumWidth: width
    maximumWidth: width
    minimumHeight: height
    maximumHeight: height
    visible: true
    title: DeviceManager.deviceName

    readonly property var tabs: [
        "icon_menu_customkey",
        "icon_menu_knob",
        "icon_menu_radi",
        "icon_menu_set",
    ]
    property int currentTab: 0

    // Exit from the tray menu: do not hide to the tray
    property bool quitting: false

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

    // top of the tabs, sidebar and pages, below the profile bar
    readonly property int contentTop: 40

    ProfileBar {
        x: 5
        y: 6
        dialogParent: Overlay.overlay
    }

    Repeater {
        model: window.tabs

        delegate: SpriteButton {
            required property string modelData
            required property int index

            x: 1
            y: window.contentTop + 44 * index
            width: 44
            height: 44
            source: "qrc:/image/button/" + modelData + ".png"
            themedIcon: modelData
            frameWidth: 44
            frameHeight: 44
            checked: window.currentTab === index
            onClicked: window.currentTab = index
        }
    }


    // sidebar panel behind the page captions and lists
    Rectangle {
        x: 45
        y: window.contentTop
        width: 260
        height: 648
        color: palette.base

        // soft right edge
        Rectangle {
            x: parent.width
            width: 4
            height: parent.height
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0; color: Qt.alpha(window.palette.shadow, 0.12) }
                GradientStop { position: 1; color: "transparent" }
            }
        }
    }

    // all pages are created at startup: they load their settings and send them to the keyboard
    StackLayout {
        x: 45
        y: window.contentTop
        width: 1135
        height: 650
        currentIndex: window.currentTab

        CustomKeyPage {}
        CustomKnobPage {}
        LightingPage {}
        SettingsPage {}
    }

    MessageDialog {
        id: databaseErrorDialog
        title: qsTr("Settings cannot be saved")
        text: DeviceManager.databaseError
        buttons: MessageDialog.Ok
    }

    Component.onCompleted: {
        if (DeviceManager.databaseError !== "")
            databaseErrorDialog.open()
    }
}
