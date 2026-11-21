#include <QtTest/QtTest>
#include "storage/DatabaseManager.h"
#include "workspace/WorkspaceManager.h"
#include "core/RoutingEngine.h"
#include "core/MetricsCollector.h"
#include "providers/OllamaAdapter.h"
#include "providers/GenericApiAdapter.h"

#include <QTemporaryDir>
#include <QFile>

class TestJarvisPhase1 : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void testDatabaseOperations();
    void testBoundedMemoryRetrieval();
    void testWorkspaceSandboxSafety();
    void testRoutingDeterministicRules();
    void testMetricsCalculation();
    void testOllamaAdapterOfflineHandling();
    void testGenericApiAdapterOfflineHandling();
};

void TestJarvisPhase1::initTestCase() {
    qDebug() << "Initializing JARVIS Phase 1 Verification Test Suite";
}

void TestJarvisPhase1::testDatabaseOperations() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QString dbFile = tempDir.filePath("test_jarvis.db");

    Jarvis::DatabaseManager db(dbFile);
    QVERIFY(db.initialize());

    // Conversation lifecycle
    QString convId = db.createConversation("Test Session", "llama3", "ollama_local");
    QVERIFY(!convId.isEmpty());

    auto convs = db.getConversations();
    QCOMPARE(convs.size(), 1);
    QCOMPARE(convs.first().second, QString("Test Session"));

    // Message lifecycle
    Jarvis::ChatMessage msg;
    msg.conversationId = convId;
    msg.role = "user";
    msg.content = "Hello JARVIS";
    msg.modelUsed = "llama3";
    msg.providerUsed = "ollama_local";
    msg.timestamp = QDateTime::currentDateTime();
    QVERIFY(db.saveMessage(msg));

    auto msgs = db.getMessages(convId);
    QCOMPARE(msgs.size(), 1);
    QCOMPARE(msgs.first().content, QString("Hello JARVIS"));

    // History search
    auto searchResults = db.searchHistory("Hello");
    QCOMPARE(searchResults.size(), 1);
}

void TestJarvisPhase1::testBoundedMemoryRetrieval() {
    QTemporaryDir tempDir;
    Jarvis::DatabaseManager db(tempDir.filePath("test_mem.db"));
    QVERIFY(db.initialize());

    Jarvis::MemoryItem m1;
    m1.key = "user_preference";
    m1.content = "Always prefer Python scripts for data analysis.";
    m1.category = "preference";
    m1.isPinned = true;
    db.saveMemory(m1);

    Jarvis::MemoryItem m2;
    m2.key = "project_name";
    m2.content = "The current desktop application is named JARVIS.";
    m2.category = "project";
    m2.isPinned = false;
    db.saveMemory(m2);

    Jarvis::MemoryItem m3;
    m3.key = "unrelated";
    m3.content = "Grocery list contains apples and milk.";
    m3.category = "notes";
    m3.isPinned = false;
    db.saveMemory(m3);

    // Query relevant to python data analysis
    auto relevant = db.getRelevantMemories("Please write a python data analysis tool", 2);
    QVERIFY(!relevant.isEmpty());
    QCOMPARE(relevant.first().key, QString("user_preference")); // Pinned + match
}

void TestJarvisPhase1::testWorkspaceSandboxSafety() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QString root = tempDir.path();

    Jarvis::WorkspaceManager ws;
    ws.setCurrentPath(root);
    QVERIFY(ws.isValid());

    // Create a legitimate file inside
    QString subFile = tempDir.filePath("script.py");
    QFile f(subFile);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("print('hello')\nprint('world')\n");
    f.close();

    // Verify legitimate read
    QString content = ws.readFileContent("script.py");
    QVERIFY(content.contains("hello"));

    // Verify directory traversal attack is blocked
    QString escapedRead = ws.readFileContent("../../etc/passwd");
    QVERIFY(escapedRead.contains("Security Error") || escapedRead.contains("Access Denied"));

    // Verify text search inside workspace
    auto matches = ws.searchText("hello");
    QCOMPARE(matches.size(), 1);
}

void TestJarvisPhase1::testRoutingDeterministicRules() {
    Jarvis::RoutingEngine router;
    QList<Jarvis::ModelInfo> models;

    Jarvis::ModelInfo general;
    general.id = "llama3:latest";
    general.name = "Llama 3";
    models.append(general);

    Jarvis::ModelInfo coder;
    coder.id = "qwen2.5-coder:latest";
    coder.name = "Qwen Coder";
    models.append(coder);

    Jarvis::ModelInfo vision;
    vision.id = "llava:latest";
    vision.name = "Llava Vision";
    vision.capabilities.vision = Jarvis::CapabilityState::Supported;
    models.append(vision);

    // 1. Manual override takes precedence when auto-routing is false
    router.setAutoRoutingEnabled(false);
    QString chosen = router.resolveModel("llama3:latest", "Write a python script", models);
    QCOMPARE(chosen, QString("llama3:latest"));

    // 2. Auto-routing matches coding task
    router.setAutoRoutingEnabled(true);
    QString coderChosen = router.resolveModel("", "Write a python function to compute primes", models);
    QCOMPARE(coderChosen, QString("qwen2.5-coder:latest"));

    // 3. Auto-routing matches vision task
    QString visionChosen = router.resolveModel("", "Analyze this screenshot image", models);
    QCOMPARE(visionChosen, QString("llava:latest"));
}

void TestJarvisPhase1::testMetricsCalculation() {
    Jarvis::MetricsCollector metrics;
    metrics.recordGenerationMetrics(50, 100, 2000, 50.0, true);

    QCOMPARE(metrics.promptTokens(), 50);
    QCOMPARE(metrics.completionTokens(), 100);
    QCOMPARE(metrics.tokensPerSecond(), 50.0);
    QCOMPARE(metrics.isExact(), true);

    metrics.resetGeneration();
    QCOMPARE(metrics.completionTokens(), 0);
    QCOMPARE(metrics.isExact(), false);
}

void TestJarvisPhase1::testOllamaAdapterOfflineHandling() {
    // Port 65530 is closed, should cleanly fail without crash
    Jarvis::OllamaAdapter adapter("ollama_test", "http://127.0.0.1:65530");
    QSignalSpy spy(&adapter, &Jarvis::IProvider::connectionTested);

    adapter.testConnection();
    QVERIFY(spy.wait(3000));
    QList<QVariant> args = spy.takeFirst();
    QCOMPARE(args.at(0).toBool(), false); // Connection should fail gracefully
}

void TestJarvisPhase1::testGenericApiAdapterOfflineHandling() {
    Jarvis::GenericApiAdapter adapter("api_test", "http://127.0.0.1:65531");
    QSignalSpy spy(&adapter, &Jarvis::IProvider::connectionTested);

    adapter.testConnection();
    QVERIFY(spy.wait(3000));
    QList<QVariant> args = spy.takeFirst();
    QCOMPARE(args.at(0).toBool(), false);
}

QTEST_MAIN(TestJarvisPhase1)
#include "TestJarvisPhase1.moc"
