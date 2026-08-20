import QtQuick
import QtQuick.Controls

Item {
    id: root

    property bool open: false
    property string query: ""
    property int selectedIndex: 0
    readonly property int rowHeight: AppTheme.space(62)
    readonly property int headerHeight: AppTheme.space(45)
    readonly property var commands: [
        { id: "text", title: "Text", description: "Just start typing with plain text", icon: "text", keywords: "plain paragraph" },
        { id: "todo", title: "To-do List", description: "Track tasks with a to-do list", icon: "todo", keywords: "task checkbox check" },
        { id: "heading1", title: "Heading 1", description: "Big section heading", icon: "heading1", keywords: "h1 title" },
        { id: "heading2", title: "Heading 2", description: "Medium section heading", icon: "heading2", keywords: "h2 subtitle" },
        { id: "heading3", title: "Heading 3", description: "Small section heading", icon: "heading3", keywords: "h3 subtitle" },
        { id: "bullet", title: "Bullet List", description: "Create a simple bullet list", icon: "bullet", keywords: "unordered list" },
        { id: "numbered", title: "Numbered List", description: "Create a list with numbering", icon: "numbered", keywords: "ordered list" },
        { id: "quote", title: "Quote", description: "Capture a quote", icon: "quote", keywords: "blockquote citation" }
    ]
    readonly property var filteredCommands: {
        var needle = query.trim().toLocaleLowerCase();
        if (needle.length === 0)
            return commands;
        return commands.filter(function(command) {
            return (command.title + " " + command.keywords).toLocaleLowerCase().indexOf(needle) !== -1;
        });
    }

    signal commandTriggered(string commandId)

    implicitWidth: AppTheme.space(356)
    implicitHeight: Math.min(AppTheme.space(455),
                             headerHeight + AppTheme.space(2)
                             + Math.max(rowHeight, filteredCommands.length * rowHeight)
                             + AppTheme.space(10))
    width: implicitWidth
    height: implicitHeight
    visible: open || opacity > 0
    enabled: open
    opacity: open ? 1 : 0
    scale: open ? 1 : 0.975
    transformOrigin: Item.TopLeft

    onQueryChanged: selectedIndex = 0
    onFilteredCommandsChanged: {
        if (selectedIndex >= filteredCommands.length)
            selectedIndex = Math.max(0, filteredCommands.length - 1);
    }

    function moveSelection(delta) {
        if (filteredCommands.length === 0)
            return;
        selectedIndex = (selectedIndex + delta + filteredCommands.length)
                        % filteredCommands.length;
        commandList.positionViewAtIndex(selectedIndex, ListView.Contain);
    }

    function activateSelected() {
        if (filteredCommands.length === 0)
            return false;
        commandTriggered(filteredCommands[selectedIndex].id);
        return true;
    }

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
            height: root.headerHeight

            Text {
                anchors.left: parent.left
                anchors.leftMargin: AppTheme.space(13)
                anchors.verticalCenter: parent.verticalCenter
                text: root.query.length > 0 ? "OMAWRITE · " + root.query.toLocaleUpperCase()
                                            : "OMAWRITE COMMANDS"
                color: AppTheme.menuSelectedText
                font.family: "iA Writer Mono S"
                font.pixelSize: AppTheme.space(10)
                font.letterSpacing: AppTheme.space(1)
                font.weight: Font.DemiBold
            }

            Text {
                anchors.right: parent.right
                anchors.rightMargin: AppTheme.space(13)
                anchors.verticalCenter: parent.verticalCenter
                text: "↑↓  ENTER  ESC"
                color: AppTheme.menuText
                opacity: 0.52
                font.family: "iA Writer Mono S"
                font.pixelSize: AppTheme.space(9)
            }
        }

        Rectangle {
            width: parent.width
            height: AppTheme.space(1)
            color: AppTheme.normalBorder
        }

        ListView {
            id: commandList
            width: parent.width
            height: root.height - root.headerHeight - AppTheme.space(3)
            clip: true
            model: root.filteredCommands
            currentIndex: root.selectedIndex
            boundsBehavior: Flickable.StopAtBounds
            leftMargin: AppTheme.space(5)
            rightMargin: AppTheme.space(5)
            topMargin: AppTheme.space(5)
            bottomMargin: AppTheme.space(5)

            ScrollBar.vertical: ScrollBar {
                policy: commandList.contentHeight > commandList.height
                        ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff
                contentItem: Rectangle {
                    implicitWidth: AppTheme.space(3)
                    radius: AppTheme.cornerRadius
                    color: AppTheme.muted
                }
                background: Rectangle { color: AppTheme.transparent }
            }

            delegate: Item {
                id: commandRow
                required property var modelData
                required property int index
                width: commandList.width - commandList.leftMargin - commandList.rightMargin
                height: root.rowHeight
                readonly property bool selected: index === root.selectedIndex

                Rectangle {
                    anchors.fill: parent
                    color: commandRow.selected
                        ? AppTheme.menuSelectedBackground
                        : (rowMouse.containsMouse ? AppTheme.hoverFill : AppTheme.transparent)
                    border.color: commandRow.selected
                        ? AppTheme.menuSelectedBorder : AppTheme.transparent
                    border.width: AppTheme.space(1)
                    radius: AppTheme.cornerRadius
                }

                Rectangle {
                    id: iconFrame
                    anchors.left: parent.left
                    anchors.leftMargin: AppTheme.space(9)
                    anchors.verticalCenter: parent.verticalCenter
                    width: AppTheme.space(36)
                    height: width
                    color: commandRow.selected ? AppTheme.focusFill : AppTheme.normalFill
                    border.color: commandRow.selected ? AppTheme.focusBorder : AppTheme.normalBorder
                    border.width: AppTheme.space(1)
                    radius: AppTheme.cornerRadius

                    CommandIcon {
                        anchors.centerIn: parent
                        width: AppTheme.space(24)
                        height: width
                        iconName: commandRow.modelData.icon
                        iconColor: commandRow.selected
                            ? AppTheme.menuSelectedText : AppTheme.menuText
                    }
                }

                Column {
                    anchors.left: iconFrame.right
                    anchors.leftMargin: AppTheme.space(11)
                    anchors.right: parent.right
                    anchors.rightMargin: AppTheme.space(10)
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: AppTheme.space(2)

                    Text {
                        width: parent.width
                        text: commandRow.modelData.title
                        color: commandRow.selected
                            ? AppTheme.menuSelectedText : AppTheme.menuText
                        elide: Text.ElideRight
                        font.family: "iA Writer Mono S"
                        font.pixelSize: AppTheme.space(13)
                        font.weight: commandRow.selected ? Font.DemiBold : Font.Normal
                    }

                    Text {
                        width: parent.width
                        text: commandRow.modelData.description
                        color: AppTheme.menuText
                        opacity: 0.52
                        elide: Text.ElideRight
                        font.family: "iA Writer Mono S"
                        font.pixelSize: AppTheme.space(10)
                    }
                }

                MouseArea {
                    id: rowMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onEntered: root.selectedIndex = commandRow.index
                    onClicked: root.commandTriggered(commandRow.modelData.id)
                }
            }

            Text {
                anchors.centerIn: parent
                visible: root.filteredCommands.length === 0
                text: "No matching command"
                color: AppTheme.menuText
                opacity: 0.58
                font.family: "iA Writer Mono S"
                font.pixelSize: AppTheme.space(12)
            }
        }
    }
}
