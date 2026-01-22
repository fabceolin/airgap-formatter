import QtQuick

QtObject {
    // App version
    readonly property string appVersion: "0.1.0"

    // Background colors
    readonly property color background: "#1e1e1e"
    readonly property color backgroundSecondary: "#252526"
    readonly property color backgroundTertiary: "#2d2d2d"

    // Text colors
    readonly property color textPrimary: "#d4d4d4"
    readonly property color textSecondary: "#808080"
    readonly property color textError: "#f44747"
    readonly property color textSuccess: "#4ec9b0"

    // Accent colors
    readonly property color accent: "#0078d4"
    readonly property color border: "#3c3c3c"
    readonly property color splitHandle: "#505050"

    // Focus indicators (accessibility)
    readonly property color focusRing: "#0078d4"
    readonly property int focusRingWidth: 2

    // Typography
    readonly property string monoFont: "Consolas, Monaco, 'Courier New', monospace"
    readonly property int monoFontSize: 14

    // Syntax highlighting colors (base16-ocean.dark theme)
    readonly property color syntaxKey: "#8fa1b3"         // Light blue
    readonly property color syntaxString: "#a3be8c"      // Green
    readonly property color syntaxNumber: "#d08770"      // Orange
    readonly property color syntaxBoolean: "#b48ead"     // Purple
    readonly property color syntaxNull: "#bf616a"        // Red
    readonly property color syntaxPunctuation: "#c0c5ce" // Light gray
    readonly property color syntaxBadge: "#65737e"       // Muted for count badges
}
