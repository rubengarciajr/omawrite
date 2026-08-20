import QtQuick
import QtQuick.Controls

Item {
    id: root

    property bool open: false
    property int bodySize: 20
    property int heading1Size: 34
    property int heading2Size: 28
    property int heading3Size: 24

    signal changeRequested(string level, int size)
    signal resetRequested()
    signal closeRequested()

    implicitWidth: AppTheme.space(370)
    implicitHeight: AppTheme.space(362)
    width: implicitWidth
    height: implicitHeight
    visible: open || opacity > 0
    enabled: open
    opacity: open ? 1 : 0
    scale: open ? 1 : 0.975
    transformOrigin: Item.BottomLeft

    Behavior on opacity { NumberAnimation { duration: 115; easing.type: Easing.OutCubic } }
    Behavior on scale { NumberAnimation { duration: 135; easing.type: Easing.OutCubic } }

    Rectangle {
        anchors.fill: parent
        color: AppTheme.menuBackground
        border.color: AppTheme.menuBorder
        border.width: AppTheme.space(1)
        radius: AppTheme.cornerRadius
    }

    Column {
        anchors.fill: parent
        anchors.margins: AppTheme.space(1)

        Item {
            width: parent.width
            height: AppTheme.space(58)

            Column {
                anchors.left: parent.left
                anchors.leftMargin: AppTheme.space(14)
                anchors.verticalCenter: parent.verticalCenter
                spacing: AppTheme.space(2)

                Text {
                    text: "TYPE FOUNDATION"
                    color: AppTheme.menuSelectedText
                    font.family: "iA Writer Mono S"
                    font.pixelSize: AppTheme.space(10)
                    font.letterSpacing: AppTheme.space(1)
                    font.weight: Font.DemiBold
                }

                Text {
                    text: "Document sizes · scales with Omarchy text size"
                    color: AppTheme.menuText
                    opacity: 0.56
                    font.family: "iA Writer Mono S"
                    font.pixelSize: AppTheme.space(9)
                }
            }

            Button {
                anchors.right: parent.right
                anchors.rightMargin: AppTheme.space(9)
                anchors.verticalCenter: parent.verticalCenter
                width: AppTheme.space(30)
                height: width
                padding: 0
                onClicked: root.closeRequested()

                contentItem: Item {
                    Rectangle {
                        anchors.centerIn: parent
                        width: AppTheme.space(13)
                        height: Math.max(1, AppTheme.space(1))
                        radius: height / 2
                        rotation: 45
                        color: AppTheme.menuText
                    }

                    Rectangle {
                        anchors.centerIn: parent
                        width: AppTheme.space(13)
                        height: Math.max(1, AppTheme.space(1))
                        radius: height / 2
                        rotation: -45
                        color: AppTheme.menuText
                    }
                }
                background: Rectangle {
                    color: AppTheme.controlFill(parent.activeFocus, parent.hovered, parent.down)
                    border.color: AppTheme.controlBorder(parent.activeFocus, parent.hovered)
                    border.width: parent.activeFocus || parent.hovered ? AppTheme.space(1) : 0
                    radius: AppTheme.cornerRadius
                }
            }
        }

        Rectangle {
            width: parent.width
            height: AppTheme.space(1)
            color: AppTheme.normalBorder
        }

        Repeater {
            model: [
                { id: "body", title: "Body", marker: "Aa", size: root.bodySize, minimum: 12, maximum: 36 },
                { id: "heading1", title: "Heading 1", marker: "H1", size: root.heading1Size, minimum: 16, maximum: 64 },
                { id: "heading2", title: "Heading 2", marker: "H2", size: root.heading2Size, minimum: 16, maximum: 64 },
                { id: "heading3", title: "Heading 3", marker: "H3", size: root.heading3Size, minimum: 16, maximum: 64 }
            ]

            delegate: Item {
                id: levelRow
                required property var modelData
                width: parent.width
                height: AppTheme.space(62)

                Rectangle {
                    anchors.left: parent.left
                    anchors.leftMargin: AppTheme.space(12)
                    anchors.verticalCenter: parent.verticalCenter
                    width: AppTheme.space(36)
                    height: width
                    color: AppTheme.normalFill
                    border.color: AppTheme.normalBorder
                    border.width: AppTheme.space(1)
                    radius: AppTheme.cornerRadius

                    Text {
                        anchors.fill: parent
                        text: levelRow.modelData.marker
                        color: levelRow.modelData.id === "body"
                            ? AppTheme.menuText : AppTheme.menuSelectedText
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        font.family: "iA Writer Mono S"
                        font.pixelSize: AppTheme.space(11)
                        font.weight: levelRow.modelData.id === "body" ? Font.Normal : Font.DemiBold
                    }
                }

                Column {
                    anchors.left: parent.left
                    anchors.leftMargin: AppTheme.space(60)
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: AppTheme.space(2)

                    Text {
                        text: levelRow.modelData.title
                        color: AppTheme.menuText
                        font.family: "iA Writer Mono S"
                        font.pixelSize: AppTheme.space(12)
                        font.weight: Font.Medium
                    }

                    Text {
                        text: levelRow.modelData.id === "body"
                            ? "Base writing size" : "Rendered heading size"
                        color: AppTheme.menuText
                        opacity: 0.5
                        font.family: "iA Writer Mono S"
                        font.pixelSize: AppTheme.space(9)
                    }
                }

                Row {
                    anchors.right: parent.right
                    anchors.rightMargin: AppTheme.space(12)
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: AppTheme.space(5)

                    StepButton {
                        width: AppTheme.space(30)
                        height: width
                        symbol: "−"
                        enabled: levelRow.modelData.size > levelRow.modelData.minimum
                        onClicked: root.changeRequested(levelRow.modelData.id,
                                                        levelRow.modelData.size - 1)
                    }

                    Text {
                        width: AppTheme.space(44)
                        height: AppTheme.space(30)
                        text: levelRow.modelData.size + " px"
                        color: AppTheme.menuSelectedText
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        font.family: "iA Writer Mono S"
                        font.pixelSize: AppTheme.space(10)
                        font.weight: Font.DemiBold
                    }

                    StepButton {
                        width: AppTheme.space(30)
                        height: width
                        symbol: "+"
                        enabled: levelRow.modelData.size < levelRow.modelData.maximum
                        onClicked: root.changeRequested(levelRow.modelData.id,
                                                        levelRow.modelData.size + 1)
                    }
                }
            }
        }

        Rectangle {
            width: parent.width
            height: AppTheme.space(1)
            color: AppTheme.normalBorder
        }

        Item {
            width: parent.width
            height: AppTheme.space(52)

            Text {
                anchors.left: parent.left
                anchors.leftMargin: AppTheme.space(14)
                anchors.verticalCenter: parent.verticalCenter
                text: "Changes apply instantly"
                color: AppTheme.menuText
                opacity: 0.5
                font.family: "iA Writer Mono S"
                font.pixelSize: AppTheme.space(9)
            }

            Button {
                anchors.right: parent.right
                anchors.rightMargin: AppTheme.space(12)
                anchors.verticalCenter: parent.verticalCenter
                width: AppTheme.space(72)
                height: AppTheme.space(30)
                padding: 0
                onClicked: root.resetRequested()
                contentItem: Text {
                    text: "Reset"
                    color: AppTheme.menuText
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.family: "iA Writer Mono S"
                    font.pixelSize: AppTheme.space(10)
                }
                background: Rectangle {
                    color: AppTheme.controlFill(parent.activeFocus, parent.hovered, parent.down)
                    border.color: AppTheme.controlBorder(parent.activeFocus, parent.hovered)
                    border.width: AppTheme.space(1)
                    radius: AppTheme.cornerRadius
                }
            }
        }
    }
}
