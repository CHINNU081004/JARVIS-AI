#pragma once
#include "IProvider.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QElapsedTimer>

namespace Jarvis {

class GenericApiAdapter : public IProvider {
    Q_OBJECT
public:
    explicit GenericApiAdapter(const QString& providerId = "generic_api",
                               const QString& endpoint = "https://api.openai.com/v1",
                               QObject* parent = nullptr);
    ~GenericApiAdapter() override;

    QString id() const override { return m_id; }
    QString name() const override { return "OpenAI-Compatible API"; }
    ProviderType type() const override { return ProviderType::GenericOpenAICompatible; }
    QString endpoint() const override { return m_endpoint; }
    void setEndpoint(const QString& url) override;
    void setApiKey(const QString& key) override { m_apiKey = key; }

    void testConnection() override;
    void discoverModels() override;
    void generateChatResponse(const QString& modelId,
                              const QList<ChatMessage>& history,
                              const QString& systemPrompt = QString()) override;
    void cancelActiveRequest() override;
    bool isBusy() const override { return m_currentReply != nullptr; }

private slots:
    void onStreamReadyRead();
    void onStreamFinished();

private:
    QString m_id;
    QString m_endpoint;
    QString m_apiKey;
    QNetworkAccessManager* m_netManager;
    QNetworkReply* m_currentReply = nullptr;
    QElapsedTimer m_generationTimer;
    QString m_activeModelId;
    QString m_accumulatedResponse;
    QByteArray m_streamBuffer;
    qint64 m_promptTokens = 0;
    qint64 m_completionTokens = 0;
    bool m_wasCancelled = false;
};

} // namespace Jarvis
