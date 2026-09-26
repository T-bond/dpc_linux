pragma ComponentBehavior: Bound

import QtQuick
import DrevoPowerConsole

// function assignment of the Genius-Knob actions
AssignmentPage {
    id: root

    caption: "Genius-Knob"
    knob: true

    readonly property var knobActions: [
        { keyValue: 500, text: qsTr("Single-Click") },
        { keyValue: 501, text: qsTr("Double-Click") },
        { keyValue: 502, text: qsTr("Forward") },
        { keyValue: 503, text: qsTr("Backward") },
    ]

    function setKnobCheck(keyValue, check) {
        for (let i = 0; i < knobButtons.count; i++) {
            if (knobActions[i].keyValue === keyValue)
                knobButtons.itemAt(i).checked = check
        }
    }

    onAssignmentChanged: (keyValue, assigned) => setKnobCheck(keyValue, assigned)

    Repeater {
        id: knobButtons
        model: root.knobActions

        delegate: SpriteButton {
            required property var modelData
            required property int index

            x: 310 + (1135 - 310 - 170 * 4) / 2 + 170 * index
            y: 160
            width: 166
            height: 50
            source: "qrc:/image/button/img_knobbutton.png"
            frameWidth: 166
            frameHeight: 66
            text: modelData.text

            onClicked: root.selectKey(modelData.keyValue, modelData.text)
        }
    }

    Image {
        x: 310 + (1135 - 310) / 2 - 317
        y: 160 + 60
        width: 634
        height: 216
        source: "qrc:/image/icon/icon_knob.png"
    }

    function loadAssignedKeys() {
        const assigned = root.assignedKeys()
        for (const action of knobActions)
            setKnobCheck(action.keyValue, assigned.includes(action.keyValue))
    }

    onProfileChanged: loadAssignedKeys()
    Component.onCompleted: loadAssignedKeys()
}
