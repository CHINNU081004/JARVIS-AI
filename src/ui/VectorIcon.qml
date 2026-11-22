import QtQuick

Item {
    id: root
    implicitWidth: 18
    implicitHeight: 18

    property string name: "bot"
    property color color: "#00f0ff"
    property real strokeWidth: 1.5

    Canvas {
        id: canvas
        anchors.fill: parent
        antialiasing: true

        onPaint: {
            var ctx = getContext("2d");
            ctx.reset();
            ctx.strokeStyle = root.color;
            ctx.lineWidth = root.strokeWidth;
            ctx.lineCap = "round";
            ctx.lineJoin = "round";

            var w = width;
            var h = height;

            if (root.name === "chat") {
                ctx.beginPath();
                ctx.moveTo(w * 0.15, h * 0.2);
                ctx.lineTo(w * 0.85, h * 0.2);
                ctx.lineTo(w * 0.85, h * 0.7);
                ctx.lineTo(w * 0.45, h * 0.7);
                ctx.lineTo(w * 0.2, h * 0.9);
                ctx.lineTo(w * 0.2, h * 0.7);
                ctx.lineTo(w * 0.15, h * 0.7);
                ctx.closePath();
                ctx.stroke();
            } else if (root.name === "folder" || root.name === "workspace") {
                ctx.beginPath();
                ctx.moveTo(w * 0.1, h * 0.25);
                ctx.lineTo(w * 0.4, h * 0.25);
                ctx.lineTo(w * 0.55, h * 0.4);
                ctx.lineTo(w * 0.9, h * 0.4);
                ctx.lineTo(w * 0.9, h * 0.85);
                ctx.lineTo(w * 0.1, h * 0.85);
                ctx.closePath();
                ctx.stroke();
            } else if (root.name === "memory" || root.name === "brain") {
                ctx.beginPath();
                ctx.arc(w * 0.35, h * 0.4, w * 0.2, Math.PI * 0.5, Math.PI * 1.5);
                ctx.arc(w * 0.5, h * 0.25, w * 0.18, Math.PI, Math.PI * 2);
                ctx.arc(w * 0.65, h * 0.4, w * 0.2, Math.PI * 1.5, Math.PI * 0.5);
                ctx.arc(w * 0.5, h * 0.75, w * 0.2, 0, Math.PI);
                ctx.closePath();
                ctx.stroke();
            } else if (root.name === "settings" || root.name === "gear") {
                ctx.beginPath();
                ctx.arc(w * 0.5, h * 0.5, w * 0.22, 0, Math.PI * 2);
                ctx.stroke();
                ctx.beginPath();
                ctx.arc(w * 0.5, h * 0.5, w * 0.38, 0, Math.PI * 2);
                ctx.stroke();
            } else if (root.name === "bot") {
                ctx.beginPath();
                // Screen outline
                ctx.roundRect ? ctx.roundRect(w * 0.15, h * 0.25, w * 0.7, h * 0.55, 4) : ctx.rect(w * 0.15, h * 0.25, w * 0.7, h * 0.55);
                ctx.stroke();
                // Antenna
                ctx.beginPath();
                ctx.moveTo(w * 0.5, h * 0.25);
                ctx.lineTo(w * 0.5, h * 0.1);
                ctx.stroke();
                // Eyes
                ctx.fillStyle = root.color;
                ctx.fillRect(w * 0.3, h * 0.42, 2.5, 4);
                ctx.fillRect(w * 0.6, h * 0.42, 2.5, 4);
            } else if (root.name === "search") {
                ctx.beginPath();
                ctx.arc(w * 0.42, h * 0.42, w * 0.25, 0, Math.PI * 2);
                ctx.stroke();
                ctx.beginPath();
                ctx.moveTo(w * 0.60, h * 0.60);
                ctx.lineTo(w * 0.85, h * 0.85);
                ctx.stroke();
            } else if (root.name === "chevron-down") {
                ctx.beginPath();
                ctx.moveTo(w * 0.25, h * 0.38);
                ctx.lineTo(w * 0.5, h * 0.65);
                ctx.lineTo(w * 0.75, h * 0.38);
                ctx.stroke();
            } else if (root.name === "refresh") {
                ctx.beginPath();
                ctx.arc(w * 0.5, h * 0.5, w * 0.35, Math.PI * 0.2, Math.PI * 1.8);
                ctx.stroke();
                ctx.beginPath();
                ctx.moveTo(w * 0.75, h * 0.35);
                ctx.lineTo(w * 0.85, h * 0.5);
                ctx.lineTo(w * 0.65, h * 0.5);
                ctx.closePath();
                ctx.fillStyle = root.color;
                ctx.fill();
            } else {
                ctx.beginPath();
                ctx.arc(w * 0.5, h * 0.5, w * 0.3, 0, Math.PI * 2);
                ctx.stroke();
            }
        }
    }

    onColorChanged: canvas.requestPaint()
    onNameChanged: canvas.requestPaint()
}
