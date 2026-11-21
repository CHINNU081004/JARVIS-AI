#pragma once
#include "IProvider.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QElapsedTimer>

namespace Jarvis {

class OllamaAdapter : public IProvider {
    Q_OBJECT
public:
    explicit OllamaAdapter(const QString& providerId = "ollama_local",
                         const QString& endpoint = "http://127.0.0.1:11434",
                         QObject* parent = nullptr);
    ~OllamaAdapter() override;

    QString id() const override { return m_id; }
    QString name() const override { return m_name; }
    ProviderType type() const override { return m_type; }
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
    QString m_name;
    ProviderType m_type;
    QString m_endpoint;
    QString m_apiKey;
    QNetworkAccessManager* m_netManager;
    QNetworkReply* m_currentReply = nullptr;
    QElapsedTimer m_generationTimer;
    QString m_activeModelId;
    QString m_accumulatedResponse;
    qint64 m_promptTokens = 0;
    qint64 m_evalTokens = 0;
    qint64 m_evalDurationNs = 0;
    bool m_wasCancelled = false;
};

} // namespace Jarvis
