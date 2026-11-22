import QtQuick
import QtQuick.Templates as T

T.Slider {
    id: control

    implicitWidth: 200
    implicitHeight: 28

    property color accentColor: "#00f0ff"

    background: Rectangle {
        x: control.leftPadding
        y: control.topPadding + control.availableHeight / 2 - height / 2
        implicitWidth: 200
        implicitHeight: 6
        width: control.availableWidth
        height: implicitHeight
        radius: 3
        color: "#0c1b3d"
        border.color: "#1e3a8a"
        border.width: 1

        Rectangle {
            width: control.visualPosition * parent.width
            height: parent.height
            color: control.accentColor
            radius: 3

            Rectangle {
                anchors.fill: parent
                radius: 3
                gradient: Gradient {
                    GradientStop { position: 0.0; color: Qt.rgba(1, 1, 1, 0.3) }
                    GradientStop { position: 1.0; color: "transparent" }
                }
            }
        }
    }

    handle: Rectangle {
        x: control.leftPadding + control.visualPosition * (control.availableWidth - width)
        y: control.topPadding + control.availableHeight / 2 - height / 2
        implicitWidth: 18
        implicitHeight: 18
        radius: 9
        color: control.pressed ? "#ffffff" : control.accentColor
        border.color: "#050914"
        border.width: 2

        Rectangle {
            anchors.fill: parent
            anchors.margins: -2
            radius: 11
            color: "transparent"
            border.color: Qt.rgba(0, 0.94, 1.0, 0.4)
            border.width: 1.5
            visible: control.hovered || control.pressed
        }
    }
}
