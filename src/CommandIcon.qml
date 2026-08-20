import QtQuick
import QtQuick.Window

Item {
    id: root

    property string iconName
    property color iconColor: AppTheme.menuText

    Canvas {
        id: canvas
        readonly property real dpr: Screen.devicePixelRatio
        width: root.width * dpr
        height: root.height * dpr
        scale: 1 / dpr
        transformOrigin: Item.TopLeft

        onDprChanged: requestPaint()
        onPaint: {
            var context = getContext("2d");
            context.resetTransform();
            context.clearRect(0, 0, width, height);
            context.setTransform(dpr, 0, 0, dpr, 0, 0);
            context.strokeStyle = root.iconColor;
            context.fillStyle = root.iconColor;
            context.lineWidth = 1.45;
            context.lineCap = "round";
            context.lineJoin = "round";

            var w = root.width;
            var h = root.height;
            var cx = w / 2;
            var cy = h / 2;
            context.beginPath();

            if (root.iconName === "text") {
                context.moveTo(5, 7); context.lineTo(w - 5, 7);
                context.moveTo(5, cy); context.lineTo(w - 8, cy);
                context.moveTo(5, h - 7); context.lineTo(w - 11, h - 7);
            } else if (root.iconName === "todo") {
                context.rect(5, 5, w - 10, h - 10);
                context.moveTo(8, cy); context.lineTo(11, cy + 3);
                context.lineTo(w - 7, 8);
            } else if (root.iconName === "bullet") {
                for (var b = 0; b < 3; ++b) {
                    var by = 7 + b * 6;
                    context.moveTo(5, by); context.lineTo(5.2, by);
                    context.moveTo(9, by); context.lineTo(w - 5, by);
                }
            } else if (root.iconName === "numbered") {
                context.font = "8px iA Writer Mono S";
                context.textAlign = "center";
                context.textBaseline = "middle";
                for (var n = 0; n < 3; ++n) {
                    var ny = 7 + n * 6;
                    context.fillText(String(n + 1), 5, ny);
                    context.moveTo(9, ny); context.lineTo(w - 5, ny);
                }
            } else if (root.iconName === "quote") {
                context.moveTo(6, 5); context.lineTo(6, h - 5);
                context.moveTo(11, 9); context.lineTo(w - 5, 9);
                context.moveTo(11, 15); context.lineTo(w - 8, 15);
            }
            context.stroke();
        }

        Connections {
            target: root
            function onIconNameChanged() { canvas.requestPaint(); }
            function onIconColorChanged() { canvas.requestPaint(); }
        }
    }

    Text {
        anchors.centerIn: parent
        visible: root.iconName.indexOf("heading") === 0
        text: "H" + root.iconName.slice(-1)
        color: root.iconColor
        font.family: "iA Writer Mono S"
        font.pixelSize: AppTheme.space(12)
        font.weight: Font.Medium
    }
}
