import QtQuick

Rectangle {
    id: root
    radius: 12
    color: "#0c1836"
    border.color: hoverArea.containsMouse ? "#00f0ff" : "#1e3d7a"
    border.width: hoverArea.containsMouse ? 1.5 : 1
    opacity: 0.95

    property color glowColor: "#00f0ff"
    property bool hoverGlow: true

    Behavior on border.color { ColorAnimation { duration: 180 } }

    // Specular glass glow layer
    Rectangle {
        anchors.fill: parent
        radius: root.radius
        color: "transparent"
        border.color: root.glowColor
        border.width: 1
        opacity: hoverArea.containsMouse ? 0.75 : 0.15
        Behavior on opacity { NumberAnimation { duration: 200 } }
    }

    // Top soft reflection
    Rectangle {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: parent.height * 0.35
        radius: root.radius
        gradient: Gradient {
            GradientStop { position: 0.0; color: Qt.rgba(1, 1, 1, hoverArea.containsMouse ? 0.08 : 0.03) }
            GradientStop { position: 1.0; color: "transparent" }
        }
    }

    MouseArea {
        id: hoverArea
        anchors.fill: parent
        hoverEnabled: root.hoverGlow
        acceptedButtons: Qt.NoButton
        propagateComposedEvents: true
    }
}
