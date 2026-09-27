pragma ComponentBehavior: Bound

import QtQuick

// keyboard view scaled to fit the available space, centered
Item {
    id: root

    property alias keyboard: view.keyboard
    property alias mode: view.mode
    property alias lightColor: view.lightColor
    property alias selectedIndex: view.selectedIndex
    // larger than the image only this much, it is a bitmap
    property real maximumScale: 1.6

    signal keyPressed(int index)

    implicitWidth: view.width + 2 * view.barMargin
    implicitHeight: view.height + 2 * view.barMargin

    KeyboardView {
        id: view
        anchors.centerIn: parent
        scale: Math.min(root.maximumScale,
                        root.width / (width + 2 * barMargin),
                        root.height / (height + 2 * barMargin))
        onKeyPressed: (index) => root.keyPressed(index)
    }
}
