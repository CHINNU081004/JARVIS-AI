import QtQuick

Item {
    id: root
    width: 220
    height: 220

    property real rotationAngle: 0
    property real pulseScale: 1.0
    property int animationMode: 2 // 0: Off, 1: Reduced, 2: Full

    // Continuous 360 Rotation for Orbital Rings
    NumberAnimation {
        target: root
        property: "rotationAngle"
        from: 0
        to: 360
        duration: root.animationMode === 1 ? 24000 : 12000
        loops: Animation.Infinite
        running: root.animationMode > 0
    }

    // Breathing Pulse Animation for Holographic Glow
    SequentialAnimation {
        running: root.animationMode > 0
        loops: Animation.Infinite
        NumberAnimation {
            target: root
            property: "pulseScale"
            from: 0.96
            to: 1.04
            duration: 2500
            easing.type: Easing.InOutQuad
        }
        NumberAnimation {
            target: root
            property: "pulseScale"
            from: 1.04
            to: 0.96
            duration: 2500
            easing.type: Easing.InOutQuad
        }
    }

    // Outer Ambient Glow Halo
    Rectangle {
        anchors.centerIn: parent
        width: parent.width * 0.95 * root.pulseScale
        height: parent.height * 0.95 * root.pulseScale
        radius: width / 2
        color: "transparent"
        border.color: "#00f0ff"
        border.width: 1
        opacity: root.animationMode === 2 ? 0.35 : 0.2
    }

    // Orbit Ring 1 (Clockwise)
    Rectangle {
        anchors.centerIn: parent
        width: parent.width * 0.92
        height: parent.height * 0.44
        radius: width / 2
        color: "transparent"
        border.color: "#00c8ff"
        border.width: 1.5
        rotation: 25 + (root.rotationAngle * 0.3)
        opacity: 0.75

        // Satellite Orb on Ring 1
        Rectangle {
            x: parent.width * 0.85
            y: parent.height * 0.5 - 4
            width: 8; height: 8; radius: 4
            color: "#00f0ff"
            visible: root.animationMode === 2
        }
    }

    // Orbit Ring 2 (Counter-Clockwise tilt)
    Rectangle {
        anchors.centerIn: parent
        width: parent.width * 0.86
        height: parent.height * 0.42
        radius: width / 2
        color: "transparent"
        border.color: "#0088ff"
        border.width: 1.5
        rotation: -40 - (root.rotationAngle * 0.25)
        opacity: 0.65

        // Satellite Orb on Ring 2
        Rectangle {
            x: parent.width * 0.15
            y: parent.height * 0.5 - 4
            width: 6; height: 6; radius: 3
            color: "#38bdf8"
            visible: root.animationMode === 2
        }
    }

    // Central Holographic Orb Sphere
    Rectangle {
        anchors.centerIn: parent
        width: parent.width * 0.58
        height: parent.height * 0.58
        radius: width / 2
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#1e3a8a" }
            GradientStop { position: 0.6; color: "#0c1f4d" }
            GradientStop { position: 1.0; color: "#050d24" }
        }
        border.color: "#00f0ff"
        border.width: 1.5

        // Digital Grid Lines
        Rectangle {
            anchors.centerIn: parent
            width: parent.width * 0.9
            height: 1
            color: "#00f0ff"
            opacity: 0.3
        }
        Rectangle {
            anchors.centerIn: parent
            width: 1
            height: parent.height * 0.9
            color: "#00f0ff"
            opacity: 0.3
        }

        // Friendly Bot Mascot Face Screen
        Rectangle {
            anchors.centerIn: parent
            width: 58
            height: 52
            radius: 14
            color: "#030a1c"
            border.color: "#00f0ff"
            border.width: 1.5

            // Left Glowing Cyan Eye
            Rectangle {
                x: 13; y: 18
                width: 9; height: 16
                radius: 4.5
                color: "#00f0ff"
            }

            // Right Glowing Cyan Eye
            Rectangle {
                x: 36; y: 18
                width: 9; height: 16
                radius: 4.5
                color: "#00f0ff"
            }
        }
    }
}
