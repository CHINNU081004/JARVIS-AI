#pragma once
#include <QString>
#include <QDateTime>
#include <QJsonObject>
#include <QStringList>

namespace Jarvis {

enum class ProviderType {
    OllamaLocal,
    OllamaRemote,
    UniversalApi
};

enum class ApiProtocol {
    OpenAICompatible,
    Anthropic,
    GoogleGemini,
    Custom
};

enum class CapabilityState {
    Supported,
    Unsupported,
    Unknown
};

struct ModelCapabilities {
    CapabilityState streaming = CapabilityState::Supported;
    CapabilityState toolCalling = CapabilityState::Unknown;
    CapabilityState vision = CapabilityState::Unknown;
    CapabilityState audio = CapabilityState::Unsupported;
    qint64 contextWindow = 0; // 0 if unknown
    bool exactMetrics = false;
};

struct ModelInfo {
    QString id;
    QString name;
    QString providerId;
    qint64 parameterSize = 0;
    QString quantization;
    QString format;
    ModelCapabilities capabilities;
};

struct ProviderProfile {
    QString id;
    QString displayName;
    ProviderType type = ProviderType::OllamaLocal;
    ApiProtocol protocol = ApiProtocol::OpenAICompatible;
    QString endpoint = "http://127.0.0.1:11434";
    QString apiKey; // Redacted when logged
    bool isConnected = false;
    qint64 lastLatencyMs = -1;
    QString lastError;
};

struct ChatMessage {
    QString id;
    QString conversationId;
    QString role; // "user", "assistant", "system"
    QString content;
    QString modelUsed;
    QString providerUsed;
    QDateTime timestamp;
    qint64 promptTokens = 0;
    qint64 completionTokens = 0;
    qint64 durationMs = 0;
    double tokensPerSec = 0.0;
    bool isExactMetrics = false;
    bool isError = false;
    bool isCancelled = false;
};

struct ActivityEvent {
    QString id;
    QDateTime timestamp;
    QString level; // "INFO", "WARN", "ERROR", "STEP"
    QString title;
    QString detail;
    QString category; // "Provider", "Chat", "Workspace", "Memory", "System"
};

struct MemoryItem {
    QString id;
    QString key;
    QString content;
    QString category; // "preference", "project", "instruction"
    QDateTime timestamp;
    bool isPinned = false;
    double relevanceScore = 0.0;
};

} // namespace Jarvis
