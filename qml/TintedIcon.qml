import QtQuick

// resource icon drawn in one color, see IconProvider
Image {
    // resource path, e.g. "image/icon/icon_light_static.png" or a "qrc:/..." url
    property string icon
    property color color: Theme.text

    source: icon === "" ? "" : "image://icon/" + color.toString().slice(1, 7) + "/" + icon.replace(/^qrc:\//, "")
    fillMode: Image.PreserveAspectFit
    smooth: true
}
