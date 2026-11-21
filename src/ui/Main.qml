import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

ApplicationWindow {
    id: window
    width: 1280
    height: 768
    minimumWidth: 1024
    minimumHeight: 640
    visible: true
    title: "JARVIS - AI Desktop Assistant"
    color: "#050914"

    // Top Navigation & Provider Bar
    Rectangle {
        id: header
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 60
        color: "#091228"
        border.color: "#16284f"
        border.width: 1

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 20
            anchors.rightMargin: 20
            spacing: 16

            // Bot Avatar & Title
            Rectangle {
                width: 36; height: 36
                radius: 18
                color: "#0e204c"
                border.color: "#00f0ff"
                border.width: 1

                Text {
                    anchors.centerIn: parent
                    text: "🤖"
                    font.pixelSize: 18
                }
            }

            ColumnLayout {
                spacing: 2
                Text {
                    text: "JARVIS Lite"
                    color: "#00f0ff"
                    font.pixelSize: 16
                    font.bold: true
                }
                Text {
                    text: "Your Local & Cloud AI Assistant"
                    color: "#94a3b8"
                    font.pixelSize: 11
                }
            }

            Item { Layout.fillWidth: true }

            // Provider Connection Pill
            Rectangle {
                width: 180; height: 32
                radius: 16
                color: "#0c1b3d"
                border.color: jarvis.isProviderConnected ? "#10b981" : "#f59e0b"
                border.width: 1

                RowLayout {
                    anchors.centerIn: parent
                    spacing: 8
                    Rectangle {
                        width: 8; height: 8; radius: 4
                        color: jarvis.isProviderConnected ? "#10b981" : "#f59e0b"
                    }
                    Text {
                        text: jarvis.selectedProviderId === "ollama_local" ? "Local Ollama" : (jarvis.selectedProviderId === "ollama_remote" ? "Remote Ollama" : "OpenAI API")
                        color: "#f8fafc"
                        font.pixelSize: 12
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: jarvis.testActiveProvider()
                }
            }

            // Model Selection Dropdown
            ComboBox {
                id: modelCombo
                model: jarvis.getDiscoveredModels()
                textRole: "name"
                implicitWidth: 190
                implicitHeight: 32
                onActivated: {
                    var sel = model.get ? model.get(currentIndex).id : model[currentIndex].id;
                    jarvis.selectModel(sel);
                }
            }

            Button {
                text: "Refresh"
                implicitHeight: 32
                onClicked: jarvis.refreshModels()
            }
        }
    }

    // Main Content Area
    RowLayout {
        anchors.top: header.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: 12
        anchors.margins: 12

        // Left Sidebar Navigation
        GlassCard {
            Layout.preferredWidth: 200
            Layout.fillHeight: true

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 8

                Repeater {
                    model: ["Chat", "Workspace", "Memory", "Settings"]
                    delegate: Rectangle {
                        Layout.fillWidth: true
                        height: 40
                        radius: 8
                        color: activeTab === modelData ? "#1e3a8a" : "transparent"
                        border.color: activeTab === modelData ? "#00f0ff" : "transparent"

                        Text {
                            anchors.left: parent.left
                            anchors.leftMargin: 12
                            anchors.verticalCenter: parent.verticalCenter
                            text: modelData
                            color: activeTab === modelData ? "#00f0ff" : "#94a3b8"
                            font.pixelSize: 13
                            font.bold: activeTab === modelData
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: activeTab = modelData
                        }
                    }
                }

                Item { Layout.fillHeight: true }

                // UI Controls
                Text {
                    text: "Visual Intensity"
                    color: "#94a3b8"
                    font.pixelSize: 11
                }
                Slider {
                    Layout.fillWidth: true
                    from: 0; to: 2; stepSize: 1
                    value: jarvis.animationIntensity
                    onMoved: jarvis.animationIntensity = value
                }
            }
        }

        // Center View
        GlassCard {
            Layout.fillWidth: true
            Layout.fillHeight: true

            // Chat View
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 14
                spacing: 10
                visible: activeTab === "Chat"

                // Holographic Avatar Header
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 16
                    HologramGlobe {
                        width: 70; height: 70
                        animate: jarvis.animationIntensity > 0
                    }
                    ColumnLayout {
                        Text {
                            text: "JARVIS Neural Console"
                            color: "#00f0ff"
                            font.pixelSize: 18
                            font.bold: true
                        }
                        Text {
                            text: jarvis.activeTaskDescription + " • " + jarvis.activeTaskStep
                            color: "#94a3b8"
                            font.pixelSize: 12
                        }
                    }
                    Item { Layout.fillWidth: true }
                    Button {
                        text: "Stop"
                        visible: jarvis.isGenerating
                        onClicked: jarvis.cancelGeneration()
                    }
                }

                // Chat Messages List
                ListView {
                    id: chatView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: jarvis.chatModel
                    spacing: 10

                    delegate: Rectangle {
                        width: chatView.width
                        implicitHeight: col.implicitHeight + 20
                        radius: 10
                        color: model.role === "user" ? "#14254f" : "#0a1738"
                        border.color: model.role === "user" ? "#1e40af" : "#00f0ff"
                        border.width: 1

                        ColumnLayout {
                            id: col
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 4

                            RowLayout {
                                Text {
                                    text: model.role === "user" ? "You" : "JARVIS"
                                    color: model.role === "user" ? "#60a5fa" : "#00f0ff"
                                    font.bold: true
                                    font.pixelSize: 12
                                }
                                Item { Layout.fillWidth: true }
                                Text {
                                    text: model.timestamp
                                    color: "#64748b"
                                    font.pixelSize: 10
                                }
                            }

                            TextEdit {
                                Layout.fillWidth: true
                                text: model.content
                                color: "#f8fafc"
                                wrapMode: TextEdit.Wrap
                                readOnly: true
                                selectByMouse: true
                                font.pixelSize: 13
                            }

                            // Telemetry metrics
                            RowLayout {
                                visible: model.role === "assistant" && model.tpsStr !== ""
                                spacing: 12
                                Text {
                                    text: model.durationStr + " • " + model.tpsStr
                                    color: "#94a3b8"
                                    font.pixelSize: 10
                                }
                                Text {
                                    text: model.isExact ? "Exact Metrics" : "Estimated"
                                    color: model.isExact ? "#10b981" : "#f59e0b"
                                    font.pixelSize: 10
                                }
                            }
                        }
                    }
                }

                // Chat Input Box
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    TextField {
                        id: inputField
                        Layout.fillWidth: true
                        placeholderText: "Type your instruction or message to JARVIS..."
                        color: "#f8fafc"
                        onAccepted: {
                            if (text.trim() !== "") {
                                jarvis.sendMessage(text);
                                text = "";
                            }
                        }
                    }

                    Button {
                        text: "Send"
                        enabled: !jarvis.isGenerating
                        onClicked: {
                            if (inputField.text.trim() !== "") {
                                jarvis.sendMessage(inputField.text);
                                inputField.text = "";
                            }
                        }
                    }
                }
            }

            // Workspace View
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 14
                spacing: 10
                visible: activeTab === "Workspace"

                RowLayout {
                    TextField {
                        id: wsInput
                        Layout.fillWidth: true
                        text: jarvis.workspace.currentPath
                        placeholderText: "Workspace folder path (e.g. /home/chinnu/Desktop/JARVIS)..."
                    }
                    Button {
                        text: "Open Folder"
                        onClicked: jarvis.workspace.currentPath = wsInput.text
                    }
                }

                ListView {
                    id: fileListView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: jarvis.workspace.listFiles("")
                    delegate: Rectangle {
                        width: fileListView.width
                        height: 36
                        radius: 6
                        color: "#0f2048"
                        border.color: "#1e3a8a"
                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 8
                            Text {
                                text: modelData.isDirectory ? "📁 " + modelData.name : "📄 " + modelData.name
                                color: "#f8fafc"
                                font.pixelSize: 12
                            }
                            Item { Layout.fillWidth: true }
                            Text {
                                text: modelData.size + " bytes"
                                color: "#64748b"
                                font.pixelSize: 11
                            }
                        }
                    }
                }
            }

            // Memory View
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 14
                spacing: 10
                visible: activeTab === "Memory"

                Text {
                    text: "Persistent Bounded Memories"
                    color: "#00f0ff"
                    font.bold: true
                    font.pixelSize: 16
                }

                ListView {
                    id: memListView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: jarvis.getAllMemories()
                    delegate: Rectangle {
                        width: memListView.width
                        height: 48
                        radius: 6
                        color: "#0e1f44"
                        border.color: "#1d3875"
                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 8
                            ColumnLayout {
                                Text {
                                    text: "[" + modelData.category + "] " + modelData.key
                                    color: "#38bdf8"
                                    font.bold: true
                                    font.pixelSize: 12
                                }
                                Text {
                                    text: modelData.content
                                    color: "#cbd5e1"
                                    font.pixelSize: 11
                                }
                            }
                            Item { Layout.fillWidth: true }
                            Button {
                                text: "Delete"
                                onClicked: {
                                    jarvis.deleteMemory(modelData.id);
                                    memListView.model = jarvis.getAllMemories();
                                }
                            }
                        }
                    }
                }
            }

            // Settings View
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 14
                spacing: 12
                visible: activeTab === "Settings"

                Text {
                    text: "Provider Configuration"
                    color: "#00f0ff"
                    font.bold: true
                    font.pixelSize: 16
                }

                RowLayout {
                    Text { text: "Local Ollama Endpoint:"; color: "#94a3b8" }
                    TextField { id: localOllamaUrl; text: "http://127.0.0.1:11434"; Layout.fillWidth: true }
                }

                RowLayout {
                    Text { text: "Remote Ollama Endpoint:"; color: "#94a3b8" }
                    TextField { id: remoteOllamaUrl; text: "http://remote-gpu:11434"; Layout.fillWidth: true }
                    Button {
                        text: "Apply"
                        onClicked: jarvis.updateProviderEndpoint("ollama_remote", remoteOllamaUrl.text)
                    }
                }

                RowLayout {
                    Text { text: "OpenAI Compatible API:"; color: "#94a3b8" }
                    TextField { id: genericApiUrl; text: "https://api.openai.com/v1"; Layout.fillWidth: true }
                    TextField { id: genericApiKey; placeholderText: "API Key (Stored securely)"; echoMode: TextInput.Password; Layout.fillWidth: true }
                    Button {
                        text: "Apply"
                        onClicked: jarvis.updateProviderEndpoint("generic_api", genericApiUrl.text, genericApiKey.text)
                    }
                }

                Item { Layout.fillHeight: true }
            }
        }

        // Right Sidebar - System Monitor & Live Activity
        GlassCard {
            Layout.preferredWidth: 260
            Layout.fillHeight: true

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 10

                // System Monitor
                Text {
                    text: "System Monitor"
                    color: "#00f0ff"
                    font.bold: true
                    font.pixelSize: 13
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 70
                    radius: 8
                    color: "#0a1738"
                    border.color: "#1e3a8a"
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 8
                        Text {
                            text: "CPU Usage: " + jarvis.metrics.cpuUsagePercent + "%"
                            color: "#38bdf8"
                            font.pixelSize: 11
                        }
                        Text {
                            text: "RAM Used: " + jarvis.metrics.ramUsageGb.toFixed(2) + " GB"
                            color: "#34d399"
                            font.pixelSize: 11
                        }
                        Text {
                            text: "Network: " + jarvis.metrics.networkKbps.toFixed(1) + " KB/s"
                            color: "#a78bfa"
                            font.pixelSize: 11
                        }
                    }
                }

                // Live Activity
                Text {
                    text: "Live Activity Timeline"
                    color: "#00f0ff"
                    font.bold: true
                    font.pixelSize: 13
                }

                ListView {
                    id: actListView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: jarvis.activityModel
                    spacing: 6

                    delegate: Rectangle {
                        width: actListView.width
                        implicitHeight: actCol.implicitHeight + 12
                        radius: 6
                        color: "#0c1a3b"
                        border.color: model.level === "ERROR" ? "#ef4444" : "#1e3a8a"

                        ColumnLayout {
                            id: actCol
                            anchors.fill: parent
                            anchors.margins: 6
                            spacing: 2
                            RowLayout {
                                Text {
                                    text: model.title
                                    color: model.level === "ERROR" ? "#ef4444" : "#38bdf8"
                                    font.bold: true
                                    font.pixelSize: 11
                                }
                                Item { Layout.fillWidth: true }
                                Text {
                                    text: model.timestamp
                                    color: "#64748b"
                                    font.pixelSize: 9
                                }
                            }
                            Text {
                                text: model.detail
                                color: "#94a3b8"
                                font.pixelSize: 10
                                wrapMode: Text.Wrap
                                Layout.fillWidth: true
                            }
                        }
                    }
                }
            }
        }
    }

    property string activeTab: "Chat"
}
