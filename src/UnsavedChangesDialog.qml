import QtQuick
import QtQuick.Controls

Dialog {
    id: root

    property string fileName: "Untitled.md"
    property bool darkMode: true
    property color textColor: AppTheme.popupText
    property color strongTextColor: AppTheme.popupText
    property color activeButtonColor: AppTheme.accent
    property int containerWidth: 420
    property int containerHeight: 320
    property real textScale: 1

    signal saveRequested()
    signal discardRequested()
    signal cancelRequested()

    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape
    onRejected: cancelRequested()

    onOpened: saveButton.forceActiveFocus()
    width: Math.min(420, containerWidth - 48)
    x: Math.round((containerWidth - width) / 2)
    y: Math.round((containerHeight - height) / 2)
    padding: 20

    background: Rectangle {
        color: AppTheme.popupBackground
        border.color: AppTheme.popupBorder
        border.width: AppTheme.space(1)
        radius: AppTheme.cornerRadius
    }

    contentItem: Column {
        spacing: 12

        Label {
            text: "Unsaved changes"
            color: root.strongTextColor
            font.family: "iA Writer Mono S"
            font.pixelSize: Math.round(16 * root.textScale)
            font.bold: true
        }

        Label {
            width: parent.width
            text: "Save changes to " + root.fileName + " before closing?"
            color: root.textColor
            wrapMode: Text.Wrap
            font.family: "iA Writer Mono S"
            font.pixelSize: Math.round(13 * root.textScale)
        }
    }

    footer: Item {
        implicitHeight: dialogButtons.implicitHeight + 20

        Row {
            id: dialogButtons
            anchors.right: parent.right
            anchors.rightMargin: 20
            anchors.verticalCenter: parent.verticalCenter
            spacing: 8

            SquareDialogButton {
                id: cancelButton
                text: "Cancel"
                darkMode: root.darkMode
                textScale: root.textScale
                labelColor: root.textColor
                KeyNavigation.left: saveButton
                KeyNavigation.right: discardButton
                KeyNavigation.tab: discardButton
                KeyNavigation.backtab: saveButton
                onClicked: root.reject()
            }

            SquareDialogButton {
                id: discardButton
                text: "Discard"
                darkMode: root.darkMode
                textScale: root.textScale
                labelColor: root.textColor
                KeyNavigation.left: cancelButton
                KeyNavigation.right: saveButton
                KeyNavigation.tab: saveButton
                KeyNavigation.backtab: cancelButton
                onClicked: {
                    root.close();
                    root.discardRequested();
                }
            }

            SquareDialogButton {
                id: saveButton
                text: "Save"
                primary: true
                darkMode: root.darkMode
                textScale: root.textScale
                activeColor: root.activeButtonColor
                KeyNavigation.left: discardButton
                KeyNavigation.right: cancelButton
                KeyNavigation.tab: cancelButton
                KeyNavigation.backtab: discardButton
                onClicked: {
                    root.close();
                    root.saveRequested();
                }
            }
        }
    }
}
