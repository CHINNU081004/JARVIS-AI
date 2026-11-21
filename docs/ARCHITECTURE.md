# JARVIS Architecture Document

## 1. System Overview
**JARVIS** is a native, lightweight C++20 and Qt 6 desktop AI assistant. It provides a futuristic glassmorphic/holographic user interface, model-agnostic provider connectivity (local/remote Ollama and standard HTTP API endpoints), dynamic model discovery, streaming inference, truthful live token and performance telemetry, bounded SQLite-backed relevant memory, and safe local workspace browsing.

```
+------------------------------------------------------------------------+
|                          JARVIS QML GUI                                |
|  - Holographic Canvas   - Chat & Code Views   - Live Activity Timeline |
|  - System & Token Meters- Provider Config     - Workspace Explorer     |
+------------------------------------+-----------------------------------+
                                     | Q_PROPERTY / Q_INVOKABLE
+------------------------------------v-----------------------------------+
|                        Core Controller & Models                        |
|  - JarvisController   - ChatModel             - ActivityLogger         |
|  - MetricsCollector   - RoutingEngine         - MemoryController       |
|  - WorkspaceManager   - SettingsManager                                |
+--------+------------------+------------------+-----------------+-------+
         |                  |                  |                 |
         v                  v                  v                 v
+----------------+  +---------------+  +---------------+  +--------------+
| Provider Layer |  | Storage Layer |  | Workspace Tool|  | Security     |
| - IProvider    |  | - SQLite DB   |  | - Path Guard  |  | - Redaction  |
| - OllamaAdapter|  | - Chat History|  | - Read/Search |  | - Command    |
| - GenericApi   |  | - Bounded Mem |  | - Safe Browsing| |   Policy     |
+----------------+  +---------------+  +---------------+  +--------------+
```

## 2. Core Modules
- **`core/`**: Central application state, controllers, metrics tracking, and event routing.
- **`providers/`**: Abstract `IProvider` interface, `OllamaAdapter`, and `GenericApiAdapter`. Supports streaming chunks, model metadata querying, connection testing, and cancellation.
- **`storage/`**: Thread-safe SQLite engine managing conversation sessions, message history, settings, and keyword/recency-indexed memories.
- **`workspace/`**: Safe sandbox file-system manager with path normalization, symlink boundary checks, and restricted file reading/text searching.
- **`ui/`**: QML components, shaders/effects, styling constants, holographic widgets, and icons.
- **`tests/`**: Unit and integration test suites using Qt Test with mock HTTP endpoints.
