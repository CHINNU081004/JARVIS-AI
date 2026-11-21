#pragma once
#include "IProvider.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QElapsedTimer>

namespace Jarvis {

class UniversalApiAdapter : public IProvider {
    Q_OBJECT
public:
    explicit UniversalApiAdapter(const QString& providerId = "universal_api",
                                 const QString& endpoint = "https://api.openai.com/v1",
                                 ApiProtocol protocol = ApiProtocol::OpenAICompatible,
                                 QObject* parent = nullptr);
    ~UniversalApiAdapter() override;

    QString id() const override { return m_id; }
    QString name() const override { return "API"; }
    ProviderType type() const override { return ProviderType::UniversalApi; }
    QString endpoint() const override { return m_endpoint; }
    void setEndpoint(const QString& url) override;
    void setApiKey(const QString& key) override { m_apiKey = key; }

    ApiProtocol protocol() const { return m_protocol; }
    void setProtocol(ApiProtocol proto) { m_protocol = proto; }

    void testConnection() override;
    void discoverModels() override;
    void generateChatResponse(const QString& modelId,
                              const QList<ChatMessage>& history,
                              const QString& systemPrompt = QString()) override;
    void cancelActiveRequest() override;
    bool isBusy() const override { return m_currentReply != nullptr; }

    static QString protocolToString(ApiProtocol p);
    static ApiProtocol stringToProtocol(const QString& str);

private slots:
    void onStreamReadyRead();
    void onStreamFinished();

private:
    void parseOpenAiStream();
    void parseAnthropicStream();
    void parseGeminiStream();

    QString m_id;
    QString m_endpoint;
    ApiProtocol m_protocol = ApiProtocol::OpenAICompatible;
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

// Typedef for backwards compatibility
using GenericApiAdapter = UniversalApiAdapter;

} // namespace Jarvis
