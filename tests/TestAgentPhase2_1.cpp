#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "core/Types.h"
#include "storage/DatabaseManager.h"
#include "workspace/WorkspaceManager.h"
#include "agent/ToolRegistry.h"
#include "agent/WorkspaceTools.h"
#include "agent/AgentEngine.h"

using namespace Jarvis;

class TestAgentPhase2_1 : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void testWorkspaceSandboxSafetyAndEscapePrevention();
    void testWorkspaceFileCreationAndApprovalFlow();
    void testWorkspaceFileEditingDiffAndRecovery();
    void testDestructiveFileDeletionWithBackup();
    void testToolRegistryValidation();
    void testAgentTaskEnginePlanningAndToolExecution();
    void testAgentApprovalRejectionBehavior();
    void testAgentMaxStepBoundaryEnforcement();
    void testAgentDatabaseTaskPersistenceAndRecovery();
    void cleanupTestCase();

private:
    std::unique_ptr<QTemporaryDir> m_tempDir;
    std::unique_ptr<QTemporaryDir> m_outsideDir;
    std::unique_ptr<DatabaseManager> m_db;
    std::unique_ptr<WorkspaceManager> m_wm;
    std::unique_ptr<ToolRegistry> m_registry;
};

void TestAgentPhase2_1::initTestCase() {
    m_tempDir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_tempDir->isValid());

    m_outsideDir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_outsideDir->isValid());

    QString dbPath = m_tempDir->path() + "/test_agent.db";
    m_db = std::make_unique<DatabaseManager>(dbPath);
    QVERIFY(m_db->initialize());

    m_wm = std::make_unique<WorkspaceManager>();
    m_wm->setCurrentPath(m_tempDir->path());
    QVERIFY(m_wm->isValid());

    m_registry = std::make_unique<ToolRegistry>();
    m_registry->registerTool(std::make_shared<ListWorkspaceFilesTool>(m_wm.get()));
    m_registry->registerTool(std::make_shared<ReadWorkspaceFileTool>(m_wm.get()));
    m_registry->registerTool(std::make_shared<SearchWorkspaceFilesTool>(m_wm.get()));
    m_registry->registerTool(std::make_shared<CreateFileTool>(m_wm.get()));
    m_registry->registerTool(std::make_shared<EditFileTool>(m_wm.get()));
    m_registry->registerTool(std::make_shared<DeleteFileTool>(m_wm.get()));
}

void TestAgentPhase2_1::testWorkspaceSandboxSafetyAndEscapePrevention() {
    // 1. Path traversal escape attempts
    QVERIFY(!m_wm->isPathSafe("../outside.txt"));
    QVERIFY(!m_wm->isPathSafe("../../etc/passwd"));
    QVERIFY(!m_wm->isPathSafe("/etc/shadow"));

    // 2. Symlink escape prevention
    QString outsideFile = m_outsideDir->path() + "/secret.txt";
    QFile f(outsideFile);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("confidential");
    f.close();

    QString symlinkPath = m_tempDir->path() + "/symlink_out";
    QFile::link(outsideFile, symlinkPath);

    // Should detect that canonical path points outside authorized workspace
    QVERIFY(!m_wm->isPathSafe("symlink_out"));
    QString content = m_wm->readFileContent("symlink_out");
    QVERIFY(content.startsWith("[Security Error"));
}

void TestAgentPhase2_1::testWorkspaceFileCreationAndApprovalFlow() {
    QString err;
    bool created = m_wm->createFile("hello.txt", "Hello from JARVIS Agent Phase 2.1", &err);
    QVERIFY(created);
    QVERIFY(QFile::exists(m_tempDir->path() + "/hello.txt"));

    QString readBack = m_wm->readFileContent("hello.txt");
    QCOMPARE(readBack, QString("Hello from JARVIS Agent Phase 2.1"));

    // Creating already existing file should fail without overwrite
    bool duplicate = m_wm->createFile("hello.txt", "Different content", &err);
    QVERIFY(!duplicate);
    QVERIFY(err.contains("already exists"));
}

void TestAgentPhase2_1::testWorkspaceFileEditingDiffAndRecovery() {
    QString diff, backupId, err;
    QString updatedContent = "Hello from JARVIS Agent Phase 2.1\nAdded line 2 for diff testing.";
    bool edited = m_wm->editFile("hello.txt", updatedContent, &diff, &backupId, &err);
    QVERIFY(edited);
    QVERIFY(!backupId.isEmpty());
    QVERIFY(diff.contains("+Added line 2 for diff testing."));

    // Verify backup exists on disk in .jarvis/backups
    QString backupFile = m_tempDir->path() + "/.jarvis/backups/" + backupId;
    QVERIFY(QFile::exists(backupFile));

    // Verify content of backup is the original version
    QFile bf(backupFile);
    QVERIFY(bf.open(QIODevice::ReadOnly));
    QCOMPARE(QString::fromUtf8(bf.readAll()), QString("Hello from JARVIS Agent Phase 2.1"));
    bf.close();
}

void TestAgentPhase2_1::testDestructiveFileDeletionWithBackup() {
    QString backupId, err;
    bool deleted = m_wm->deleteFile("hello.txt", &backupId, &err);
    QVERIFY(deleted);
    QVERIFY(!QFile::exists(m_tempDir->path() + "/hello.txt"));
    QVERIFY(!backupId.isEmpty());

    // Verify deletion preserved recovery backup
    QString backupFile = m_tempDir->path() + "/.jarvis/backups/" + backupId;
    QVERIFY(QFile::exists(backupFile));
}

void TestAgentPhase2_1::testToolRegistryValidation() {
    // Missing arguments
    QString err;
    QJsonObject emptyArgs;
    QVERIFY(!m_registry->validateToolCall("create_file", emptyArgs, &err));
    QVERIFY(err.contains("path"));

    // Non-existent tool
    QVERIFY(!m_registry->validateToolCall("non_existent_tool", emptyArgs, &err));
    QVERIFY(err.contains("not registered"));

    // Valid arguments
    QJsonObject validArgs;
    validArgs["path"] = "valid_test.txt";
    validArgs["content"] = "test content";
    QVERIFY(m_registry->validateToolCall("create_file", validArgs, &err));
}

void TestAgentPhase2_1::testAgentTaskEnginePlanningAndToolExecution() {
    AgentEngine agent(m_registry.get(), m_db.get());
    agent.startTask("Create test artifact", nullptr, "test_model");
    QCOMPARE(agent.status(), AgentTaskStatus::Planning);

    // Simulate structured tool request from model
    QString simulatedModelToolResponse = 
        "```json\n"
        "{\n"
        "  \"tool\": \"create_file\",\n"
        "  \"arguments\": {\n"
        "    \"path\": \"artifact.txt\",\n"
        "    \"content\": \"Generated artifact content.\"\n"
        "  },\n"
        "  \"reason\": \"Create artifact as requested\"\n"
        "}\n"
        "```";

    agent.handleModelResponse(simulatedModelToolResponse);

    // Should transition to AwaitingApproval because create_file requires approval
    QCOMPARE(agent.status(), AgentTaskStatus::AwaitingApproval);
    QVERIFY(agent.isAwaitingApproval());
    QVERIFY(agent.pendingActionTitle().contains("create_file"));

    // User approves
    agent.approveAction();

    // Verify file actually created through tool
    QVERIFY(QFile::exists(m_tempDir->path() + "/artifact.txt"));

    // Model now sends FINAL_ANSWER
    agent.handleModelResponse("FINAL_ANSWER: Artifact creation complete.");
    QCOMPARE(agent.status(), AgentTaskStatus::Completed);
}

void TestAgentPhase2_1::testAgentApprovalRejectionBehavior() {
    AgentEngine agent(m_registry.get(), m_db.get());
    agent.startTask("Create unauthorized file", nullptr, "test_model");

    QString toolCall = 
        "```json\n"
        "{\n"
        "  \"tool\": \"create_file\",\n"
        "  \"arguments\": {\"path\": \"blocked.txt\", \"content\": \"data\"}\n"
        "}\n"
        "```";

    agent.handleModelResponse(toolCall);
    QCOMPARE(agent.status(), AgentTaskStatus::AwaitingApproval);

    // User rejects the proposed operation
    agent.rejectAction("File creation disallowed by user policy.");
    QVERIFY(!agent.isAwaitingApproval());
    // File must NOT exist
    QVERIFY(!QFile::exists(m_tempDir->path() + "/blocked.txt"));
}

void TestAgentPhase2_1::testAgentMaxStepBoundaryEnforcement() {
    AgentEngine agent(m_registry.get(), m_db.get());
    agent.setMaxSteps(3);
    agent.startTask("Infinite loop test", nullptr, "test_model");

    // Send read actions repeatedly without completing
    for (int i = 0; i < 4; ++i) {
        if (agent.status() == AgentTaskStatus::Failed || agent.status() == AgentTaskStatus::Completed) break;
        QString simulatedTool = "```json\n{\"tool\": \"list_workspace_files\", \"arguments\": {}}\n```";
        agent.handleModelResponse(simulatedTool);
    }

    // Must enforce step limits and terminate
    QCOMPARE(agent.status(), AgentTaskStatus::Failed);
}

void TestAgentPhase2_1::testAgentDatabaseTaskPersistenceAndRecovery() {
    AgentTask t;
    t.id = "task_persist_001";
    t.goal = "Persist test goal";
    t.status = AgentTaskStatus::Planning;
    t.currentStep = 1;
    t.maxSteps = 5;
    t.createdAt = QDateTime::currentDateTime();

    QVERIFY(m_db->saveTask(t));

    AgentTaskStep step;
    step.index = 1;
    step.description = "List files";
    step.toolName = "list_workspace_files";
    step.approvalStatus = "AutoApproved";
    step.timestamp = QDateTime::currentDateTime();
    QVERIFY(m_db->recordTaskStep(t.id, step));

    QVERIFY(m_db->updateTaskStatus(t.id, AgentTaskStatus::Completed, "", "All steps executed cleanly."));

    AgentTask retrieved = m_db->getTask(t.id);
    QCOMPARE(retrieved.id, t.id);
    QCOMPARE(retrieved.status, AgentTaskStatus::Completed);
    QCOMPARE(retrieved.summary, QString("All steps executed cleanly."));
}

void TestAgentPhase2_1::cleanupTestCase() {
}

QTEST_MAIN(TestAgentPhase2_1)
#include "TestAgentPhase2_1.moc"
