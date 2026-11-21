import QtQuick

Rectangle {
    id: root
    radius: 12
    color: "#0f1c3f"
    border.color: "#1e3d7a"
    border.width: 1
    opacity: 0.95

    property color glowColor: "#00f0ff"
    property bool hoverGlow: false

    Rectangle {
        anchors.fill: parent
        radius: root.radius
        color: "transparent"
        border.color: root.glowColor
        border.width: 1
        opacity: root.hoverGlow ? 0.6 : 0.2
        Behavior on opacity { NumberAnimation { duration: 250 } }
    }
}
