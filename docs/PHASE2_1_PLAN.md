# JARVIS Phase 2.1 Implementation Plan: Agent Engine, Safe Workspace Operations, and Tool Execution

## 1. Baseline Status
- **Stack:** C++20, Qt 6.10.2, QML, CMake/Ninja, Ubuntu Linux.
- **Current Baseline:** Phase 1, Phase 1.1–1.4 completed and all 14 unit/integration tests passing.
- **Clean Workspace:** No uncommitted changes, Git repository synced with `origin/main`.

---

## 2. Architecture & Component Design

### A. Core Agent Types (`src/core/Types.h`)
- `AgentTaskStatus`: `Idle`, `Planning`, `AwaitingApproval`, `ExecutingTool`, `Completed`, `Failed`, `Cancelled`.
- `ToolPermission`: `ReadOnly`, `RequiresApproval`, `DestructiveConfirm`.
- `ToolExecutionRequest`: `id`, `toolName`, `arguments` (QJsonObject), `reason`, `diffPreview`, `permission`.
- `ToolExecutionResult`: `success`, `output` (QString), `errorMessage`, `modifiedFiles` (QStringList), `backupIds` (QStringList).
- `AgentTask`: `id`, `userGoal`, `status`, `stepIndex`, `maxSteps`, `startTime`, `endTime`, `filesRead`, `filesChanged`, `toolCalls`.

### B. Tool System (`src/agent/ITool.h` & `src/agent/ToolRegistry.h`)
- `ITool`: Interface defining `name()`, `description()`, `parameterSchema()`, `permission()`, `validate()`, `execute()`, `cancel()`.
- `ToolRegistry`: Discovers, registers, validates, and manages execution lifecycle with timeouts and sandbox boundaries.
- **Workspace Tools:**
  1. `list_workspace_files`: Lists directory contents safely.
  2. `read_workspace_file`: Reads text files with bounded byte limits.
  3. `search_workspace_files`: Content substring/grep across text files.
  4. `create_file`: Safe creation within authorized workspace (requires approval).
  5. `edit_file`: Generates unified diff, creates pre-modification backup, applies edit upon approval.
  6. `move_file`: Renames or relocates files within workspace.
  7. `delete_file`: Destructive operation (always requires explicit confirmation) with automatic recovery backup.
  8. `undo_file_modification`: Reverts modified/deleted file using backup.

### C. Workspace Sandbox & Conflict Recovery (`src/workspace/WorkspaceManager.h/cpp`)
- Path validation: absolute path check, symlink resolution (`QFileInfo::canonicalFilePath`), root containment check (`startsWith(m_currentPath)`).
- File conflict detection: SHA-256 hash or modification timestamp comparison before applying edits.
- Automated file recovery: Backups stored in `<workspace>/.jarvis/backups/<backup_id>`.

### D. Agent Execution Engine (`src/agent/AgentEngine.h/cpp`)
- Manages multi-turn task loops (Model $\leftrightarrow$ Tool $\leftrightarrow$ Model).
- Bounded step limits (default 10 steps max) to prevent infinite loops.
- Structured tool-request parsing: Supports JSON tool blocks `{"tool": "...", "arguments": {...}}` or function-calling schemas.
- Approval workflow: Suspends execution, emits `approvalRequested(ToolExecutionRequest)`, waits for `approveCurrentAction()` or `rejectCurrentAction()`.
- Model-agnostic fallback: If model fails to produce valid structured requests or loops, provides informative diagnostics and allows model switching.

### E. Persistent Task History (`src/storage/DatabaseManager.h/cpp`)
- Migration to version 2 schema:
  - `tasks`: id, goal, status, created_at, finished_at, error_message.
  - `task_steps`: id, task_id, step_index, model_thoughts, tool_name, tool_args, tool_result, approval_state, timestamp.
  - `file_audits`: id, task_id, file_path, action ("read", "create", "edit", "delete"), backup_path, timestamp.

### F. UI Integration (`src/ui/Main.qml`, `src/ui/AgentConsole.qml` or integrated view)
- Mode toggle: **Chat** vs **Agent Mode**.
- Interactive Agent Task Pane:
  - Goal input & Start Task button.
  - Step progress indicators & Status badges (`Planning`, `Awaiting Approval`, `Executing`, `Completed`).
  - Unified Diff Preview for file edits/creations.
  - "Approve Action", "Reject Action", and "Abort Task" controls.
  - Touched files summary (Files Read, Files Modified).

---

## 3. Implementation Steps & Validation
1. Extend `DatabaseManager` with task & tool persistence tables.
2. Implement `WorkspaceManager` editing, diff generation, backup/recovery, and deletion safeguards.
3. Build `ITool`, `WorkspaceTools`, and `ToolRegistry`.
4. Build `AgentEngine` with step boundaries, timeout timers, and approval state machine.
5. Integrate with `JarvisController` and expose to QML.
6. Design and integrate glassmorphism Agent view into `Main.qml`.
7. Write comprehensive automated test suite `tests/TestAgentEngine.cpp`.
8. Build, run regression tests, verify offscreen and desktop launch, and package `.deb`.
