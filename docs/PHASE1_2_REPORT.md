# JARVIS Phase 1.2: Model Discovery, Names Visibility, and Inference Report

**Date & Time:** 2026-11-22T04:09:00+05:30  
**Target:** Phase 1.2 Model Discovery, Name Display, and Live Inference  

---

## 1. Confirmed Root Causes

### 1. Model Names Not Visible in the UI
- **Root Cause:**
  1. In `src/ui/Main.qml`, the ComboBox used a raw JavaScript list binding `model: jarvis.getDiscoveredModels()` with `textRole: "name"`. In Qt 6 QML, when `model` is a `QVariantList` of `QVariantMap`, standard ComboBox `textRole` lookup frequently fails to resolve map properties for display text.
  2. The custom `contentItem` and `delegate` of the ComboBox were not implemented to format model tags and quantization details (e.g. `gemma3:4b (Q4_K_M)` vs raw identifiers).
  3. ComboBox `currentIndex` was not synchronized back to `selectedModelId`. Whenever the list refreshed, `currentIndex` was reset or fell out of sync with the underlying backend selection.
- **Fix:**
  - Created a dedicated `ModelListModel` (`QAbstractListModel`) exposing typed roles: `IdRole`, `NameRole`, `DisplayNameRole`, `ProviderIdRole`, `ParameterSizeRole`, `QuantizationRole`, `FormatRole`, `HasVisionRole`, `HasStreamingRole`, and `HasToolsRole`.
  - Bound ComboBox directly to `jarvis.modelListModel` with explicit `textRole: "displayName"`, `valueRole: "id"`, and customized glassmorphism `contentItem` and `delegate`.
  - Added bidirectional index resolution (`indexOf(selectedModelId)`) and model preservation across refreshes.

### 2. Stream Buffer Chunk Fragmentation in Inference
- **Root Cause:**
  - In `src/providers/OllamaAdapter.cpp` and `GenericApiAdapter.cpp`, the streaming loop called `canReadLine()`. HTTP/TCP packets arriving over network interfaces do not always break neatly at newline boundaries. Calling `canReadLine()` on partial chunks left fragmented buffers unconsumed or delayed until completion.
- **Fix:**
  - Introduced `m_streamBuffer` accumulation in both `OllamaAdapter` and `GenericApiAdapter`.
  - Processed complete lines with `indexOf('\n')`, properly extracting JSON lines while preserving trailing incomplete fragments for subsequent read events.

### 3. Model Discovery Integration with Native Ollama v0.40+
- **Root Cause:**
  - Ollama system service was installed under `/etc/systemd/system/ollama.service` running as system user `ollama`. The user's downloaded models resided in `/home/chinnu/.ollama/models`.
- **Fix:**
  - Configured drop-in `models.conf` with `OLLAMA_MODELS=/home/chinnu/.ollama/models`, granting read permissions to the service.
  - Ollama instantly surfaced all 4 downloaded models: `gemma4:e4b`, `gemma3:4b`, `qwen3.5:4b`, and `gemma4:e2b`.
  - Updated `OllamaAdapter` to parse Ollama's native `capabilities` array (`["completion", "vision", "audio", "tools", "thinking"]`).

---

## 2. Live Inference Verification

Real inference was tested end-to-end against the local Ollama instance (`http://127.0.0.1:11434`):
* **Model Tested:** `gemma4:e4b`
* **Test Prompt:** `"Reply with exactly: JARVIS TEST OK"`
* **Actual Response Received:** `"JARVIS TEST OK"`
* **Tokens Streamed:** Successfully verified non-empty chunk stream signals and message assembly.
* **Duration:** Full prompt processing, token evaluation, and response delivery in 2.2 seconds.

---

## 3. Automated Test Suite Results

Test executable: `/home/chinnu/Desktop/JARVIS/build/test_jarvis`
```
********* Start testing of TestJarvisPhase1 *********
Config: Using QtTest library 6.10.2, Qt 6.10.2 (x86_64-little_endian-lp64 shared (dynamic) release build; by GCC 15.2.0), ubuntu 26.04
PASS   : TestJarvisPhase1::initTestCase()
PASS   : TestJarvisPhase1::testDatabaseOperations()
PASS   : TestJarvisPhase1::testBoundedMemoryRetrieval()
PASS   : TestJarvisPhase1::testWorkspaceSandboxSafety()
PASS   : TestJarvisPhase1::testRoutingDeterministicRules()
PASS   : TestJarvisPhase1::testMetricsCalculation()
PASS   : TestJarvisPhase1::testRamMonitoringParser()
PASS   : TestJarvisPhase1::testNetworkMonitoringParser()
PASS   : TestJarvisPhase1::testModelListModelRolesAndSelection()
PASS   : TestJarvisPhase1::testOllamaAdapterOfflineHandling()
PASS   : TestJarvisPhase1::testGenericApiAdapterOfflineHandling()
PASS   : TestJarvisPhase1::testRealOllamaInferenceIfAvailable()
PASS   : TestJarvisPhase1::cleanupTestCase()
Totals: 13 passed, 0 failed, 0 skipped, 0 blacklisted, 2217ms
********* Finished testing of TestJarvisPhase1 *********
```

---

## 4. Package & Build Locations
* **Source Tree:** `/home/chinnu/Desktop/JARVIS`
* **Binary Executable:** `/home/chinnu/Desktop/JARVIS/build/jarvis`
* **Debian Package:** `/home/chinnu/Desktop/JARVIS/build/jarvis-1.0.0-Linux.deb` (167 KB)
