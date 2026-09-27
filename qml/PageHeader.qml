import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// title and short description at the top of a page
ColumnLayout {
    property string title
    property string subtitle

    spacing: 2

    Label {
        text: parent.title
        font.pointSize: 20
        font.weight: Font.Bold
        color: Theme.text
    }
    Label {
        Layout.fillWidth: true
        text: parent.subtitle
        wrapMode: Text.WordWrap
        font.pointSize: 10
        color: Theme.textMuted
    }
}
