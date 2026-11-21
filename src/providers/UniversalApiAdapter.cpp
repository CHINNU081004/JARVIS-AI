#include "UniversalApiAdapter.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QNetworkRequest>

namespace Jarvis {

UniversalApiAdapter::UniversalApiAdapter(const QString& providerId,
                                         const QString& endpoint,
                                         ApiProtocol protocol,
                                         QObject* parent)
    : IProvider(parent)
    , m_id(providerId)
    , m_endpoint(endpoint)
    , m_protocol(protocol)
    , m_netManager(new QNetworkAccessManager(this))
{
}

UniversalApiAdapter::~UniversalApiAdapter() {
    cancelActiveRequest();
}

QString UniversalApiAdapter::protocolToString(ApiProtocol p) {
    switch (p) {
        case ApiProtocol::Anthropic: return "Anthropic";
        case ApiProtocol::GoogleGemini: return "Google Gemini";
        case ApiProtocol::Custom: return "Custom HTTP";
        case ApiProtocol::OpenAICompatible:
        default: return "OpenAI-Compatible";
    }
}

ApiProtocol UniversalApiAdapter::stringToProtocol(const QString& str) {
    QString s = str.trimmed().toLower();
    if (s.contains("anthropic") || s.contains("claude")) return ApiProtocol::Anthropic;
    if (s.contains("gemini") || s.contains("google")) return ApiProtocol::GoogleGemini;
    if (s.contains("custom")) return ApiProtocol::Custom;
    return ApiProtocol::OpenAICompatible;
}

void UniversalApiAdapter::setEndpoint(const QString& url) {
    m_endpoint = url.trimmed();
    if (m_endpoint.endsWith('/')) {
        m_endpoint.chop(1);
    }
}

void UniversalApiAdapter::testConnection() {
    QUrl url;
    QNetworkRequest request;

    if (m_protocol == ApiProtocol::Anthropic) {
        url = QUrl(m_endpoint + "/v1/models");
        request.setUrl(url);
        if (!m_apiKey.isEmpty()) request.setRawHeader("x-api-key", m_apiKey.toUtf8());
        request.setRawHeader("anthropic-version", "2023-06-01");
    } else if (m_protocol == ApiProtocol::GoogleGemini) {
        QString ep = m_endpoint + "/v1beta/models";
        if (!m_apiKey.isEmpty()) ep += "?key=" + m_apiKey;
        url = QUrl(ep);
        request.setUrl(url);
    } else {
        // OpenAI Compatible / Custom
        url = QUrl(m_endpoint + "/models");
        request.setUrl(url);
        if (!m_apiKey.isEmpty()) request.setRawHeader("Authorization", "Bearer " + m_apiKey.toUtf8());
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
            emit connectionTested(false, latency, reply->errorString());
        }
    });
}

void UniversalApiAdapter::discoverModels() {
    QUrl url;
    QNetworkRequest request;

    if (m_protocol == ApiProtocol::GoogleGemini) {
        QString ep = m_endpoint + "/v1beta/models";
        if (!m_apiKey.isEmpty()) ep += "?key=" + m_apiKey;
        url = QUrl(ep);
        request.setUrl(url);
    } else if (m_protocol == ApiProtocol::Anthropic) {
        url = QUrl(m_endpoint + "/v1/models");
        request.setUrl(url);
        if (!m_apiKey.isEmpty()) request.setRawHeader("x-api-key", m_apiKey.toUtf8());
        request.setRawHeader("anthropic-version", "2023-06-01");
    } else {
        url = QUrl(m_endpoint + "/models");
        request.setUrl(url);
        if (!m_apiKey.isEmpty()) request.setRawHeader("Authorization", "Bearer " + m_apiKey.toUtf8());
    }

    QNetworkReply* reply = m_netManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            // If discovery endpoint not supported (common on custom APIs), supply fallback standard models
            QList<ModelInfo> fallbacks;
            if (m_protocol == ApiProtocol::Anthropic) {
                ModelInfo m; m.id = "claude-3-7-sonnet-20250219"; m.name = "Claude 3.7 Sonnet"; m.providerId = m_id;
                fallbacks.append(m);
            } else if (m_protocol == ApiProtocol::GoogleGemini) {
                ModelInfo m; m.id = "gemini-2.0-flash"; m.name = "Gemini 2.0 Flash"; m.providerId = m_id;
                fallbacks.append(m);
            }
            if (!fallbacks.isEmpty()) {
                emit modelsDiscovered(fallbacks);
                return;
            }
            emit errorOccurred("Model Discovery Notice", reply->errorString());
            return;
        }

        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QList<ModelInfo> discovered;

        if (doc.isObject()) {
            QJsonArray modelsArray = doc.object().value(m_protocol == ApiProtocol::GoogleGemini ? "models" : "data").toArray();
            for (const auto& item : modelsArray) {
                QJsonObject mObj = item.toObject();
                ModelInfo info;
                QString rawId = mObj.value(m_protocol == ApiProtocol::GoogleGemini ? "name" : "id").toString();
                if (rawId.startsWith("models/")) rawId = rawId.mid(7);
                info.id = rawId;
                info.name = mObj.value("displayName").toString(info.id);
                info.providerId = m_id;
                info.capabilities.streaming = CapabilityState::Supported;
                info.capabilities.toolCalling = CapabilityState::Supported;
                discovered.append(info);
            }
        }

        emit modelsDiscovered(discovered);
    });
}

void UniversalApiAdapter::generateChatResponse(const QString& modelId,
                                              const QList<ChatMessage>& history,
                                              const QString& systemPrompt)
{
    cancelActiveRequest();

    m_activeModelId = modelId;
    m_accumulatedResponse.clear();
    m_streamBuffer.clear();
    m_promptTokens = 0;
    m_completionTokens = 0;
    m_wasCancelled = false;

    QUrl url;
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QByteArray payload;

    if (m_protocol == ApiProtocol::Anthropic) {
        url = QUrl(m_endpoint + "/v1/messages");
        request.setUrl(url);
        if (!m_apiKey.isEmpty()) request.setRawHeader("x-api-key", m_apiKey.toUtf8());
        request.setRawHeader("anthropic-version", "2023-06-01");

        QJsonObject root;
        root["model"] = modelId.isEmpty() ? "claude-3-7-sonnet-20250219" : modelId;
        root["max_tokens"] = 4096;
        root["stream"] = true;
        if (!systemPrompt.isEmpty()) root["system"] = systemPrompt;

        QJsonArray msgs;
        for (const auto& m : history) {
            QJsonObject o;
            o["role"] = m.role == "assistant" ? "assistant" : "user";
            o["content"] = m.content;
            msgs.append(o);
        }
        root["messages"] = msgs;
        payload = QJsonDocument(root).toJson(QJsonDocument::Compact);

    } else if (m_protocol == ApiProtocol::GoogleGemini) {
        QString cleanModel = modelId.startsWith("models/") ? modelId.mid(7) : modelId;
        if (cleanModel.isEmpty()) cleanModel = "gemini-2.0-flash";
        QString ep = QString("%1/v1beta/models/%2:streamGenerateContent?alt=sse").arg(m_endpoint, cleanModel);
        if (!m_apiKey.isEmpty()) ep += "&key=" + m_apiKey;
        url = QUrl(ep);
        request.setUrl(url);

        QJsonObject root;
        QJsonArray contents;
        for (const auto& m : history) {
            QJsonObject item;
            item["role"] = m.role == "assistant" ? "model" : "user";
            QJsonArray parts;
            QJsonObject textPart;
            textPart["text"] = m.content;
            parts.append(textPart);
            item["parts"] = parts;
            contents.append(item);
        }
        root["contents"] = contents;
        payload = QJsonDocument(root).toJson(QJsonDocument::Compact);

    } else {
        // OpenAI Compatible / Custom
        url = QUrl(m_endpoint + "/chat/completions");
        request.setUrl(url);
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
        payload = QJsonDocument(root).toJson(QJsonDocument::Compact);
    }

    m_generationTimer.start();
    m_currentReply = m_netManager->post(request, payload);

    connect(m_currentReply, &QNetworkReply::readyRead, this, &UniversalApiAdapter::onStreamReadyRead);
    connect(m_currentReply, &QNetworkReply::finished, this, &UniversalApiAdapter::onStreamFinished);
}

void UniversalApiAdapter::onStreamReadyRead() {
    if (!m_currentReply) return;

    m_streamBuffer.append(m_currentReply->readAll());

    if (m_protocol == ApiProtocol::Anthropic) {
        parseAnthropicStream();
    } else if (m_protocol == ApiProtocol::GoogleGemini) {
        parseGeminiStream();
    } else {
        parseOpenAiStream();
    }
}

void UniversalApiAdapter::parseOpenAiStream() {
    int newlineIdx = -1;
    while ((newlineIdx = m_streamBuffer.indexOf('\n')) != -1) {
        QByteArray line = m_streamBuffer.left(newlineIdx).trimmed();
        m_streamBuffer.remove(0, newlineIdx + 1);

        if (line.isEmpty()) continue;
        if (line.startsWith("data: ")) {
            line = line.mid(6).trimmed();
        }
        if (line == "[DONE]") continue;

        QJsonDocument doc = QJsonDocument::fromJson(line);
        if (!doc.isObject()) continue;

        QJsonObject obj = doc.object();
        QJsonArray choices = obj.value("choices").toArray();
        if (!choices.isEmpty()) {
            QJsonObject delta = choices[0].toObject().value("delta").toObject();
            if (delta.contains("content")) {
                QString chunk = delta.value("content").toString();
                if (!chunk.isEmpty()) {
                    m_accumulatedResponse.append(chunk);
                    emit streamChunkReceived(chunk);
                }
            }
        }

        if (obj.contains("usage")) {
            QJsonObject usage = obj.value("usage").toObject();
            m_promptTokens = usage.value("prompt_tokens").toInteger(0);
            m_completionTokens = usage.value("completion_tokens").toInteger(0);
        }
    }
}

void UniversalApiAdapter::parseAnthropicStream() {
    int newlineIdx = -1;
    while ((newlineIdx = m_streamBuffer.indexOf('\n')) != -1) {
        QByteArray line = m_streamBuffer.left(newlineIdx).trimmed();
        m_streamBuffer.remove(0, newlineIdx + 1);

        if (line.startsWith("data: ")) line = line.mid(6).trimmed();
        if (line.isEmpty()) continue;

        QJsonDocument doc = QJsonDocument::fromJson(line);
        if (!doc.isObject()) continue;
        QJsonObject obj = doc.object();

        QString type = obj.value("type").toString();
        if (type == "content_block_delta") {
            QString chunk = obj.value("delta").toObject().value("text").toString();
            if (!chunk.isEmpty()) {
                m_accumulatedResponse.append(chunk);
                emit streamChunkReceived(chunk);
            }
        }
    }
}

void UniversalApiAdapter::parseGeminiStream() {
    int newlineIdx = -1;
    while ((newlineIdx = m_streamBuffer.indexOf('\n')) != -1) {
        QByteArray line = m_streamBuffer.left(newlineIdx).trimmed();
        m_streamBuffer.remove(0, newlineIdx + 1);

        if (line.startsWith("data: ")) line = line.mid(6).trimmed();
        if (line.isEmpty()) continue;

        QJsonDocument doc = QJsonDocument::fromJson(line);
        if (!doc.isObject()) continue;
        QJsonObject obj = doc.object();

        QJsonArray candidates = obj.value("candidates").toArray();
        if (!candidates.isEmpty()) {
            QJsonArray parts = candidates[0].toObject().value("content").toObject().value("parts").toArray();
            for (const auto& p : parts) {
                QString chunk = p.toObject().value("text").toString();
                if (!chunk.isEmpty()) {
                    m_accumulatedResponse.append(chunk);
                    emit streamChunkReceived(chunk);
                }
            }
        }
    }
}

void UniversalApiAdapter::onStreamFinished() {
    if (!m_currentReply) return;

    QNetworkReply* reply = m_currentReply;
    m_currentReply = nullptr;
    reply->deleteLater();

    if (m_wasCancelled) {
        emit generationCancelled();
        return;
    }

    if (reply->error() != QNetworkReply::NoError) {
        emit errorOccurred("API Error", reply->errorString());
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
    finalMsg.completionTokens = m_completionTokens > 0 ? m_completionTokens : m_accumulatedResponse.split(' ', Qt::SkipEmptyParts).size();
    finalMsg.isExactMetrics = (m_completionTokens > 0);

    if (finalMsg.durationMs > 0 && finalMsg.completionTokens > 0) {
        finalMsg.tokensPerSec = (finalMsg.completionTokens * 1000.0) / finalMsg.durationMs;
    }

    emit generationFinished(finalMsg);
}

void UniversalApiAdapter::cancelActiveRequest() {
    if (m_currentReply) {
        m_wasCancelled = true;
        m_currentReply->abort();
        m_currentReply = nullptr;
    }
}

} // namespace Jarvis
