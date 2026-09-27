import QtQuick
import QtQuick.Shapes

// highlight of a key in keyboard image coordinates: a rectangle, or the L shape of the ISO / JIS Enter
Item {
    id: root

    property rect keyRect
    // lower part of an L-shaped key, below the right end of keyRect; empty for a rectangle
    property rect lowerRect
    // distance of the highlight from the key edges, negative values are outside
    property real leftInset: 0
    property real topInset: 0
    property real rightInset: 0
    property real bottomInset: 0

    property color color: "transparent"
    property color borderColor: "transparent"
    property real borderWidth: 0

    readonly property bool lShaped: lowerRect.width > 0 && lowerRect.height > 0

    x: keyRect.x + leftInset
    y: keyRect.y + topInset
    width: keyRect.width - leftInset - rightInset
    height: (lShaped ? lowerRect.y + lowerRect.height : keyRect.y + keyRect.height) - bottomInset - y

    Rectangle {
        visible: !root.lShaped
        anchors.fill: parent
        color: root.color
        border.color: root.borderColor
        border.width: root.borderWidth
    }

    Shape {
        id: shape

        // like the Rectangle border, the stroke stays inside the outline
        readonly property real b: root.borderWidth / 2
        // left edge of the lower part, and the bottom edge of the upper part left of it
        readonly property real stemX: root.lowerRect.x + root.leftInset - root.x
        readonly property real stepY: root.keyRect.y + root.keyRect.height - root.bottomInset - root.y

        visible: root.lShaped
        anchors.fill: parent

        ShapePath {
            fillColor: root.color
            strokeColor: root.borderWidth > 0 ? root.borderColor : "transparent"
            strokeWidth: root.borderWidth
            joinStyle: ShapePath.MiterJoin

            startX: shape.b
            startY: shape.b
            PathPolyline {
                path: [
                    Qt.point(shape.b, shape.b),
                    Qt.point(shape.width - shape.b, shape.b),
                    Qt.point(shape.width - shape.b, shape.height - shape.b),
                    Qt.point(shape.stemX + shape.b, shape.height - shape.b),
                    Qt.point(shape.stemX + shape.b, shape.stepY - shape.b),
                    Qt.point(shape.b, shape.stepY - shape.b),
                    Qt.point(shape.b, shape.b),
                ]
            }
        }
    }
}
