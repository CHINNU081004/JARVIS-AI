#include "GenericApiAdapter.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QNetworkRequest>

namespace Jarvis {

GenericApiAdapter::GenericApiAdapter(const QString& providerId,
                                     const QString& endpoint,
                                     QObject* parent)
    : IProvider(parent)
    , m_id(providerId)
    , m_endpoint(endpoint)
    , m_netManager(new QNetworkAccessManager(this))
{
}

GenericApiAdapter::~GenericApiAdapter() {
    cancelActiveRequest();
}

void GenericApiAdapter::setEndpoint(const QString& url) {
    m_endpoint = url.trimmed();
    if (m_endpoint.endsWith('/')) {
        m_endpoint.chop(1);
    }
}

void GenericApiAdapter::testConnection() {
    QUrl url(m_endpoint + "/models");
    QNetworkRequest request(url);
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
            emit connectionTested(false, latency, reply->errorString());
        }
    });
}

void GenericApiAdapter::discoverModels() {
    QUrl url(m_endpoint + "/models");
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
            emit errorOccurred("Invalid Response", "Failed to parse API models response.");
            return;
        }

        QJsonArray modelsArray = doc.object().value("data").toArray();
        QList<ModelInfo> discovered;

        for (const auto& item : modelsArray) {
            QJsonObject mObj = item.toObject();
            ModelInfo info;
            info.id = mObj.value("id").toString();
            info.name = info.id;
            info.providerId = m_id;
            info.capabilities.streaming = CapabilityState::Supported;
            info.capabilities.toolCalling = CapabilityState::Supported;
            info.capabilities.exactMetrics = false; // Reported in usage event if provided

            discovered.append(info);
        }

        emit modelsDiscovered(discovered);
    });
}

void GenericApiAdapter::generateChatResponse(const QString& modelId,
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

    QUrl url(m_endpoint + "/chat/completions");
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

    connect(m_currentReply, &QNetworkReply::readyRead, this, &GenericApiAdapter::onStreamReadyRead);
    connect(m_currentReply, &QNetworkReply::finished, this, &GenericApiAdapter::onStreamFinished);
}

void GenericApiAdapter::onStreamReadyRead() {
    if (!m_currentReply) return;

    m_streamBuffer.append(m_currentReply->readAll());

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

void GenericApiAdapter::onStreamFinished() {
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

void GenericApiAdapter::cancelActiveRequest() {
    if (m_currentReply) {
        m_wasCancelled = true;
        m_currentReply->abort();
        m_currentReply = nullptr;
    }
}

} // namespace Jarvis
