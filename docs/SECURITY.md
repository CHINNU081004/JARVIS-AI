# JARVIS Security & Privacy Policy

## 1. Local-First & Privacy Principles
- **No Silent Cloud Uploads:** No local files, memories, credentials, or prompts are sent to external/remote services without explicit user-configured endpoints and selection.
- **Ollama Security Notice:** Remote Ollama endpoints should use authenticated reverse proxies, VPNs, or SSH tunnels. Never expose an unauthenticated Ollama service directly to the public internet.
- **Credential Storage:** Secrets and API keys are stored in private configurations and redacted from logging, diagnostics, and GUI error popups.

## 2. Tool & Workspace Sandboxing
- **Directory Traversal Protection:** All file paths are strictly resolved and canonicalized against the approved workspace root.
- **Symlink Boundary Checks:** Symlinks pointing outside the designated workspace are blocked.
- **Read-Only Inspection in Phase 1:** Phase 1 permits safe browsing, file reading up to bounded limits, and text searching. Destructive write operations and arbitrary shell execution require Phase 2 explicit confirmation gates.
