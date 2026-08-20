pragma Singleton
import QtQuick

QtObject {
    readonly property color background: backend.themeBackground
    readonly property color foreground: backend.themeForeground
    readonly property color accent: backend.themeAccent
    readonly property color urgent: backend.themeUrgent
    readonly property color muted: backend.themeMuted
    readonly property color selection: backend.themeSelection
    readonly property color transparent: backend.themeTransparent

    readonly property color popupBackground: backend.themePopupBackground
    readonly property color popupText: backend.themePopupText
    readonly property color popupBorder: backend.themePopupBorder

    readonly property color menuBackground: backend.themeMenuBackground
    readonly property color menuText: backend.themeMenuText
    readonly property color menuBorder: backend.themeMenuBorder
    readonly property color menuSelectedBackground: backend.themeMenuSelectedBackground
    readonly property color menuSelectedText: backend.themeMenuSelectedText
    readonly property color menuSelectedBorder: backend.themeMenuSelectedBorder

    readonly property color normalFill: backend.themeNormalFill
    readonly property color hoverFill: backend.themeHoverFill
    readonly property color pressedFill: backend.themePressedFill
    readonly property color focusFill: backend.themeFocusFill
    readonly property color normalBorder: backend.themeNormalBorder
    readonly property color hoverBorder: backend.themeHoverBorder
    readonly property color focusBorder: backend.themeFocusBorder

    readonly property real scale: backend.textScale * backend.themeSpacingScale
    readonly property int cornerRadius: backend.themeCornerRadius

    function space(pixels) {
        return Math.max(1, Math.round(pixels * scale));
    }

    function controlFill(focused, hovered, pressed) {
        if (pressed)
            return pressedFill;
        if (focused)
            return focusFill;
        if (hovered)
            return hoverFill;
        return normalFill;
    }

    function controlBorder(focused, hovered) {
        if (focused)
            return focusBorder;
        if (hovered)
            return hoverBorder;
        return normalBorder;
    }
}
