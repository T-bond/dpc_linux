pragma ComponentBehavior: Bound

import QtQuick

// button drawn from a sprite with 4 frames side by side: normal, hover, pressed, checked
Item {
    id: root

    property url source
    property int frameWidth
    property int frameHeight
    property string text
    property bool checked: false
    // sprite name for image://tabicon: draws the checked state on the theme's panel color
    property string themedIcon

    readonly property bool themedCheck: checked && themedIcon !== ""

    signal clicked()

    readonly property int frame: checked ? 3 : mouseArea.pressed ? 2 : mouseArea.containsMouse ? 1 : 0

    Rectangle {
        visible: root.themedCheck
        anchors.fill: parent
        color: palette.base

        Image {
            anchors.centerIn: parent
            source: root.themedCheck ? "image://tabicon/" + root.themedIcon + "/" + palette.text : ""
        }
    }

    Image {
        visible: !root.themedCheck
        x: Math.round((root.width - root.frameWidth) / 2)
        y: Math.round((root.height - root.frameHeight) / 2)
        source: root.source
        sourceClipRect: Qt.rect(root.frame * root.frameWidth, 0, root.frameWidth, root.frameHeight)
    }

    Text {
        visible: root.text !== ""
        anchors.horizontalCenter: parent.horizontalCenter
        y: 32 - baselineOffset
        text: root.text
        font.pointSize: 10
        color: Qt.rgba(51 / 255, 51 / 255, 51 / 255, 1)
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        acceptedButtons: Qt.LeftButton
        onClicked: root.clicked()
    }
}
