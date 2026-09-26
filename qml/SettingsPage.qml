pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import DrevoPowerConsole

// report rate and sleep times
Control {
    id: root

    width: 1135
    height: 620
    padding: 0
    font.family: "Open Sans"
    font.pointSize: 12

    KeyboardSettings {
        id: settings
    }

    component Heading: Label {
        font.pointSize: 14
    }

    // report interval in ms
    component RateButton: RadioButton {
        property int rate
        height: 23
        padding: 0
        checked: settings.reportRate === rate
        onClicked: settings.reportRate = rate
    }

    // sleep time in s, 0 = never; the checkbox switches to the default time
    component SleepTime: Item {
        id: sleepTime

        property int value
        property int defaultValue
        signal edited(int value)

        width: 200
        height: 23

        CheckBox {
            height: 23
            padding: 0
            spacing: 4
            text: qsTr("set a time(s)")
            checked: sleepTime.value !== 0
            onClicked: sleepTime.edited(checked ? sleepTime.defaultValue : 0)
        }
        SpinBox {
            x: 115
            width: 85
            height: 23
            from: 0
            to: 9999
            editable: true
            value: sleepTime.value
            onValueModified: sleepTime.edited(value)
        }
    }

    Label {
        x: 5
        y: 5
        text: qsTr("Settings")
        font.pointSize: 16
        font.bold: true
    }

    Heading {
        x: 5
        y: 45
        text: qsTr("Report Rate")
    }
    RateButton {
        x: 5
        y: 75
        text: "125Hz/8ms"
        rate: 8
    }
    RateButton {
        x: 130
        y: 75
        text: "250Hz/4ms"
        rate: 4
    }
    RateButton {
        x: 5
        y: 110
        text: "500Hz/2ms"
        rate: 2
    }
    RateButton {
        x: 130
        y: 110
        text: "1000Hz/1ms"
        rate: 1
    }

    Heading {
        x: 5
        y: 160
        text: qsTr("Set sleep mode")
    }

    Label {
        x: 5
        y: 190
        text: qsTr("USB connection mode")
    }
    SleepTime {
        x: 5
        y: 220
        value: settings.usbSleep
        defaultValue: 300
        onEdited: (value) => settings.usbSleep = value
    }

    Heading {
        x: 5
        y: 255
        text: qsTr("Backlighting")
    }
    SleepTime {
        x: 5
        y: 285
        value: settings.backlightSleep
        defaultValue: 120
        onEdited: (value) => settings.backlightSleep = value
    }

    Heading {
        x: 5
        y: 320
        text: "2.4G/Bluetooth"
    }
    SleepTime {
        x: 5
        y: 350
        value: settings.wirelessSleep
        defaultValue: 180
        onEdited: (value) => settings.wirelessSleep = value
    }

    Button {
        x: 5
        y: 400
        // grows for longer translations
        width: Math.max(120, implicitWidth)
        height: 25
        text: qsTr("Restore Keyboard")
        font.pointSize: 10
        onClicked: settings.resetKeyboard()
    }

    // regional variant of the layout: the keyboard does not report it
    Heading {
        x: 5
        y: 445
        visible: regionCombo.visible
        text: qsTr("Keyboard layout")
    }
    ComboBox {
        id: regionCombo
        x: 5
        y: 475
        width: 200
        height: 25
        visible: count > 0
        model: DeviceManager.keyboardRegions
        textRole: "text"
        valueRole: "value"
        onActivated: DeviceManager.keyboardRegion = currentValue
        Component.onCompleted: currentIndex = Math.max(0, indexOfValue(DeviceManager.keyboardRegion))
    }

    Heading {
        id: windowHeading
        x: 5
        y: regionCombo.visible ? 520 : 445
        text: qsTr("Window")
    }
    CheckBox {
        id: trayCheckBox
        x: 5
        y: windowHeading.y + 30
        height: 23
        padding: 0
        spacing: 4
        enabled: SystemTray.available
        text: qsTr("Close to the system tray")
        checked: DeviceManager.closeToTray
        onClicked: DeviceManager.closeToTray = checked
    }
    Label {
        x: 5
        y: trayCheckBox.y + 26
        width: 250
        visible: !SystemTray.available
        wrapMode: Text.WordWrap
        font.pointSize: 10
        opacity: 0.7
        text: qsTr("The desktop does not show tray icons.")
    }

    KeyboardModel {
        id: keyboardModel
    }

    KeyboardView {
        x: 340
        y: 160
        keyboard: keyboardModel
    }
}
