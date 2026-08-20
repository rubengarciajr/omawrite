import QtQuick
import QtQuick.Controls
import QtQuick.Window

Button {
    id: control

    property string iconName
    property color iconColor: AppTheme.popupText

    implicitWidth: 42
    implicitHeight: 42
    padding: 0

    Keys.onReturnPressed: clicked()
    Keys.onEnterPressed: clicked()

    background: Rectangle {
        color: AppTheme.controlFill(control.activeFocus, control.hovered, control.down)
        border.color: AppTheme.controlBorder(control.activeFocus, control.hovered)
        border.width: control.activeFocus || control.hovered ? AppTheme.space(1) : 0
        radius: AppTheme.cornerRadius
    }

    contentItem: Item {
        Canvas {
            id: iconCanvas
            anchors.centerIn: parent

            // Canvas rasterizes one surface pixel per logical pixel and gets
            // upscaled blurry on hidpi screens. Draw at the physical size and
            // scale the item back down so surface pixels match screen pixels.
            readonly property real dpr: Screen.devicePixelRatio
            width: 20 * dpr
            height: 20 * dpr
            scale: 1 / dpr
            onDprChanged: requestPaint()

            onPaint: {
                var context = getContext("2d");
                context.setTransform(dpr, 0, 0, dpr, 0, 0);
                context.clearRect(0, 0, width, height);
                context.strokeStyle = control.iconColor;
                context.lineWidth = 2;
                context.lineCap = "square";
                context.lineJoin = "miter";
                context.beginPath();
                if (control.iconName === "close") {
                    context.moveTo(4, 4);
                    context.lineTo(16, 16);
                    context.moveTo(16, 4);
                    context.lineTo(4, 16);
                } else if (control.iconName === "up") {
                    context.moveTo(4, 13);
                    context.lineTo(10, 7);
                    context.lineTo(16, 13);
                } else {
                    context.moveTo(4, 7);
                    context.lineTo(10, 13);
                    context.lineTo(16, 7);
                }
                context.stroke();
            }

            Connections {
                target: control
                function onIconColorChanged() { iconCanvas.requestPaint(); }
                function onIconNameChanged() { iconCanvas.requestPaint(); }
            }
        }
    }
}
