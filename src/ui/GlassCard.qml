import QtQuick

Rectangle {
    id: root
    radius: 12
    color: "#0c1836"
    border.color: "#1e3d7a"
    border.width: 1
    opacity: 0.95

    property color glowColor: (typeof jarvis !== "undefined" && jarvis.accentColor) ? jarvis.accentColor : "#00f0ff"
    property bool hoverGlow: false

    // Dynamic accent glow layer
    Rectangle {
        anchors.fill: parent
        radius: root.radius
        color: "transparent"
        border.color: root.glowColor
        border.width: 1
        opacity: 0.22
        Behavior on border.color { ColorAnimation { duration: 200 } }
    }

    // Top subtle static reflection sheen
    Rectangle {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: parent.height * 0.35
        radius: root.radius
        gradient: Gradient {
            GradientStop { position: 0.0; color: Qt.rgba(1, 1, 1, 0.03) }
            GradientStop { position: 1.0; color: "transparent" }
        }
    }
}
