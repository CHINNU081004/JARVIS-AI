# Changelog

All notable changes to JARVIS will be documented in this file.

## [1.0.0] - Phase 1 Release (2026-11-22)

### Added
- **Core Architecture:**
  - Modern C++20 and Qt 6 architecture with separation of UI, providers, storage, metrics, router, and workspace.
  - Native SQLite storage engine managing conversation sessions, chat history, activity logs, and bounded memories.
  - System metrics sampler gathering real Linux CPU usage, RAM consumption, and network rate.
- **AI Providers:**
  - Asynchronous `IProvider` interface with streaming and request cancellation.
  - `OllamaAdapter` for local and remote Ollama instances with dynamic model tags discovery.
  - `GenericApiAdapter` for OpenAI-compatible chat completion APIs with streaming SSE parser.
  - Deterministic `RoutingEngine` with manual override, vision routing, and coding task matching.
- **Holographic Glassmorphic UI:**
  - Cyberpunk dark palette, glass cards, and glowing borders inspired by the reference design.
  - Animated `HologramGlobe` avatar with adjustable animation intensity settings.
  - Sidebar navigation supporting Chat, Workspace, Memory, and Settings tabs.
  - Live activity timeline tracking background system events.
- **Workspace Tools:**
  - `WorkspaceManager` with directory traversal protection, symlink boundary checks, safe file inspection, and text search.
- **Testing & Packaging:**
  - Qt Test verification test suite (`test_jarvis`) covering database operations, memory retrieval, sandbox boundaries, deterministic routing, metrics calculation, and offline resilience.
  - Automated Debian `.deb` package generation via CPack.
