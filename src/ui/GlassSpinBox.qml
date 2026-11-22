import QtQuick
import QtQuick.Templates as T

T.SpinBox {
    id: control

    implicitWidth: 140
    implicitHeight: 36
    padding: 4

    property color accentColor: "#00f0ff"
    property real cornerRadius: 10

    font.pixelSize: 12
    font.bold: true

    contentItem: TextInput {
        z: 2
        text: control.textFromValue(control.value, control.locale)
        font: control.font
        color: "#00f0ff"
        selectionColor: control.accentColor
        selectedTextColor: "#050914"
        horizontalAlignment: Qt.AlignHCenter
        verticalAlignment: Qt.AlignVCenter
        readOnly: !control.editable
        validator: control.validator
        inputMethodHints: control.inputMethodHints
    }

    up.indicator: Rectangle {
        x: control.mirrored ? 4 : parent.width - width - 4
        y: 4
        height: parent.height - 8
        implicitWidth: 28
        radius: 6
        color: control.up.pressed ? "#1e3a8a" : (control.up.hovered ? "#0d204d" : "#081329")
        border.color: control.up.hovered ? control.accentColor : "#1e3a8a"

        Text {
            text: "+"
            font.pixelSize: 14
            font.bold: true
            color: control.up.hovered ? control.accentColor : "#94a3b8"
            anchors.centerIn: parent
        }
    }

    down.indicator: Rectangle {
        x: control.mirrored ? parent.width - width - 4 : 4
        y: 4
        height: parent.height - 8
        implicitWidth: 28
        radius: 6
        color: control.down.pressed ? "#1e3a8a" : (control.down.hovered ? "#0d204d" : "#081329")
        border.color: control.down.hovered ? control.accentColor : "#1e3a8a"

        Text {
            text: "-"
            font.pixelSize: 14
            font.bold: true
            color: control.down.hovered ? control.accentColor : "#94a3b8"
            anchors.centerIn: parent
        }
    }

    background: Rectangle {
        implicitWidth: 140
        implicitHeight: 36
        radius: control.cornerRadius
        color: "#071126"
        border.color: control.hovered ? "#0088ff" : "#1e3a8a"
        border.width: 1
    }
}
