import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls

T.ComboBox {
    id: control

    implicitWidth: Math.max(180, contentItem.implicitWidth + 36)
    implicitHeight: 36
    hoverEnabled: true

    font.pixelSize: 12
    font.bold: true

    property real cornerRadius: 12
    property color accentColor: "#00f0ff"
    readonly property bool isHovered: control.hovered || comboHoverMouse.containsMouse

    MouseArea {
        id: comboHoverMouse
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.NoButton
        propagateComposedEvents: true
        cursorShape: Qt.PointingHandCursor
    }

    contentItem: Text {
        leftPadding: 14
        rightPadding: control.indicator.width + 14
        text: control.displayText !== "" ? control.displayText : (control.currentText !== "" ? control.currentText : "Select...")
        font: control.font
        color: control.count > 0 ? (control.isHovered ? "#ffffff" : control.accentColor) : "#64748b"
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
        Behavior on color { ColorAnimation { duration: 150 } }
    }

    indicator: Item {
        x: control.width - width - 12
        y: (control.height - height) / 2
        width: 16
        height: 16
        VectorIcon {
            anchors.centerIn: parent
            name: "chevron-down"
            color: control.isHovered ? "#ffffff" : control.accentColor
            width: 14; height: 14
            rotation: control.popup.visible ? 180 : 0
            Behavior on rotation { NumberAnimation { duration: 180 } }
            Behavior on color { ColorAnimation { duration: 150 } }
        }
    }

    background: Rectangle {
        implicitWidth: 180
        implicitHeight: 36
        radius: control.cornerRadius
        color: control.down ? "#07122a" : (control.isHovered ? "#10234c" : "#091738")
        border.color: control.popup.visible ? control.accentColor : (control.isHovered ? "#00f0ff" : "#1e3a8a")
        border.width: (control.popup.visible || control.isHovered) ? 1.5 : 1

        scale: control.down ? 0.98 : (control.isHovered ? 1.02 : 1.0)
        Behavior on scale { NumberAnimation { duration: 120 } }
        Behavior on color { ColorAnimation { duration: 150 } }
        Behavior on border.color { ColorAnimation { duration: 150 } }

        // Top glass reflection highlight
        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: parent.height * 0.45
            radius: control.cornerRadius
            gradient: Gradient {
                GradientStop { position: 0.0; color: Qt.rgba(1, 1, 1, control.isHovered ? 0.18 : 0.05) }
                GradientStop { position: 1.0; color: "transparent" }
            }
        }

        // Exterior soft cyan glow outline on hover
        Rectangle {
            anchors.fill: parent
            anchors.margins: -2
            radius: control.cornerRadius + 2
            color: "transparent"
            border.color: Qt.rgba(0, 0.94, 1.0, 0.4)
            border.width: 1.5
            visible: control.isHovered || control.popup.visible
        }
    }

    popup: Popup {
        y: control.height + 4
        width: Math.max(control.width, 240)
        implicitHeight: Math.min(contentItem.implicitHeight + 16, 280)
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
        height: 38
        hoverEnabled: true

        contentItem: Text {
            text: {
                if (control.textRole) {
                    if (typeof model !== "undefined" && model !== null && model[control.textRole] !== undefined) {
                        return model[control.textRole];
                    }
                    if (typeof modelData !== "undefined" && modelData !== null && modelData[control.textRole] !== undefined) {
                        return modelData[control.textRole];
                    }
                }
                if (typeof modelData !== "undefined" && modelData !== null) {
                    return modelData;
                }
                return "";
            }
            color: (itemDelegate.highlighted || itemDelegate.hovered) ? control.accentColor : "#f8fafc"
            font.pixelSize: 12
            font.bold: itemDelegate.highlighted || itemDelegate.hovered
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
            leftPadding: 8
            Behavior on color { ColorAnimation { duration: 120 } }
        }

        background: Rectangle {
            radius: 8
            color: itemDelegate.highlighted ? "#163470" : (itemDelegate.hovered ? "#0d224d" : "transparent")
            border.color: (itemDelegate.hovered || itemDelegate.highlighted) ? "#00f0ff" : "transparent"
            border.width: 1
            Behavior on color { ColorAnimation { duration: 120 } }
            Behavior on border.color { ColorAnimation { duration: 120 } }
        }
    }
}
