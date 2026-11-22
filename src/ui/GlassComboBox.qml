import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls

T.ComboBox {
    id: control

    implicitWidth: Math.max(180, contentItem.implicitWidth + 36)
    implicitHeight: 36

    font.pixelSize: 12
    font.bold: true

    property real cornerRadius: 12
    property color accentColor: "#00f0ff"

    contentItem: Text {
        leftPadding: 14
        rightPadding: control.indicator.width + 14
        text: control.displayText !== "" ? control.displayText : "Select..."
        font: control.font
        color: control.count > 0 ? control.accentColor : "#64748b"
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    indicator: Item {
        x: control.width - width - 12
        y: (control.height - height) / 2
        width: 16
        height: 16
        VectorIcon {
            anchors.centerIn: parent
            name: "chevron-down"
            color: control.accentColor
            width: 14; height: 14
            rotation: control.popup.visible ? 180 : 0
            Behavior on rotation { NumberAnimation { duration: 180 } }
        }
    }

    background: Rectangle {
        implicitWidth: 180
        implicitHeight: 36
        radius: control.cornerRadius
        color: control.down ? "#07122a" : (control.hovered ? "#0d204d" : "#091738")
        border.color: control.popup.visible ? control.accentColor : (control.hovered ? "#0088ff" : "#1e3a8a")
        border.width: control.popup.visible ? 1.5 : 1

        // Top glass reflection
        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: parent.height * 0.4
            radius: control.cornerRadius
            gradient: Gradient {
                GradientStop { position: 0.0; color: Qt.rgba(1, 1, 1, control.hovered ? 0.09 : 0.04) }
                GradientStop { position: 1.0; color: "transparent" }
            }
        }

        Behavior on color { ColorAnimation { duration: 150 } }
        Behavior on border.color { ColorAnimation { duration: 150 } }
    }

    popup: Popup {
        y: control.height + 4
        width: Math.max(control.width, 220)
        implicitHeight: Math.min(contentItem.implicitHeight + 16, 260)
        padding: 6

        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: control.popup.visible ? control.delegateModel : null
            currentIndex: control.highlightedIndex
            ScrollIndicator.vertical: ScrollIndicator { }
        }

        background: Rectangle {
            radius: control.cornerRadius
            color: "#07122c"
            border.color: control.accentColor
            border.width: 1.5

            // Outer subtle drop glow
            Rectangle {
                anchors.fill: parent
                anchors.margins: -1
                radius: control.cornerRadius + 1
                color: "transparent"
                border.color: Qt.rgba(0, 0.94, 1.0, 0.25)
                border.width: 1
            }
        }
    }

    delegate: ItemDelegate {
        id: itemDelegate
        width: control.popup.width - 12
        height: 36

        contentItem: Text {
            text: control.textRole ? (Array.isArray(control.model) ? modelData[control.textRole] : model[control.textRole]) : modelData
            color: itemDelegate.highlighted ? control.accentColor : "#f8fafc"
            font.pixelSize: 12
            font.bold: itemDelegate.highlighted
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
            leftPadding: 6
        }

        background: Rectangle {
            radius: 8
            color: itemDelegate.highlighted ? "#163470" : "transparent"
            Behavior on color { ColorAnimation { duration: 120 } }
        }
    }
}
