#include "JarvisController.h"
#include <QUuid>
#include <QDir>
#include <QVariantMap>

namespace Jarvis {

// =================== ChatMessageModel ===================

ChatMessageModel::ChatMessageModel(QObject* parent) : QAbstractListModel(parent) {}

int ChatMessageModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return m_messages.size();
}

QVariant ChatMessageModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_messages.size()) return QVariant();
    const auto& msg = m_messages.at(index.row());
    switch (role) {
        case RoleRole: return msg.role;
        case ContentRole: return msg.content;
        case TimestampRole: return msg.timestamp.toString("hh:mm:ss AP");
        case ModelRole: return msg.modelUsed;
        case DurationRole: return QString::number(msg.durationMs / 1000.0, 'f', 1) + "s";
        case TpsRole: return QString::number(msg.tokensPerSec, 'f', 1) + " tok/s";
        case IsExactRole: return msg.isExactMetrics;
        case IsErrorRole: return msg.isError;
        case IsCancelledRole: return msg.isCancelled;
        default: return QVariant();
    }
}

QHash<int, QByteArray> ChatMessageModel::roleNames() const {
    return {
        {RoleRole, "role"},
        {ContentRole, "content"},
        {TimestampRole, "timestamp"},
        {ModelRole, "modelUsed"},
        {DurationRole, "durationStr"},
        {TpsRole, "tpsStr"},
        {IsExactRole, "isExact"},
        {IsErrorRole, "isError"},
        {IsCancelledRole, "isCancelled"}
    };
}

void ChatMessageModel::setMessages(const QList<ChatMessage>& messages) {
    beginResetModel();
    m_messages = messages;
    endResetModel();
}

void ChatMessageModel::appendMessage(const ChatMessage& message) {
    beginInsertRows(QModelIndex(), m_messages.size(), m_messages.size());
    m_messages.append(message);
    endInsertRows();
}

void ChatMessageModel::updateLastAssistantChunk(const QString& chunk) {
    if (m_messages.isEmpty()) return;
    int lastIdx = m_messages.size() - 1;
    if (m_messages[lastIdx].role == "assistant") {
        m_messages[lastIdx].content.append(chunk);
        QModelIndex idx = index(lastIdx, 0);
        emit dataChanged(idx, idx, {ContentRole});
    }
}

void ChatMessageModel::updateLastAssistantComplete(const ChatMessage& completeMessage) {
    if (m_messages.isEmpty()) return;
    int lastIdx = m_messages.size() - 1;
    if (m_messages[lastIdx].role == "assistant") {
        m_messages[lastIdx] = completeMessage;
        QModelIndex idx = index(lastIdx, 0);
        emit dataChanged(idx, idx);
    }
}

void ChatMessageModel::clear() {
    beginResetModel();
    m_messages.clear();
    endResetModel();
}

// =================== ActivityModel ===================

ActivityModel::ActivityModel(QObject* parent) : QAbstractListModel(parent) {}

int ActivityModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return m_events.size();
}

QVariant ActivityModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_events.size()) return QVariant();
    const auto& ev = m_events.at(index.row());
    switch (role) {
        case TimestampRole: return ev.timestamp.toString("hh:mm:ss");
        case LevelRole: return ev.level;
        case TitleRole: return ev.title;
        case DetailRole: return ev.detail;
        case CategoryRole: return ev.category;
        default: return QVariant();
    }
}

QHash<int, QByteArray> ActivityModel::roleNames() const {
    return {
        {TimestampRole, "timestamp"},
        {LevelRole, "level"},
        {TitleRole, "title"},
        {DetailRole, "detail"},
        {CategoryRole, "category"}
    };
}

void ActivityModel::addActivity(const ActivityEvent& event) {
    beginInsertRows(QModelIndex(), 0, 0);
    m_events.prepend(event);
    if (m_events.size() > 100) {
        m_events.removeLast();
    }
    endInsertRows();
}

void ActivityModel::setActivities(const QList<ActivityEvent>& list) {
    beginResetModel();
    m_events = list;
    endResetModel();
}

// =================== JarvisController ===================

JarvisController::JarvisController(QObject* parent)
    : QObject(parent)
{
}

JarvisController::~JarvisController() {
    qDeleteAll(m_providers);
}

bool JarvisController::initialize() {
    // 1. Initialize SQLite Database
    if (!m_db.initialize()) {
        logEvent("ERROR", "Storage Failure", "Failed to initialize SQLite persistence.", "Storage");
        return false;
    }

    // 2. Restore saved settings
    QString savedIntensity = m_db.getSetting("ui_animation_intensity", "2");
    m_animationIntensity = savedIntensity.toInt();
    m_particlesEnabled = m_db.getSetting("ui_particles_enabled", "1") == "1";
    m_memoryEnabled = m_db.getSetting("memory_enabled", "1") == "1";
    m_maxMemoryRetrievalCount = m_db.getSetting("memory_max_retrieval", "3").toInt();
    if (m_maxMemoryRetrievalCount <= 0) m_maxMemoryRetrievalCount = 3;

    // 3. Register Providers
    auto* localOllama = new OllamaAdapter("ollama_local", "http://127.0.0.1:11434", this);
    auto* remoteOllama = new OllamaAdapter("ollama_remote", m_db.getSetting("remote_ollama_url", "http://remote-server:11434"), this);

    QString apiProtoStr = m_db.getSetting("api_protocol", "OpenAI-Compatible");
    auto* universalApi = new UniversalApiAdapter("universal_api",
                                                 m_db.getSetting("universal_api_url", "https://api.openai.com/v1"),
                                                 UniversalApiAdapter::stringToProtocol(apiProtoStr),
                                                 this);
    QString savedApiKey = m_db.getSetting("universal_api_key", "");
    if (!savedApiKey.isEmpty()) universalApi->setApiKey(savedApiKey);

    m_providers[localOllama->id()] = localOllama;
    m_providers[remoteOllama->id()] = remoteOllama;
    m_providers[universalApi->id()] = universalApi;
    // Map generic_api alias to universal_api
    m_providers["generic_api"] = universalApi;

    for (auto* prov : m_providers.values()) {
        connect(prov, &IProvider::connectionTested, this, &JarvisController::onConnectionTested);
        connect(prov, &IProvider::modelsDiscovered, this, &JarvisController::onModelsDiscovered);
        connect(prov, &IProvider::streamChunkReceived, this, &JarvisController::onStreamChunk);
        connect(prov, &IProvider::generationFinished, this, &JarvisController::onGenerationFinished);
        connect(prov, &IProvider::generationCancelled, this, &JarvisController::onGenerationCancelled);
        connect(prov, &IProvider::errorOccurred, this, &JarvisController::onProviderError);
    }

    // 4. Initial activity timeline load
    m_activityModel.setActivities(m_db.getRecentActivities(30));
    logEvent("INFO", "JARVIS Core Initialized", "System ready. Native C++20/Qt6 runtime active.", "System");

    // 5. Connect default provider
    testActiveProvider();
    refreshModels();

    // 6. Ensure default conversation exists
    auto convs = m_db.getConversations();
    if (convs.isEmpty()) {
        createNewConversation("Initial Session");
    } else {
        loadConversation(convs.first().first);
    }

    return true;
}

IProvider* JarvisController::currentProvider() const {
    return m_providers.value(m_selectedProviderId, nullptr);
}

bool JarvisController::isProviderConnected() const {
    // Current connection status based on active provider
    return false; // updated dynamically via connectionTested signals
}

void JarvisController::setAnimationIntensity(int level) {
    if (m_animationIntensity != level) {
        m_animationIntensity = level;
        m_db.saveSetting("ui_animation_intensity", QString::number(level));
        emit animationIntensityChanged();
    }
}

void JarvisController::setParticlesEnabled(bool enabled) {
    if (m_particlesEnabled != enabled) {
        m_particlesEnabled = enabled;
        m_db.saveSetting("ui_particles_enabled", enabled ? "1" : "0");
        emit animationIntensityChanged();
    }
}

void JarvisController::setMemoryEnabled(bool enabled) {
    if (m_memoryEnabled != enabled) {
        m_memoryEnabled = enabled;
        m_db.saveSetting("memory_enabled", enabled ? "1" : "0");
        emit memorySettingsChanged();
    }
}

void JarvisController::setMaxMemoryRetrievalCount(int count) {
    if (count > 0 && m_maxMemoryRetrievalCount != count) {
        m_maxMemoryRetrievalCount = count;
        m_db.saveSetting("memory_max_retrieval", QString::number(count));
        emit memorySettingsChanged();
    }
}

void JarvisController::selectProvider(const QString& providerId) {
    if (m_providers.contains(providerId) && m_selectedProviderId != providerId) {
        m_selectedProviderId = providerId;
        emit providerChanged();
        logEvent("INFO", "Provider Changed", QString("Active provider switched to %1").arg(providerId), "Provider");

        // Update model list for this provider
        QList<ModelInfo> currentProviderModels;
        for (const auto& m : m_allModels) {
            if (m.providerId == m_selectedProviderId) {
                currentProviderModels.append(m);
            }
        }
        m_modelListModel.setModels(currentProviderModels);
        m_selectedModelId = currentProviderModels.isEmpty() ? QString() : currentProviderModels.first().id;
        emit modelChanged();

        testActiveProvider();
        refreshModels();
    }
}

void JarvisController::selectModel(const QString& modelId) {
    if (m_selectedModelId != modelId) {
        m_selectedModelId = modelId;
        emit modelChanged();
        logEvent("INFO", "Model Selected", QString("Active model set to %1").arg(modelId), "Model");
    }
}

void JarvisController::testActiveProvider() {
    auto* prov = currentProvider();
    if (prov) {
        m_activeTaskStep = QString("Testing connection to %1...").arg(prov->endpoint());
        emit activeTaskChanged();
        prov->testConnection();
    }
}

void JarvisController::refreshModels() {
    auto* prov = currentProvider();
    if (prov) {
        logEvent("INFO", "Model Discovery", QString("Querying available models from %1").arg(prov->endpoint()), "Provider");
        prov->discoverModels();
    }
}

void JarvisController::updateProviderEndpoint(const QString& providerId, const QString& newUrl, const QString& apiKey, const QString& protocolStr) {
    auto* prov = m_providers.value(providerId, nullptr);
    if (prov) {
        prov->setEndpoint(newUrl);
        if (!apiKey.isEmpty()) prov->setApiKey(apiKey);

        if (providerId == "ollama_remote") {
            m_db.saveSetting("remote_ollama_url", newUrl);
            m_metrics.setRemoteGpuConfigured(true);
        } else if (providerId == "universal_api" || providerId == "generic_api") {
            m_db.saveSetting("universal_api_url", newUrl);
            if (!apiKey.isEmpty()) m_db.saveSetting("universal_api_key", apiKey);
            if (!protocolStr.isEmpty()) {
                m_db.saveSetting("api_protocol", protocolStr);
                auto* uApi = qobject_cast<UniversalApiAdapter*>(prov);
                if (uApi) {
                    uApi->setProtocol(UniversalApiAdapter::stringToProtocol(protocolStr));
                }
            }
        }
        testActiveProvider();
        refreshModels();
    }
}

QString JarvisController::getProviderProtocol(const QString& providerId) const {
    auto* prov = m_providers.value(providerId, nullptr);
    auto* uApi = qobject_cast<UniversalApiAdapter*>(prov);
    if (uApi) {
        return UniversalApiAdapter::protocolToString(uApi->protocol());
    }
    return QString();
}

QVariantList JarvisController::getDiscoveredModels() {
    QVariantList list;
    for (const auto& m : m_allModels) {
        if (m.providerId == m_selectedProviderId) {
            QVariantMap map;
            map["id"] = m.id;
            map["name"] = m.name;
            map["providerId"] = m.providerId;
            map["parameterSize"] = m.parameterSize;
            map["quantization"] = m.quantization;
            map["hasVision"] = (m.capabilities.vision == CapabilityState::Supported);
            map["hasStreaming"] = (m.capabilities.streaming == CapabilityState::Supported);
            list.append(map);
        }
    }
    return list;
}

QVariantList JarvisController::getConversations() {
    QVariantList list;
    auto convs = m_db.getConversations();
    for (const auto& c : convs) {
        QVariantMap map;
        map["id"] = c.first;
        map["title"] = c.second;
        list.append(map);
    }
    return list;
}

void JarvisController::createNewConversation(const QString& title) {
    m_currentConversationId = m_db.createConversation(title, m_selectedModelId, m_selectedProviderId);
    m_currentConversationTitle = title;
    m_chatModel.clear();
    emit currentConversationChanged();
    logEvent("INFO", "Conversation Created", QString("Session: %1").arg(title), "Chat");
}

void JarvisController::loadConversation(const QString& convId) {
    m_currentConversationId = convId;
    auto msgs = m_db.getMessages(convId);
    m_chatModel.setMessages(msgs);
    emit currentConversationChanged();
}

void JarvisController::deleteConversation(const QString& convId) {
    m_db.deleteConversation(convId);
    auto convs = m_db.getConversations();
    if (!convs.isEmpty()) {
        loadConversation(convs.first().first);
    } else {
        createNewConversation("New Chat");
    }
}

void JarvisController::sendMessage(const QString& text) {
    if (text.trimmed().isEmpty() || m_isGenerating) return;

    // 1. Create and save User Message
    ChatMessage userMsg;
    userMsg.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    userMsg.conversationId = m_currentConversationId;
    userMsg.role = "user";
    userMsg.content = text;
    userMsg.timestamp = QDateTime::currentDateTime();
    userMsg.modelUsed = m_selectedModelId;
    userMsg.providerUsed = m_selectedProviderId;

    m_db.saveMessage(userMsg);
    m_chatModel.appendMessage(userMsg);

    // 2. Deterministic Model Routing
    QString routingReason;
    QString targetModel = m_router.resolveModel(m_selectedModelId, text, m_allModels, &routingReason);
    logEvent("INFO", "Routing Resolution", QString("Resolved Model: %1 (%2)").arg(targetModel, routingReason), "Router");

    // 3. Bounded Relevant Memory Retrieval (Respect user setting)
    QString systemPrompt = "You are JARVIS, a helpful, precise native AI assistant.";
    if (m_memoryEnabled) {
        auto relevantMems = m_db.getRelevantMemories(text, m_maxMemoryRetrievalCount);
        if (!relevantMems.isEmpty()) {
            systemPrompt += "\n[Relevant Memories]:";
            for (const auto& mem : relevantMems) {
                systemPrompt += QString("\n- (%1) %2").arg(mem.key, mem.content);
            }
            logEvent("INFO", "Memory Retrieved", QString("Retrieved %1 relevant memories for query.").arg(relevantMems.size()), "Memory");
        }
    }

    // 4. Create empty assistant placeholder message
    ChatMessage assistantMsg;
    assistantMsg.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    assistantMsg.conversationId = m_currentConversationId;
    assistantMsg.role = "assistant";
    assistantMsg.content = "";
    assistantMsg.timestamp = QDateTime::currentDateTime();
    assistantMsg.modelUsed = targetModel;
    assistantMsg.providerUsed = m_selectedProviderId;

    m_chatModel.appendMessage(assistantMsg);

    // 5. Initiate Stream Generation via active Provider
    auto* prov = currentProvider();
    if (!prov) {
        onProviderError("No Provider", "Selected provider is not available.");
        return;
    }

    m_isGenerating = true;
    m_activeTaskDescription = "Generating response...";
    m_activeTaskStep = "Streaming tokens";
    emit isGeneratingChanged();
    emit activeTaskChanged();

    m_metrics.resetGeneration();

    auto history = m_db.getMessages(m_currentConversationId);
    prov->generateChatResponse(targetModel, history, systemPrompt);
}

void JarvisController::cancelGeneration() {
    auto* prov = currentProvider();
    if (prov && m_isGenerating) {
        prov->cancelActiveRequest();
        logEvent("WARN", "Request Cancelled", "User aborted generation.", "Chat");
    }
}

void JarvisController::onConnectionTested(bool success, qint64 latencyMs, const QString& error) {
    m_metrics.recordLatency(latencyMs);
    if (success) {
        m_activeTaskStep = QString("Connected (%1 ms)").arg(latencyMs);
        logEvent("INFO", "Connection Successful", QString("%1 latency: %2 ms").arg(m_selectedProviderId).arg(latencyMs), "Provider");
    } else {
        m_activeTaskStep = "Offline";
        logEvent("WARN", "Connection Failed", error, "Provider");
    }
    emit providerConnectionChanged();
    emit activeTaskChanged();
}

void JarvisController::onModelsDiscovered(const QList<ModelInfo>& models) {
    m_allModels.erase(std::remove_if(m_allModels.begin(), m_allModels.end(),
                      [this](const ModelInfo& m) { return m.providerId == m_selectedProviderId; }),
                      m_allModels.end());
    m_allModels.append(models);

    // Update typed model list for UI
    QList<ModelInfo> currentProviderModels;
    for (const auto& m : m_allModels) {
        if (m.providerId == m_selectedProviderId) {
            currentProviderModels.append(m);
        }
    }
    m_modelListModel.setModels(currentProviderModels);

    // Preserve selected model if still in list, else default to first available
    bool selectedStillExists = false;
    for (const auto& m : currentProviderModels) {
        if (m.id == m_selectedModelId) {
            selectedStillExists = true;
            break;
        }
    }

    if (!selectedStillExists && !currentProviderModels.isEmpty()) {
        m_selectedModelId = currentProviderModels.first().id;
        emit modelChanged();
    } else if (currentProviderModels.isEmpty()) {
        m_selectedModelId.clear();
        emit modelChanged();
    }

    logEvent("INFO", "Models Updated", QString("Discovered %1 models from %2").arg(models.size()).arg(m_selectedProviderId), "Provider");
    emit modelsListChanged();
}

void JarvisController::onStreamChunk(const QString& chunk) {
    m_chatModel.updateLastAssistantChunk(chunk);
}

void JarvisController::onGenerationFinished(const ChatMessage& message) {
    m_isGenerating = false;
    m_activeTaskDescription = "Ready";
    m_activeTaskStep = "Idle";
    emit isGeneratingChanged();
    emit activeTaskChanged();

    m_db.saveMessage(message);
    m_chatModel.updateLastAssistantComplete(message);

    m_metrics.recordGenerationMetrics(message.promptTokens, message.completionTokens,
                                      message.durationMs, message.tokensPerSec, message.isExactMetrics);

    logEvent("INFO", "Generation Finished",
             QString("Generated %1 tokens in %2 ms (%3 tok/s)")
             .arg(message.completionTokens).arg(message.durationMs).arg(QString::number(message.tokensPerSec, 'f', 1)),
             "Chat");
}

void JarvisController::onGenerationCancelled() {
    m_isGenerating = false;
    m_activeTaskDescription = "Ready";
    m_activeTaskStep = "Cancelled";
    emit isGeneratingChanged();
    emit activeTaskChanged();

    ChatMessage cancMsg;
    cancMsg.role = "assistant";
    cancMsg.content = "[Generation cancelled by user]";
    cancMsg.isCancelled = true;
    m_chatModel.updateLastAssistantComplete(cancMsg);
}

void JarvisController::onProviderError(const QString& title, const QString& detail) {
    m_isGenerating = false;
    m_activeTaskDescription = "Error";
    m_activeTaskStep = detail;
    emit isGeneratingChanged();
    emit activeTaskChanged();

    logEvent("ERROR", title, detail, "Provider");
    emit errorNotice(title, detail);
}

void JarvisController::logEvent(const QString& level, const QString& title, const QString& detail, const QString& category) {
    ActivityEvent ev;
    ev.timestamp = QDateTime::currentDateTime();
    ev.level = level;
    ev.title = title;
    ev.detail = detail;
    ev.category = category;
    m_db.logActivity(ev);
    m_activityModel.addActivity(ev);
}

QVariantList JarvisController::getAllMemories() {
    QVariantList list;
    auto mems = m_db.getAllMemories();
    for (const auto& m : mems) {
        QVariantMap map;
        map["id"] = m.id;
        map["key"] = m.key;
        map["content"] = m.content;
        map["category"] = m.category;
        map["pinned"] = m.isPinned;
        map["timestamp"] = m.timestamp.toString("yyyy-MM-dd hh:mm");
        list.append(map);
    }
    return list;
}

void JarvisController::addMemory(const QString& key, const QString& content, const QString& category, bool pinned) {
    MemoryItem item;
    item.key = key;
    item.content = content;
    item.category = category;
    item.isPinned = pinned;
    item.timestamp = QDateTime::currentDateTime();
    m_db.saveMemory(item);
    logEvent("INFO", "Memory Stored", QString("Key: %1").arg(key), "Memory");
}

void JarvisController::deleteMemory(const QString& id) {
    m_db.deleteMemory(id);
    logEvent("INFO", "Memory Deleted", QString("ID: %1").arg(id), "Memory");
}

} // namespace Jarvis
