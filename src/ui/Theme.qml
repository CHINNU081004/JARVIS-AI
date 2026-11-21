pragma Singleton
import QtQuick

QtObject {
    // Holographic Cyberpunk Palette
    readonly property color backgroundDark: "#060b19"
    readonly property color backgroundSurface: "#0c1833"
    readonly property color glassCard: "#11224d"
    readonly property color glassCardBorder: "#1e3a8a"
    
    readonly property color neonCyan: "#00f0ff"
    readonly property color neonBlue: "#0088ff"
    readonly property color neonTeal: "#00d2aa"
    readonly property color glowAccent: "#00e5ff"
    
    readonly property color textPrimary: "#f8fafc"
    readonly property color textSecondary: "#94a3b8"
    readonly property color textMuted: "#64748b"
    
    readonly property color statusGreen: "#10b981"
    readonly property color statusWarning: "#f59e0b"
    readonly property color statusError: "#ef4444"
    
    readonly property font titleFont: Qt.font({ family: "Sans Serif", pixelSize: 18, bold: true })
    readonly property font bodyFont: Qt.font({ family: "Sans Serif", pixelSize: 13, bold: false })
    readonly property font monoFont: Qt.font({ family: "Monospace", pixelSize: 12, bold: false })
    readonly property font smallFont: Qt.font({ family: "Sans Serif", pixelSize: 11, bold: false })
}
