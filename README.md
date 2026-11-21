# JARVIS — Native AI Desktop Assistant

JARVIS is a native C++20 and Qt 6 desktop AI assistant featuring a futuristic holographic glassmorphism interface, model-agnostic provider connectivity (local Ollama, remote Ollama, and OpenAI-compatible HTTP APIs), dynamic model discovery, streaming chat, truthful live token telemetry, persistent SQLite-backed bounded memory, and safe local workspace browsing.

![JARVIS Reference Design](reference%20image.png)

---

## Key Features in Phase 1 Release

* **Futuristic Holographic Interface:** Glassmorphism panels, glowing neon cyan/blue highlights, animated orbital hologram globe, and responsive layout.
* **Model-Agnostic AI Connectivity:**
  * Local Ollama (`http://127.0.0.1:11434`)
  * Remote/Cloud Ollama with custom endpoints
  * OpenAI-compatible HTTP API endpoints
* **Dynamic Model Discovery:** Discovers models directly from running servers without hardcoded model lists or code edits.
* **Deterministic Rule-Based Routing:** Manual model override support, automatic fallback, and capability routing (e.g. vision or coding specialization).
* **Truthful Telemetry:** Real-time tokens/second, duration, latency, Linux CPU usage, RAM consumption, and network throughput.
* **Persistent SQLite Storage:** Conversations, chat history, and bounded relevant memories persisted across restarts.
* **Safe Local Workspace:** Sandboxed project folder explorer with directory traversal safeguards, file content reading, and in-project text search.
* **Debian Packaging:** Native `.deb` package for Ubuntu x86-64.

---

## Building and Running from Source

### Prerequisites
Ubuntu 24.04+ or 26.04 x86_64:
```bash
sudo apt update
sudo apt install -y build-essential cmake ninja-build qt6-base-dev qt6-declarative-dev \
    qml6-module-qtquick qml6-module-qtquick-controls qml6-module-qtquick-layouts libgl1-mesa-dev
```

### Build Commands
```bash
mkdir -p build && cd build
cmake .. -GNinja -DCMAKE_BUILD_TYPE=Release
ninja
```

### Run Tests
```bash
./test_jarvis
```

### Run Application
```bash
./jarvis
```

---

## Debian Package Installation

To build the `.deb` package:
```bash
cd build
cpack -G DEB
```

To install the package:
```bash
sudo dpkg -i jarvis-1.0.0-Linux.deb
```

To uninstall:
```bash
sudo apt remove jarvis
```
