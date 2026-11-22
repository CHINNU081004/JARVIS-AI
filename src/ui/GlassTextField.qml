import QtQuick
import QtQuick.Templates as T

T.TextField {
    id: control

    implicitWidth: Math.max(200, placeholder.implicitWidth + leftPadding + rightPadding)
    implicitHeight: 38

    font.pixelSize: 13
    color: "#f8fafc"
    selectedTextColor: "#050914"
    selectionColor: "#00f0ff"
    verticalAlignment: TextInput.AlignVCenter

    leftPadding: leadingIcon !== "" ? 38 : 14
    rightPadding: trailingClearButton && control.text !== "" ? 36 : 14

    property string leadingIcon: ""
    property bool trailingClearButton: false
    property real cornerRadius: 12

    placeholderTextColor: "#64748b"

    Text {
        id: placeholder
        x: control.leftPadding
        y: control.topPadding
        width: control.width - (control.leftPadding + control.rightPadding)
        height: control.height - (control.topPadding + control.bottomPadding)
        text: control.placeholderText
        font: control.font
        color: control.placeholderTextColor
        verticalAlignment: control.verticalAlignment
        visible: !control.length && !control.preeditText && (!control.activeFocus || control.horizontalAlignment !== Qt.AlignHCenter)
        elide: Text.ElideRight
    }

    // Leading Vector Icon (e.g. search, folder)
    VectorIcon {
        anchors.left: parent.left
        anchors.leftMargin: 12
        anchors.verticalCenter: parent.verticalCenter
        name: control.leadingIcon
        color: control.activeFocus ? "#00f0ff" : "#64748b"
        width: 16; height: 16
        visible: control.leadingIcon !== ""
        Behavior on color { ColorAnimation { duration: 150 } }
    }

    // Trailing Clear Button (when text is present)
    Rectangle {
        anchors.right: parent.right
        anchors.rightMargin: 10
        anchors.verticalCenter: parent.verticalCenter
        width: 20; height: 20
        radius: 10
        color: clearMouse.containsMouse ? "#1e3a8a" : "transparent"
        visible: control.trailingClearButton && control.text !== ""

        Text {
            anchors.centerIn: parent
            text: "✕"
            color: clearMouse.containsMouse ? "#00f0ff" : "#64748b"
            font.pixelSize: 11
            font.bold: true
        }

        MouseArea {
            id: clearMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                control.text = "";
                control.forceActiveFocus();
            }
        }
    }

    background: Rectangle {
        implicitWidth: 200
        implicitHeight: 38
        radius: control.cornerRadius
        color: control.activeFocus ? "#0a1838" : (control.hovered ? "#091530" : "#071126")

        border.color: {
            if (!control.enabled) return "#1e293b"
            if (control.activeFocus) return "#00f0ff"
            if (control.hovered) return "#0088ff"
            return "#1b3366"
        }
        border.width: control.activeFocus ? 1.5 : 1

        // Subtle upper glass gradient sheen
        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: parent.height * 0.45
            radius: control.cornerRadius
            gradient: Gradient {
                GradientStop { position: 0.0; color: Qt.rgba(1, 1, 1, control.activeFocus ? 0.08 : (control.hovered ? 0.05 : 0.02)) }
                GradientStop { position: 1.0; color: "transparent" }
            }
        }

        Behavior on color { ColorAnimation { duration: 150 } }
        Behavior on border.color { ColorAnimation { duration: 150 } }
    }
}
