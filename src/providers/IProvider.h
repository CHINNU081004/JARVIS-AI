#pragma once
#include "Types.h"
#include <QObject>
#include <QList>
#include <functional>

namespace Jarvis {

class IProvider : public QObject {
    Q_OBJECT
public:
    explicit IProvider(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~IProvider() = default;

    virtual QString id() const = 0;
    virtual QString name() const = 0;
    virtual ProviderType type() const = 0;
    virtual QString endpoint() const = 0;
    virtual void setEndpoint(const QString& url) = 0;
    virtual void setApiKey(const QString& key) = 0;

    // Async operations
    virtual void testConnection() = 0;
    virtual void discoverModels() = 0;
    virtual void generateChatResponse(const QString& modelId,
                                      const QList<ChatMessage>& history,
                                      const QString& systemPrompt = QString()) = 0;
    virtual void cancelActiveRequest() = 0;
    virtual bool isBusy() const = 0;

signals:
    void connectionTested(bool success, qint64 latencyMs, const QString& errorMessage);
    void modelsDiscovered(const QList<ModelInfo>& models);
    void streamChunkReceived(const QString& textChunk);
    void generationFinished(const ChatMessage& finalMessage);
    void generationCancelled();
    void errorOccurred(const QString& errorTitle, const QString& errorDetails);
};

} // namespace Jarvis
