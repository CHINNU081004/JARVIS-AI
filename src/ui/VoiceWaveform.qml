import QtQuick

Row {
    id: root
    spacing: 3
    height: 24

    property bool active: false
    property color waveColor: "#00f0ff"
    property int animationMode: 2

    Repeater {
        model: [10, 16, 22, 14, 8, 18, 24, 12, 19, 7]
        delegate: Rectangle {
            id: bar
            width: 3
            height: root.active && root.animationMode > 0 ? dynHeight : modelData * 0.4
            radius: 1.5
            color: root.waveColor
            anchors.verticalCenter: parent.verticalCenter

            property real dynHeight: modelData

            SequentialAnimation on dynHeight {
                running: root.active && root.animationMode > 0
                loops: Animation.Infinite
                NumberAnimation {
                    from: modelData * 0.3
                    to: modelData * 1.2
                    duration: 250 + (index * 40)
                    easing.type: Easing.InOutSine
                }
                NumberAnimation {
                    from: modelData * 1.2
                    to: modelData * 0.3
                    duration: 250 + (index * 40)
                    easing.type: Easing.InOutSine
                }
            }
        }
    }
}
