import QtQuick
import QtQuick.Templates as T

T.CheckBox {
    id: control

    implicitWidth: Math.max(24, indicator.width + (control.text !== "" ? control.spacing + contentItem.implicitWidth : 0))
    implicitHeight: Math.max(24, indicator.height)

    padding: 0
    spacing: 8

    property color accentColor: "#00f0ff"

    indicator: Rectangle {
        implicitWidth: 20
        implicitHeight: 20
        x: control.leftPadding
        anchors.verticalCenter: parent.verticalCenter
        radius: 6
        color: control.checked ? Qt.rgba(0, 0.94, 1.0, 0.2) : "#071126"
        border.color: control.checked ? control.accentColor : (control.hovered ? "#0088ff" : "#1e3a8a")
        border.width: control.checked ? 1.5 : 1

        Behavior on color { ColorAnimation { duration: 150 } }
        Behavior on border.color { ColorAnimation { duration: 150 } }

        Text {
            anchors.centerIn: parent
            text: "✓"
            font.pixelSize: 12
            font.bold: true
            color: control.accentColor
            visible: control.checked
        }
    }

    contentItem: Text {
        leftPadding: control.indicator.width + control.spacing
        text: control.text
        font: control.font
        color: control.checked ? "#f8fafc" : "#94a3b8"
        verticalAlignment: Text.AlignVCenter
        visible: control.text !== ""
    }
}
