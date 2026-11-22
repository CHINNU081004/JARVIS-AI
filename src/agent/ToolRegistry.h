#pragma once
#include "ITool.h"
#include <QObject>
#include <QMap>
#include <memory>
#include <QJsonArray>

namespace Jarvis {

class ToolRegistry : public QObject {
    Q_OBJECT
public:
    explicit ToolRegistry(QObject* parent = nullptr);

    void registerTool(std::shared_ptr<ITool> tool);
    std::shared_ptr<ITool> getTool(const QString& name) const;
    QStringList registeredToolNames() const;

    QJsonArray getToolSchemas() const;
    QString generateSystemPromptToolsSummary() const;

    bool validateToolCall(const QString& name, const QJsonObject& args, QString* errorOut = nullptr) const;
    ToolExecutionResult executeTool(const QString& name, const QJsonObject& args);

private:
    QMap<QString, std::shared_ptr<ITool>> m_tools;
};

} // namespace Jarvis
