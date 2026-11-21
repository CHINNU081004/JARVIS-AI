import QtQuick
import QtQuick.Templates as T

T.Button {
    id: control

    implicitWidth: Math.max(buttonContent.implicitWidth + 28, 88)
    implicitHeight: 36

    font.pixelSize: 12
    font.bold: true

    property color accentColor: "#00f0ff"
    property color baseColor: "#0f2048"
    property bool isSecondary: false
    property bool isDanger: false

    contentItem: Item {
        id: buttonContent
        implicitWidth: labelText.implicitWidth
        implicitHeight: labelText.implicitHeight

        Text {
            id: labelText
            anchors.centerIn: parent
            text: control.text
            font: control.font
            color: {
                if (!control.enabled) return "#64748b"
                if (control.isDanger) return "#f87171"
                if (control.isSecondary) return "#94a3b8"
                return "#f8fafc"
            }
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }

    background: Rectangle {
        implicitWidth: 100
        implicitHeight: 36
        radius: 10

        color: {
            if (!control.enabled) return "#081024"
            if (control.isDanger) return control.down ? "#7f1d1d" : (control.hovered ? "#991b1b" : "#450a0a")
            if (control.isSecondary) return control.down ? "#162850" : (control.hovered ? "#1c356e" : "#0e1a38")
            // Primary glass
            return control.down ? "#1d4ed8" : (control.hovered ? "#1e40af" : control.baseColor)
        }

        border.color: {
            if (!control.enabled) return "#1e293b"
            if (control.isDanger) return "#ef4444"
            if (control.visualFocus) return "#ffffff"
            return control.hovered ? control.accentColor : Qt.rgba(0, 0.94, 1.0, 0.4)
        }
        border.width: control.visualFocus ? 1.5 : 1

        // Glassmorphism subtle inner glow highlight
        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: parent.height * 0.4
            radius: 10
            gradient: Gradient {
                GradientStop { position: 0.0; color: Qt.rgba(1, 1, 1, control.hovered ? 0.18 : 0.08) }
                GradientStop { position: 1.0; color: "transparent" }
            }
        }

        Behavior on color { ColorAnimation { duration: 150 } }
        Behavior on border.color { ColorAnimation { duration: 150 } }

        scale: control.down ? 0.97 : (control.hovered ? 1.02 : 1.0)
        Behavior on scale { NumberAnimation { duration: 120 } }
    }
}
