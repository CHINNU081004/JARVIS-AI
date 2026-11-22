#include "OllamaAdapter.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QNetworkRequest>
#include <QTimer>

namespace Jarvis {

OllamaAdapter::OllamaAdapter(const QString& providerId,
                             const QString& endpoint,
                             QObject* parent)
    : IProvider(parent)
    , m_id(providerId)
    , m_name(providerId.contains("remote", Qt::CaseInsensitive) ? "Ollama (Remote)" : "Ollama (Local)")
    , m_type(providerId.contains("remote", Qt::CaseInsensitive) ? ProviderType::OllamaRemote : ProviderType::OllamaLocal)
    , m_endpoint(endpoint)
    , m_netManager(new QNetworkAccessManager(this))
{
}

OllamaAdapter::~OllamaAdapter() {
    cancelActiveRequest();
}

void OllamaAdapter::setEndpoint(const QString& url) {
    m_endpoint = url.trimmed();
    if (m_endpoint.endsWith('/')) {
        m_endpoint.chop(1);
    }
}

void OllamaAdapter::testConnection() {
    QUrl url(m_endpoint + "/api/tags");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    if (!m_apiKey.isEmpty()) {
        request.setRawHeader("Authorization", "Bearer " + m_apiKey.toUtf8());
    }

    QElapsedTimer* timer = new QElapsedTimer();
    timer->start();

    QNetworkReply* reply = m_netManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, timer]() {
        qint64 latency = timer->elapsed();
        delete timer;
        reply->deleteLater();

        if (reply->error() == QNetworkReply::NoError) {
            emit connectionTested(true, latency, QString());
        } else {
            QString err = reply->errorString();
            emit connectionTested(false, latency, err);
        }
    });
}

void OllamaAdapter::discoverModels() {
    QUrl url(m_endpoint + "/api/tags");
    QNetworkRequest request(url);
    if (!m_apiKey.isEmpty()) {
        request.setRawHeader("Authorization", "Bearer " + m_apiKey.toUtf8());
    }

    QNetworkReply* reply = m_netManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit errorOccurred("Model Discovery Failed", reply->errorString());
            return;
        }

        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (!doc.isObject()) {
            emit errorOccurred("Invalid Response", "Failed to parse Ollama model tags.");
            return;
        }

        QJsonArray modelsArray = doc.object().value("models").toArray();
        QList<ModelInfo> discovered;

        for (const auto& item : modelsArray) {
            QJsonObject mObj = item.toObject();
            ModelInfo info;
            info.id = mObj.value("model").toString(mObj.value("name").toString());
            info.name = mObj.value("name").toString(info.id);
            info.providerId = m_id;
            info.parameterSize = mObj.value("size").toInteger();
            
            QJsonObject details = mObj.value("details").toObject();
            info.quantization = details.value("quantization_level").toString();
            info.format = details.value("format").toString();

            // Native capabilities array from Ollama v0.40+ (e.g. ["completion", "vision", "tools", "thinking"])
            QJsonArray capsArray = mObj.value("capabilities").toArray();
            bool hasCaps = !capsArray.isEmpty();
            bool canVision = false;
            bool canTools = false;

            for (const auto& cVal : capsArray) {
                QString cap = cVal.toString().toLower();
                if (cap == "vision") canVision = true;
                if (cap == "tools") canTools = true;
            }

            // Capability heuristics
            info.capabilities.streaming = CapabilityState::Supported;
            info.capabilities.exactMetrics = true;

            QString idLower = info.id.toLower();
            if (canVision || idLower.contains("vision") || idLower.contains("llava") || idLower.contains("minicpm-v")) {
                info.capabilities.vision = CapabilityState::Supported;
            } else {
                info.capabilities.vision = CapabilityState::Unsupported;
            }

            if (canTools || idLower.contains("tools") || idLower.contains("coder")) {
                info.capabilities.toolCalling = CapabilityState::Supported;
            } else {
                info.capabilities.toolCalling = hasCaps ? CapabilityState::Unsupported : CapabilityState::Unknown;
            }

            discovered.append(info);
        }

        emit modelsDiscovered(discovered);
    });
}

void OllamaAdapter::generateChatResponse(const QString& modelId,
                                        const QList<ChatMessage>& history,
                                        const QString& systemPrompt)
{
    cancelActiveRequest();

    m_activeModelId = modelId;
    m_accumulatedResponse.clear();
    m_streamBuffer.clear();
    m_promptTokens = 0;
    m_evalTokens = 0;
    m_evalDurationNs = 0;
    m_wasCancelled = false;

    QUrl url(m_endpoint + "/api/chat");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    if (!m_apiKey.isEmpty()) {
        request.setRawHeader("Authorization", "Bearer " + m_apiKey.toUtf8());
    }

    QJsonObject root;
    root["model"] = modelId;
    root["stream"] = true;

    QJsonArray messages;
    if (!systemPrompt.isEmpty()) {
        QJsonObject sysObj;
        sysObj["role"] = "system";
        sysObj["content"] = systemPrompt;
        messages.append(sysObj);
    }

    for (const auto& msg : history) {
        QJsonObject mObj;
        mObj["role"] = msg.role;
        mObj["content"] = msg.content;
        messages.append(mObj);
    }
    root["messages"] = messages;

    QByteArray payload = QJsonDocument(root).toJson(QJsonDocument::Compact);

    m_generationTimer.start();
    m_currentReply = m_netManager->post(request, payload);

    connect(m_currentReply, &QNetworkReply::readyRead, this, &OllamaAdapter::onStreamReadyRead);
    connect(m_currentReply, &QNetworkReply::finished, this, &OllamaAdapter::onStreamFinished);
}

void OllamaAdapter::onStreamReadyRead() {
    if (!m_currentReply) return;

    m_streamBuffer.append(m_currentReply->readAll());

    int newlineIdx = -1;
    while ((newlineIdx = m_streamBuffer.indexOf('\n')) != -1) {
        QByteArray line = m_streamBuffer.left(newlineIdx).trimmed();
        m_streamBuffer.remove(0, newlineIdx + 1);

        if (line.isEmpty()) continue;

        QJsonDocument doc = QJsonDocument::fromJson(line);
        if (!doc.isObject()) continue;

        QJsonObject obj = doc.object();
        if (obj.contains("message")) {
            QString chunk = obj.value("message").toObject().value("content").toString();
            if (!chunk.isEmpty()) {
                m_accumulatedResponse.append(chunk);
                emit streamChunkReceived(chunk);
            }
        }

        if (obj.value("done").toBool()) {
            m_promptTokens = obj.value("prompt_eval_count").toInteger(0);
            m_evalTokens = obj.value("eval_count").toInteger(0);
            m_evalDurationNs = obj.value("eval_duration").toInteger(0);
        }
    }
}

void OllamaAdapter::onStreamFinished() {
    if (!m_currentReply) return;

    QNetworkReply* reply = m_currentReply;
    m_currentReply = nullptr;
    reply->deleteLater();

    if (m_wasCancelled) {
        emit generationCancelled();
        return;
    }

    if (reply->error() != QNetworkReply::NoError) {
        emit errorOccurred("Generation Error", reply->errorString());
        return;
    }

    ChatMessage finalMsg;
    finalMsg.role = "assistant";
    finalMsg.content = m_accumulatedResponse;
    finalMsg.modelUsed = m_activeModelId;
    finalMsg.providerUsed = m_id;
    finalMsg.timestamp = QDateTime::currentDateTime();
    finalMsg.durationMs = m_generationTimer.elapsed();
    finalMsg.promptTokens = m_promptTokens;
    finalMsg.completionTokens = m_evalTokens > 0 ? m_evalTokens : m_accumulatedResponse.split(' ', Qt::SkipEmptyParts).size();
    finalMsg.isExactMetrics = (m_evalTokens > 0);

    if (m_evalDurationNs > 0 && m_evalTokens > 0) {
        double seconds = static_cast<double>(m_evalDurationNs) / 1e9;
        finalMsg.tokensPerSec = seconds > 0 ? (m_evalTokens / seconds) : 0.0;
    } else if (finalMsg.durationMs > 0 && finalMsg.completionTokens > 0) {
        finalMsg.tokensPerSec = (finalMsg.completionTokens * 1000.0) / finalMsg.durationMs;
    }

    emit generationFinished(finalMsg);
}

void OllamaAdapter::cancelActiveRequest() {
    if (m_currentReply) {
        m_wasCancelled = true;
        disconnect(m_currentReply, nullptr, this, nullptr);
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }
}

} // namespace Jarvis
