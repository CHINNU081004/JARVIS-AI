import QtQuick
import QtQuick.Templates as T

T.CheckBox {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftPadding + rightPadding,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topPadding + bottomPadding,
                             implicitContentHeight + topPadding + bottomPadding,
                             indicator ? indicator.height + topPadding + bottomPadding : 0)

    padding: 6
    spacing: 10

    property color accentColor: "#00f0ff"

    indicator: Rectangle {
        implicitWidth: 22
        implicitHeight: 22
        x: control.leftPadding
        y: parent.height / 2 - height / 2
        radius: 6
        color: control.checked ? Qt.rgba(0, 0.94, 1.0, 0.2) : "#071126"
        border.color: control.checked ? control.accentColor : (control.hovered ? "#0088ff" : "#1e3a8a")
        border.width: control.checked ? 1.5 : 1

        Behavior on color { ColorAnimation { duration: 150 } }
        Behavior on border.color { ColorAnimation { duration: 150 } }

        // Checkmark vector / glyph
        Text {
            anchors.centerIn: parent
            text: "✓"
            font.pixelSize: 13
            font.bold: true
            color: control.accentColor
            visible: control.checked
        }
    }

    contentItem: Text {
        leftPadding: control.indicator ? control.indicator.width + control.spacing : 0
        text: control.text
        font: control.font
        color: control.checked ? "#f8fafc" : "#94a3b8"
        verticalAlignment: Text.AlignVCenter
    }
}
