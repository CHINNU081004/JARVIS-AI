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

    // Ambient floating particles background (honors animation settings)
    ParticlesField {
        running: jarvis.animationIntensity === 2 && jarvis.particlesEnabled
        count: 28
    }

    // Top Navigation & Provider Bar
    Rectangle {
        id: header
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 64
        color: "#091228"
        border.color: "#16284f"
        border.width: 1

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 20
            anchors.rightMargin: 20
            spacing: 16

            // Native Bot Avatar & Title (Replacing emoji with Vector Icon)
            Rectangle {
                width: 40; height: 40
                radius: 20
                color: "#0e204c"
                border.color: "#00f0ff"
                border.width: 1.5

                VectorIcon {
                    anchors.centerIn: parent
                    name: "bot"
                    color: "#00f0ff"
                    width: 20; height: 20
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

            // Real-time Voice Waveform Visualizer
            VoiceWaveform {
                Layout.leftMargin: 20
                active: jarvis.isGenerating
                animationMode: jarvis.animationIntensity
            }

            Item { Layout.fillWidth: true }

            // Provider Connection Pill
            Rectangle {
                width: 180; height: 36
                radius: 18
                color: "#0c1b3d"
                border.color: jarvis.metrics.lastLatencyMs > 0 ? "#10b981" : "#f59e0b"
                border.width: 1.5

                RowLayout {
                    anchors.centerIn: parent
                    spacing: 8
                    Rectangle {
                        width: 8; height: 8; radius: 4
                        color: jarvis.metrics.lastLatencyMs > 0 ? "#10b981" : "#f59e0b"
                    }
                    Text {
                        text: {
                            if (jarvis.selectedProviderId === "ollama_local") return "Local Ollama";
                            if (jarvis.selectedProviderId === "ollama_remote") return "Remote Ollama";
                            return "API (" + jarvis.getProviderProtocol(jarvis.selectedProviderId) + ")";
                        }
                        color: "#f8fafc"
                        font.pixelSize: 12
                        font.bold: true
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: jarvis.testActiveProvider()
                }
            }

            // Polished Model Selection Dropdown using GlassComboBox
            GlassComboBox {
                id: modelCombo
                model: jarvis.modelListModel
                textRole: "displayName"
                valueRole: "id"
                implicitWidth: 230
                implicitHeight: 36

                currentIndex: jarvis.modelListModel.indexOf(jarvis.selectedModelId)

                onActivated: (index) => {
                    var selId = jarvis.modelListModel.idAt(index);
                    if (selId !== "") {
                        jarvis.selectModel(selId);
                    }
                }
            }

            GlassButton {
                text: "Refresh"
                isSecondary: true
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
                    model: [
                        { name: "Chat", icon: "chat" },
                        { name: "Workspace", icon: "folder" },
                        { name: "Memory", icon: "memory" },
                        { name: "Settings", icon: "settings" }
                    ]
                    delegate: Rectangle {
                        Layout.fillWidth: true
                        height: 42
                        radius: 12
                        color: activeTab === modelData.name ? "#1e3a8a" : "transparent"
                        border.color: activeTab === modelData.name ? "#00f0ff" : "transparent"
                        border.width: 1

                        Behavior on color { ColorAnimation { duration: 150 } }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 14
                            spacing: 12

                            VectorIcon {
                                name: modelData.icon
                                color: activeTab === modelData.name ? "#00f0ff" : "#94a3b8"
                                width: 18; height: 18
                            }

                            Text {
                                text: modelData.name
                                color: activeTab === modelData.name ? "#00f0ff" : "#94a3b8"
                                font.pixelSize: 13
                                font.bold: activeTab === modelData.name
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: activeTab = modelData.name
                        }
                    }
                }

                Item { Layout.fillHeight: true }

                // Quick session button
                GlassButton {
                    Layout.fillWidth: true
                    text: "+ New Chat"
                    onClicked: jarvis.createNewConversation("Chat " + Qt.formatDateTime(new Date(), "hh:mm"))
                }
            }
        }

        // Center View
        GlassCard {
            Layout.fillWidth: true
            Layout.fillHeight: true

            // 1. Chat View
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 14
                spacing: 10
                visible: activeTab === "Chat"

                // Holographic Center Avatar Header
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 16
                    HologramGlobe {
                        width: 90; height: 90
                        animationMode: jarvis.animationIntensity
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
                    GlassButton {
                        text: "Stop Generation"
                        isDanger: true
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
                        implicitHeight: col.implicitHeight + 24
                        radius: 14
                        color: model.role === "user" ? "#0c1f44" : "#07132a"
                        border.color: model.role === "user" ? "#1e40af" : "#00f0ff"
                        border.width: model.role === "user" ? 1 : 1.5

                        // Subtle upper glass reflection highlight
                        Rectangle {
                            anchors.top: parent.top
                            anchors.left: parent.left
                            anchors.right: parent.right
                            height: parent.height * 0.35
                            radius: 14
                            gradient: Gradient {
                                GradientStop { position: 0.0; color: Qt.rgba(1, 1, 1, model.role === "user" ? 0.04 : 0.07) }
                                GradientStop { position: 1.0; color: "transparent" }
                            }
                        }

                        ColumnLayout {
                            id: col
                            anchors.fill: parent
                            anchors.margins: 14
                            spacing: 8

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

                    GlassTextField {
                        id: inputField
                        Layout.fillWidth: true
                        placeholderText: "Type your instruction or message to JARVIS..."
                        leadingIcon: "chat"
                        trailingClearButton: true
                        onAccepted: {
                            if (text.trim() !== "") {
                                jarvis.sendMessage(text);
                                text = "";
                            }
                        }
                    }

                    GlassButton {
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

            // 2. Workspace View
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 14
                spacing: 10
                visible: activeTab === "Workspace"

                RowLayout {
                    GlassTextField {
                        id: wsInput
                        Layout.fillWidth: true
                        text: jarvis.workspace.currentPath
                        placeholderText: "Workspace folder path (e.g. /home/chinnu/Desktop/JARVIS)..."
                        leadingIcon: "folder"
                        trailingClearButton: true
                    }
                    GlassButton {
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
                        height: 38
                        radius: 8
                        color: "#0f2048"
                        border.color: "#1e3a8a"
                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 10
                            VectorIcon {
                                name: modelData.isDirectory ? "folder" : "chat"
                                color: "#00f0ff"
                                width: 14; height: 14
                            }
                            Text {
                                text: modelData.name
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

            // 3. Memory View
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 14
                spacing: 10
                visible: activeTab === "Memory"

                RowLayout {
                    Text {
                        text: "Persistent Bounded Memories"
                        color: "#00f0ff"
                        font.bold: true
                        font.pixelSize: 16
                    }
                    Item { Layout.fillWidth: true }
                    Text {
                        text: jarvis.memoryEnabled ? "Active" : "Disabled"
                        color: jarvis.memoryEnabled ? "#10b981" : "#f59e0b"
                        font.pixelSize: 12
                        font.bold: true
                    }
                }

                ListView {
                    id: memListView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: jarvis.getAllMemories()
                    delegate: Rectangle {
                        width: memListView.width
                        height: 52
                        radius: 8
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
                            GlassButton {
                                text: "Delete"
                                isDanger: true
                                onClicked: {
                                    jarvis.deleteMemory(modelData.id);
                                    memListView.model = jarvis.getAllMemories();
                                }
                            }
                        }
                    }
                }
            }

            // 4. Settings View (Phase 1.4 Glassmorphism Redesign & Configuration Expansion)
            ScrollView {
                anchors.fill: parent
                anchors.margins: 14
                visible: activeTab === "Settings"
                clip: true

                ColumnLayout {
                    width: parent.width - 12
                    spacing: 16

                    Text {
                        text: "JARVIS System Configuration"
                        color: "#00f0ff"
                        font.bold: true
                        font.pixelSize: 17
                    }

                    // 1. Visual & Theme Customization Card
                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: themeCol.implicitHeight + 24
                        radius: 14
                        color: "#07122a"
                        border.color: "#1e3a8a"

                        ColumnLayout {
                            id: themeCol
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 10

                            Text {
                                text: "Appearance & Effects"
                                color: "#00f0ff"
                                font.bold: true
                                font.pixelSize: 13
                            }

                            RowLayout {
                                Text { text: "UI Animation Mode:"; color: "#94a3b8"; Layout.preferredWidth: 170 }
                                GlassButton {
                                    text: "Full"
                                    isSecondary: jarvis.animationIntensity !== 2
                                    onClicked: jarvis.animationIntensity = 2
                                }
                                GlassButton {
                                    text: "Reduced"
                                    isSecondary: jarvis.animationIntensity !== 1
                                    onClicked: jarvis.animationIntensity = 1
                                }
                                GlassButton {
                                    text: "Off"
                                    isSecondary: jarvis.animationIntensity !== 0
                                    onClicked: jarvis.animationIntensity = 0
                                }
                            }

                            RowLayout {
                                Text { text: "Ambient Particles:"; color: "#94a3b8"; Layout.preferredWidth: 170 }
                                GlassCheckBox {
                                    checked: jarvis.particlesEnabled
                                    onToggled: jarvis.particlesEnabled = checked
                                }
                                Text {
                                    text: jarvis.particlesEnabled ? "Enabled (Holographic space)" : "Disabled"
                                    color: jarvis.particlesEnabled ? "#10b981" : "#64748b"
                                    font.pixelSize: 11
                                }
                            }

                            RowLayout {
                                Text { text: "Accent Theme Glow:"; color: "#94a3b8"; Layout.preferredWidth: 170 }
                                Repeater {
                                    model: [
                                        { name: "Cyan", color: "#00f0ff" },
                                        { name: "Cobalt", color: "#38bdf8" },
                                        { name: "Emerald", color: "#10b981" },
                                        { name: "Amber", color: "#f59e0b" },
                                        { name: "Purple", color: "#c084fc" }
                                    ]
                                    delegate: Rectangle {
                                        width: 26; height: 26
                                        radius: 13
                                        color: modelData.color
                                        border.color: jarvis.accentColor === modelData.color ? "#ffffff" : "transparent"
                                        border.width: 2
                                        MouseArea {
                                            anchors.fill: parent
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: jarvis.accentColor = modelData.color
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // 2. Generation & Inference Parameters Card
                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: genCol.implicitHeight + 24
                        radius: 14
                        color: "#07122a"
                        border.color: "#1e3a8a"

                        ColumnLayout {
                            id: genCol
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 10

                            Text {
                                text: "AI Generation Parameters"
                                color: "#00f0ff"
                                font.bold: true
                                font.pixelSize: 13
                            }

                            RowLayout {
                                Text { text: "Temperature (" + tempSlider.value.toFixed(2) + "):"; color: "#94a3b8"; Layout.preferredWidth: 170 }
                                GlassSlider {
                                    id: tempSlider
                                    Layout.fillWidth: true
                                    from: 0.0
                                    to: 1.5
                                    stepSize: 0.05
                                    value: jarvis.temperature
                                    onMoved: jarvis.temperature = value
                                }
                                Text {
                                    text: tempSlider.value <= 0.3 ? "Precise" : (tempSlider.value <= 0.8 ? "Balanced" : "Creative")
                                    color: "#38bdf8"
                                    font.pixelSize: 11
                                    Layout.preferredWidth: 60
                                }
                            }

                            RowLayout {
                                Text { text: "Max Generation Tokens:"; color: "#94a3b8"; Layout.preferredWidth: 170 }
                                GlassSpinBox {
                                    from: 256
                                    to: 16384
                                    stepSize: 256
                                    value: jarvis.maxTokens
                                    onValueModified: jarvis.maxTokens = value
                                }
                                Text { text: "tokens per request"; color: "#64748b"; font.pixelSize: 11 }
                            }

                            RowLayout {
                                Text { text: "Stream Responses:"; color: "#94a3b8"; Layout.preferredWidth: 170 }
                                GlassCheckBox {
                                    checked: jarvis.streamEnabled
                                    onToggled: jarvis.streamEnabled = checked
                                }
                                Text {
                                    text: jarvis.streamEnabled ? "Real-time progressive token streaming" : "Wait for full completion"
                                    color: jarvis.streamEnabled ? "#10b981" : "#64748b"
                                    font.pixelSize: 11
                                }
                            }
                        }
                    }

                    // 3. Persistent Memory & Context Management Card
                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: memCol.implicitHeight + 24
                        radius: 14
                        color: "#07122a"
                        border.color: "#1e3a8a"

                        ColumnLayout {
                            id: memCol
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 10

                            Text {
                                text: "Persistent Memory & Privacy"
                                color: "#00f0ff"
                                font.bold: true
                                font.pixelSize: 13
                            }

                            RowLayout {
                                Text { text: "Persistent Context Retrieval:"; color: "#94a3b8"; Layout.preferredWidth: 170 }
                                GlassCheckBox {
                                    checked: jarvis.memoryEnabled
                                    onToggled: jarvis.memoryEnabled = checked
                                }
                                Text {
                                    text: jarvis.memoryEnabled ? "Active (SQLite encrypted store)" : "Disabled"
                                    color: jarvis.memoryEnabled ? "#10b981" : "#f59e0b"
                                    font.pixelSize: 11
                                }
                            }

                            RowLayout {
                                Text { text: "Max Injected Memories:"; color: "#94a3b8"; Layout.preferredWidth: 170 }
                                GlassSpinBox {
                                    from: 1; to: 10
                                    value: jarvis.maxMemoryRetrievalCount
                                    onValueModified: jarvis.maxMemoryRetrievalCount = value
                                }
                                Text { text: "top relevant items"; color: "#64748b"; font.pixelSize: 11 }
                            }
                        }
                    }

                    // 4. Provider Endpoints & Universal API Protocol Configuration
                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: provCol.implicitHeight + 24
                        radius: 14
                        color: "#07122a"
                        border.color: "#1e3a8a"

                        ColumnLayout {
                            id: provCol
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 12

                            Text {
                                text: "AI Model Providers & Cloud Endpoints"
                                color: "#00f0ff"
                                font.bold: true
                                font.pixelSize: 13
                            }

                            RowLayout {
                                Text { text: "Active Provider:"; color: "#94a3b8"; Layout.preferredWidth: 170 }
                                GlassButton {
                                    text: "Local Ollama"
                                    isSecondary: jarvis.selectedProviderId !== "ollama_local"
                                    onClicked: jarvis.selectProvider("ollama_local")
                                }
                                GlassButton {
                                    text: "Remote Ollama"
                                    isSecondary: jarvis.selectedProviderId !== "ollama_remote"
                                    onClicked: jarvis.selectProvider("ollama_remote")
                                }
                                GlassButton {
                                    text: "API"
                                    isSecondary: jarvis.selectedProviderId !== "universal_api" && jarvis.selectedProviderId !== "generic_api"
                                    onClicked: jarvis.selectProvider("universal_api")
                                }
                            }

                            // Remote Ollama Config
                            RowLayout {
                                Text { text: "Remote Ollama Endpoint:"; color: "#94a3b8"; Layout.preferredWidth: 170 }
                                GlassTextField {
                                    id: remoteOllamaUrl
                                    text: "http://remote-gpu:11434"
                                    Layout.fillWidth: true
                                }
                                GlassButton {
                                    text: "Save & Connect"
                                    onClicked: jarvis.updateProviderEndpoint("ollama_remote", remoteOllamaUrl.text)
                                }
                            }

                            // Universal API Configuration Block
                            Rectangle {
                                Layout.fillWidth: true
                                implicitHeight: apiSubCol.implicitHeight + 20
                                radius: 12
                                color: "#091530"
                                border.color: "#1e3a8a"

                                ColumnLayout {
                                    id: apiSubCol
                                    anchors.fill: parent
                                    anchors.margins: 10
                                    spacing: 8

                                    RowLayout {
                                        Text { text: "API Protocol:"; color: "#38bdf8"; font.bold: true; Layout.preferredWidth: 160 }
                                        GlassComboBox {
                                            id: apiProtoCombo
                                            model: ["OpenAI-Compatible", "Anthropic", "Google Gemini", "Custom HTTP"]
                                            currentIndex: {
                                                var curr = jarvis.getProviderProtocol("universal_api");
                                                var idx = model.indexOf(curr);
                                                return idx >= 0 ? idx : 0;
                                            }
                                            implicitWidth: 200
                                            implicitHeight: 34
                                        }
                                    }

                                    RowLayout {
                                        Text { text: "API Base Endpoint:"; color: "#94a3b8"; Layout.preferredWidth: 160 }
                                        GlassTextField {
                                            id: universalApiUrl
                                            text: {
                                                if (apiProtoCombo.currentText === "Anthropic") return "https://api.anthropic.com";
                                                if (apiProtoCombo.currentText === "Google Gemini") return "https://generativelanguage.googleapis.com";
                                                return "https://api.openai.com/v1";
                                            }
                                            Layout.fillWidth: true
                                        }
                                    }

                                    RowLayout {
                                        Text { text: "API Authentication Key:"; color: "#94a3b8"; Layout.preferredWidth: 160 }
                                        GlassTextField {
                                            id: universalApiKey
                                            placeholderText: "API Key (Redacted in telemetry logs)"
                                            echoMode: TextInput.Password
                                            Layout.fillWidth: true
                                        }
                                        GlassButton {
                                            text: "Save & Connect"
                                            onClicked: jarvis.updateProviderEndpoint("universal_api", universalApiUrl.text, universalApiKey.text, apiProtoCombo.currentText)
                                        }
                                    }
                                }
                            }
                        }
                    }

                    Item { Layout.fillHeight: true; height: 20 }
                }
            }
        }

        // Right Sidebar - System Monitor, GPU Card & Live Activity
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
                    implicitHeight: sysCol.implicitHeight + 16
                    radius: 12
                    color: "#0a1738"
                    border.color: "#1e3a8a"
                    ColumnLayout {
                        id: sysCol
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 6
                        Text {
                            text: "CPU: " + jarvis.metrics.cpuUsagePercent + "%"
                            color: "#38bdf8"
                            font.pixelSize: 11
                            font.bold: true
                        }
                        Text {
                            text: "RAM: " + jarvis.metrics.ramDisplayStr
                            color: "#34d399"
                            font.pixelSize: 11
                            font.bold: true
                        }
                        Text {
                            text: "Network: " + jarvis.metrics.networkDisplayStr
                            color: "#a78bfa"
                            font.pixelSize: 11
                            font.bold: true
                        }
                        Text {
                            text: "Interface: " + jarvis.metrics.activeInterface
                            color: "#94a3b8"
                            font.pixelSize: 10
                        }

                        // Local & Cloud GPU Inference Telemetry
                        Rectangle {
                            Layout.fillWidth: true
                            height: 1
                            color: "#1e3a8a"
                        }
                        Text {
                            text: "Local GPU: " + jarvis.metrics.gpuName
                            color: "#38bdf8"
                            font.pixelSize: 10
                            font.bold: true
                        }
                        Text {
                            text: jarvis.metrics.gpuTypeStr
                            color: "#64748b"
                            font.pixelSize: 9
                        }
                        Text {
                            text: "Remote GPU Inference: " + (jarvis.selectedProviderId === "ollama_remote" || jarvis.selectedProviderId === "universal_api" ? "Active Cloud Endpoint" : "Not In Use (Local Mode)")
                            color: (jarvis.selectedProviderId === "ollama_remote" || jarvis.selectedProviderId === "universal_api") ? "#10b981" : "#94a3b8"
                            font.pixelSize: 10
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
                        radius: 8
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
