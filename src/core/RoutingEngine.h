#pragma once
#include "Types.h"
#include <QObject>
#include <QList>
#include <QMap>

namespace Jarvis {

class RoutingEngine : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool autoRoutingEnabled READ isAutoRoutingEnabled WRITE setAutoRoutingEnabled NOTIFY autoRoutingChanged)
    Q_PROPERTY(QString fallbackModelId READ fallbackModelId WRITE setFallbackModelId NOTIFY fallbackModelChanged)

public:
    explicit RoutingEngine(QObject* parent = nullptr);

    bool isAutoRoutingEnabled() const { return m_autoRoutingEnabled; }
    void setAutoRoutingEnabled(bool enabled);

    QString fallbackModelId() const { return m_fallbackModelId; }
    void setFallbackModelId(const QString& modelId);

    // Deterministic rule-based model selection
    QString resolveModel(const QString& userSelection,
                         const QString& prompt,
                         const QList<ModelInfo>& availableModels,
                         QString* explanationOut = nullptr);

signals:
    void autoRoutingChanged();
    void fallbackModelChanged();
    void routingDecisionMade(const QString& chosenModel, const QString& reason);

private:
    bool m_autoRoutingEnabled = false; // Manual override by default per Rule 8
    QString m_fallbackModelId;
};

} // namespace Jarvis
