import QtQuick
import QtQuick.Controls

Button {
    id: control

    property bool primary: false
    property bool darkMode: true
    property color labelColor: primary ? AppTheme.background : AppTheme.foreground
    property color activeColor: AppTheme.accent
    property real textScale: 1

    leftPadding: 16
    rightPadding: 16
    topPadding: 7
    bottomPadding: 7

    Keys.onReturnPressed: clicked()
    Keys.onEnterPressed: clicked()

    contentItem: Label {
        text: control.text
        color: control.labelColor
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        font.family: "iA Writer Mono S"
        font.pixelSize: Math.round(12 * control.textScale)
    }

    background: Rectangle {
        implicitWidth: 88
        implicitHeight: 34
        radius: AppTheme.cornerRadius
        color: control.primary
            ? control.activeColor
            : AppTheme.controlFill(control.activeFocus, control.hovered, control.down)
        opacity: control.primary && control.down ? 0.78
                 : control.primary && control.hovered ? 0.9 : 1
        border.color: control.primary
            ? control.activeColor
            : AppTheme.controlBorder(control.activeFocus, control.hovered)
        border.width: AppTheme.space(1)
    }
}
