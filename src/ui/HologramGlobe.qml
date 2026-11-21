import QtQuick

Item {
    id: root
    width: 200
    height: 200

    property real rotationAngle: 0
    property bool animate: true

    NumberAnimation {
        target: root
        property: "rotationAngle"
        from: 0
        to: 360
        duration: 16000
        loops: Animation.Infinite
        running: root.animate
    }

    // Outer Neon Glow Ring
    Rectangle {
        anchors.centerIn: parent
        width: parent.width * 0.9
        height: parent.height * 0.9
        radius: width / 2
        color: "transparent"
        border.color: "#00f0ff"
        border.width: 1.5
        opacity: 0.4
    }

    // Tilted Orbital Ring
    Rectangle {
        anchors.centerIn: parent
        width: parent.width * 0.98
        height: parent.height * 0.45
        radius: width / 2
        color: "transparent"
        border.color: "#00aaff"
        border.width: 2
        rotation: 25
        opacity: 0.7
    }

    // Inner Glowing Core
    Rectangle {
        anchors.centerIn: parent
        width: parent.width * 0.6
        height: parent.height * 0.6
        radius: width / 2
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#1e40af" }
            GradientStop { position: 0.7; color: "#0b1b44" }
            GradientStop { position: 1.0; color: "#060e24" }
        }
        border.color: "#00e5ff"
        border.width: 1
    }

    // Friendly Bot Mask
    Item {
        anchors.centerIn: parent
        width: 60
        height: 60

        // Bot Face Screen
        Rectangle {
            anchors.fill: parent
            radius: 14
            color: "#030b1e"
            border.color: "#00f0ff"
            border.width: 1.5

            // Left Eye
            Rectangle {
                x: 14; y: 22
                width: 10; height: 16
                radius: 5
                color: "#00f0ff"
            }
            // Right Eye
            Rectangle {
                x: 36; y: 22
                width: 10; height: 16
                radius: 5
                color: "#00f0ff"
            }
        }
    }
}
