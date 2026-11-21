#include "RoutingEngine.h"

namespace Jarvis {

RoutingEngine::RoutingEngine(QObject* parent)
    : QObject(parent)
{
}

void RoutingEngine::setAutoRoutingEnabled(bool enabled) {
    if (m_autoRoutingEnabled != enabled) {
        m_autoRoutingEnabled = enabled;
        emit autoRoutingChanged();
    }
}

void RoutingEngine::setFallbackModelId(const QString& modelId) {
    if (m_fallbackModelId != modelId) {
        m_fallbackModelId = modelId;
        emit fallbackModelChanged();
    }
}

QString RoutingEngine::resolveModel(const QString& userSelection,
                                   const QString& prompt,
                                   const QList<ModelInfo>& availableModels,
                                   QString* explanationOut)
{
    // 1. If auto-routing is disabled or an explicit valid user selection exists, honor it
    if (!m_autoRoutingEnabled && !userSelection.isEmpty()) {
        if (explanationOut) *explanationOut = "Using manual model selection.";
        emit routingDecisionMade(userSelection, "Manual user override active.");
        return userSelection;
    }

    if (availableModels.isEmpty()) {
        if (explanationOut) *explanationOut = "No discovered models available.";
        return userSelection;
    }

    // 2. Rule-based capability routing
    QString promptLower = prompt.toLower();
    bool requiresVision = promptLower.contains("image") || promptLower.contains("photo") || promptLower.contains("screenshot") || promptLower.contains("picture");
    bool isCodingTask = promptLower.contains("code") || promptLower.contains("python") || promptLower.contains("function") || promptLower.contains("c++") || promptLower.contains("script");

    // Match vision capability if requested
    if (requiresVision) {
        for (const auto& m : availableModels) {
            if (m.capabilities.vision == CapabilityState::Supported) {
                QString reason = QString("Selected %1 for vision-related task.").arg(m.name);
                if (explanationOut) *explanationOut = reason;
                emit routingDecisionMade(m.id, reason);
                return m.id;
            }
        }
    }

    // Match coding specialization if available (e.g. qwen2.5-coder, codellama, deepseek-coder)
    if (isCodingTask) {
        for (const auto& m : availableModels) {
            QString idLower = m.id.toLower();
            if (idLower.contains("code") || idLower.contains("coder")) {
                QString reason = QString("Selected %1 for coding/scripting task.").arg(m.name);
                if (explanationOut) *explanationOut = reason;
                emit routingDecisionMade(m.id, reason);
                return m.id;
            }
        }
    }

    // Default to user selection if valid in available models
    for (const auto& m : availableModels) {
        if (m.id == userSelection) {
            if (explanationOut) *explanationOut = "Using selected model.";
            return userSelection;
        }
    }

    // Fallback to first available model
    QString fallback = availableModels.first().id;
    QString reason = QString("Defaulting to available model %1.").arg(fallback);
    if (explanationOut) *explanationOut = reason;
    emit routingDecisionMade(fallback, reason);
    return fallback;
}

} // namespace Jarvis
