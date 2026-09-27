pragma Singleton

import QtCore
import QtQuick

// colors and sizes of the interface
QtObject {
    id: theme

    enum Appearance { System, Light, Dark }

    // Theme.System, Theme.Light or Theme.Dark, stored in the settings file
    property alias appearance: store.appearance
    // follows the desktop with System; dark if the desktop does not tell
    readonly property bool dark: appearance === Theme.System ? Qt.styleHints.colorScheme !== Qt.Light
                                                             : appearance === Theme.Dark

    property Settings settings: Settings {
        id: store
        category: "Appearance"
        property int appearance: Theme.System
    }

    readonly property color background: dark ? "#111215" : "#f1f2f5"
    readonly property color rail: dark ? "#17181c" : "#ffffff"
    readonly property color surface: dark ? "#1c1d22" : "#ffffff"
    // hovered rows, inputs and wells inside cards
    readonly property color surfaceAlt: dark ? "#25272d" : "#f3f4f7"
    readonly property color border: dark ? "#2b2d34" : "#e1e3e8"
    readonly property color text: dark ? "#ecedf0" : "#1a1b1f"
    readonly property color textMuted: dark ? "#9b9ea6" : "#656872"

    // DREVO yellow: fills, and a darker variant for text and icons on light backgrounds
    readonly property color accent: dark ? "#ffd60a" : "#f2c200"
    readonly property color accentText: dark ? "#ffd60a" : "#8a6c00"
    readonly property color accentForeground: "#17181c"
    readonly property color accentSoft: Qt.alpha(accent, dark ? 0.14 : 0.2)

    readonly property color success: dark ? "#3ddc84" : "#1e9e57"
    readonly property color danger: dark ? "#ff6b6b" : "#d93636"

    readonly property int radius: 12
    readonly property int smallRadius: 8
    readonly property int spacing: 20
    readonly property int pageMargin: 28
    // pages are centered at most this wide on large windows
    readonly property int maximumContentWidth: 1500
    // keyboard preview: its card never gets shorter or taller than this
    readonly property int minimumStageHeight: 240
    readonly property int maximumStageHeight: 620

    readonly property string fontFamily: "Open Sans"
}
