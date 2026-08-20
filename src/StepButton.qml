import QtQuick
import QtQuick.Controls

Button {
    id: control

    property string symbol: "+"
    property color symbolColor: AppTheme.menuText

    implicitWidth: AppTheme.space(30)
    implicitHeight: AppTheme.space(30)
    padding: 0
    focusPolicy: Qt.NoFocus
    leftPadding: 0
    rightPadding: 0
    topPadding: 0
    bottomPadding: 0

    contentItem: Item {
        Text {
            anchors.fill: parent
            text: control.symbol
            color: control.symbolColor
            opacity: control.enabled ? 1 : 0.32
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            font.family: "iA Writer Mono S"
            font.pixelSize: AppTheme.space(control.symbol === "−" ? 15 : 14)
            font.weight: Font.Medium
        }
    }

    background: Rectangle {
        color: AppTheme.controlFill(control.activeFocus, control.hovered, control.down)
        border.color: AppTheme.controlBorder(control.activeFocus, control.hovered)
        border.width: AppTheme.space(1)
        radius: AppTheme.cornerRadius
    }
}
