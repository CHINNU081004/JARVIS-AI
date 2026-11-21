import QtQuick

Item {
    id: root
    anchors.fill: parent
    clip: true

    property int count: 24
    property bool running: true

    Repeater {
        model: root.running ? root.count : 0
        delegate: Rectangle {
            id: p
            width: (index % 3) + 2
            height: width
            radius: width / 2
            color: (index % 2 === 0) ? "#00f0ff" : "#38bdf8"
            opacity: 0.15 + (index % 5) * 0.1

            property real startX: (index * 67) % root.width
            property real startY: (index * 91) % root.height

            x: startX
            y: startY

            SequentialAnimation on y {
                loops: Animation.Infinite
                NumberAnimation {
                    from: p.startY
                    to: p.startY - 40 - (index % 30)
                    duration: 4000 + (index * 250)
                    easing.type: Easing.InOutQuad
                }
                NumberAnimation {
                    from: p.startY - 40 - (index % 30)
                    to: p.startY
                    duration: 4000 + (index * 250)
                    easing.type: Easing.InOutQuad
                }
            }
        }
    }
}
