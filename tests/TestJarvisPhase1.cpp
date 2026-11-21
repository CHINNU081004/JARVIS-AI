#include <QtTest/QtTest>
#include "storage/DatabaseManager.h"
#include "workspace/WorkspaceManager.h"
#include "core/RoutingEngine.h"
#include "core/MetricsCollector.h"
#include "core/ModelListModel.h"
#include "providers/OllamaAdapter.h"
#include "providers/UniversalApiAdapter.h"

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
    void testRamMonitoringParser();
    void testNetworkMonitoringParser();
    void testGpuDetectionAndTelemetry();
    void testModelListModelRolesAndSelection();
    void testUniversalApiProtocols();
    void testOllamaAdapterOfflineHandling();
    void testGenericApiAdapterOfflineHandling();
};

void TestJarvisPhase1::initTestCase() {
    qDebug() << "Initializing JARVIS Phase 1.3 Verification Test Suite";
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
    router.setAutoRoutingEnabled(true);
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

void TestJarvisPhase1::testRamMonitoringParser() {
    QString sampleMemInfo =
        "MemTotal:        8000000 kB\n"
        "MemFree:         1000000 kB\n"
        "MemAvailable:    5000000 kB\n"
        "Buffers:          200000 kB\n";

    auto stats = Jarvis::MetricsCollector::parseMemInfo(sampleMemInfo);
    QVERIFY(stats.isValid);
    QCOMPARE(stats.totalBytes, static_cast<qint64>(8000000) * 1024);
    QCOMPARE(stats.availableBytes, static_cast<qint64>(5000000) * 1024);
    QCOMPARE(stats.usedBytes, static_cast<qint64>(3000000) * 1024);
    QCOMPARE(static_cast<int>(stats.usedPercent), 37);

    // Malformed input test
    auto invalidStats = Jarvis::MetricsCollector::parseMemInfo("Bad content without keys");
    QVERIFY(!invalidStats.isValid);
}

void TestJarvisPhase1::testNetworkMonitoringParser() {
    QString sampleDev =
        "Inter-|   Receive                                                |  Transmit\n"
        " face |bytes    packets errs drop fifo frame compressed multicast|bytes    packets errs drop fifo colls carrier compressed\n"
        "    lo:  100000      50    0    0    0     0          0         0   100000      50    0    0    0     0       0          0\n"
        "wlp2s0: 2048000     100    0    0    0     0          0         0  1024000     100    0    0    0     0       0          0\n";

    // First sample (baseline)
    auto stats1 = Jarvis::MetricsCollector::parseNetworkDev(sampleDev, 0, 0, 0);
    QCOMPARE(stats1.activeInterface, QString("wlp2s0"));
    QCOMPARE(stats1.rxBytes, static_cast<qint64>(2048000));
    QCOMPARE(stats1.txBytes, static_cast<qint64>(1024000));

    // Second sample 1000ms later (+1024000 RX bytes = 1000 KB/s)
    QString sampleDev2 =
        "    lo:  100000      50    0    0    0     0          0         0   100000      50    0    0    0     0       0          0\n"
        "wlp2s0: 3072000     150    0    0    0     0          0         0  1536000     150    0    0    0     0       0          0\n";
    auto stats2 = Jarvis::MetricsCollector::parseNetworkDev(sampleDev2, stats1.rxBytes, stats1.txBytes, 1000);
    QVERIFY(stats2.isValid);
    QCOMPARE(static_cast<int>(stats2.rxKbps), 1000);
    QCOMPARE(static_cast<int>(stats2.txKbps), 500);
    QCOMPARE(static_cast<int>(stats2.totalKbps), 1500);

    // Counter reset protection test
    auto statsReset = Jarvis::MetricsCollector::parseNetworkDev(sampleDev, 5000000, 5000000, 1000);
    QVERIFY(statsReset.isValid);
    QCOMPARE(statsReset.totalKbps, 0.0);
}

void TestJarvisPhase1::testGpuDetectionAndTelemetry() {
    Jarvis::MetricsCollector metrics;
    // On this Ubuntu laptop, vendor is 0x8086 (Intel)
    QCOMPARE(metrics.gpuName(), QString("Intel HD Graphics 620"));
    QCOMPARE(metrics.isGpuDedicated(), false);
    QVERIFY(metrics.gpuTypeStr().contains("Integrated iGPU"));

    metrics.setRemoteGpuConfigured(true);
    QCOMPARE(metrics.isRemoteGpuConfigured(), true);
}

void TestJarvisPhase1::testModelListModelRolesAndSelection() {
    Jarvis::ModelListModel listModel;

    QList<Jarvis::ModelInfo> testModels;
    Jarvis::ModelInfo m1;
    m1.id = "gemma3:4b";
    m1.name = "gemma3:4b";
    m1.quantization = "Q4_K_M";
    m1.capabilities.vision = Jarvis::CapabilityState::Supported;
    testModels.append(m1);

    Jarvis::ModelInfo m2;
    m2.id = "qwen3.5:4b";
    m2.name = "qwen3.5:4b";
    m2.quantization = "Q4_K_M";
    m2.capabilities.toolCalling = Jarvis::CapabilityState::Supported;
    testModels.append(m2);

    listModel.setModels(testModels);
    QCOMPARE(listModel.rowCount(), 2);

    // Test lookup by ID
    QCOMPARE(listModel.indexOf("qwen3.5:4b"), 1);
    QCOMPARE(listModel.idAt(0), QString("gemma3:4b"));
    QCOMPARE(listModel.idAt(1), QString("qwen3.5:4b"));
    QCOMPARE(listModel.nameAt(0), QString("gemma3:4b"));

    // Verify role data
    QModelIndex idx0 = listModel.index(0, 0);
    QCOMPARE(listModel.data(idx0, Jarvis::ModelListModel::DisplayNameRole).toString(), QString("gemma3:4b (Q4_K_M)"));
    QCOMPARE(listModel.data(idx0, Jarvis::ModelListModel::HasVisionRole).toBool(), true);
}

void TestJarvisPhase1::testUniversalApiProtocols() {
    QCOMPARE(Jarvis::UniversalApiAdapter::stringToProtocol("Anthropic"), Jarvis::ApiProtocol::Anthropic);
    QCOMPARE(Jarvis::UniversalApiAdapter::stringToProtocol("claude"), Jarvis::ApiProtocol::Anthropic);
    QCOMPARE(Jarvis::UniversalApiAdapter::stringToProtocol("Google Gemini"), Jarvis::ApiProtocol::GoogleGemini);
    QCOMPARE(Jarvis::UniversalApiAdapter::stringToProtocol("Custom HTTP"), Jarvis::ApiProtocol::Custom);
    QCOMPARE(Jarvis::UniversalApiAdapter::stringToProtocol("OpenAI-Compatible"), Jarvis::ApiProtocol::OpenAICompatible);

    Jarvis::UniversalApiAdapter adapter("test_api", "https://api.anthropic.com", Jarvis::ApiProtocol::Anthropic);
    QCOMPARE(adapter.name(), QString("API"));
    QCOMPARE(adapter.protocol(), Jarvis::ApiProtocol::Anthropic);
}

void TestJarvisPhase1::testOllamaAdapterOfflineHandling() {
    Jarvis::OllamaAdapter adapter("ollama_test", "http://127.0.0.1:65530");
    QSignalSpy spy(&adapter, &Jarvis::IProvider::connectionTested);

    adapter.testConnection();
    QVERIFY(spy.wait(3000));
    QList<QVariant> args = spy.takeFirst();
    QCOMPARE(args.at(0).toBool(), false);
}

void TestJarvisPhase1::testGenericApiAdapterOfflineHandling() {
    Jarvis::UniversalApiAdapter adapter("api_test", "http://127.0.0.1:65531");
    QSignalSpy spy(&adapter, &Jarvis::IProvider::connectionTested);

    adapter.testConnection();
    QVERIFY(spy.wait(3000));
    QList<QVariant> args = spy.takeFirst();
    QCOMPARE(args.at(0).toBool(), false);
}

QTEST_MAIN(TestJarvisPhase1)
#include "TestJarvisPhase1.moc"
