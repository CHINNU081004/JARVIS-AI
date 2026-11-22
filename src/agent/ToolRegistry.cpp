#include "ToolRegistry.h"
#include <QJsonDocument>

namespace Jarvis {

ToolRegistry::ToolRegistry(QObject* parent)
    : QObject(parent)
{
}

void ToolRegistry::registerTool(std::shared_ptr<ITool> tool) {
    if (tool) {
        m_tools[tool->name()] = tool;
    }
}

std::shared_ptr<ITool> ToolRegistry::getTool(const QString& name) const {
    return m_tools.value(name, nullptr);
}

QStringList ToolRegistry::registeredToolNames() const {
    return m_tools.keys();
}

QJsonArray ToolRegistry::getToolSchemas() const {
    QJsonArray array;
    for (const auto& tool : m_tools) {
        QJsonObject obj;
        obj["name"] = tool->name();
        obj["description"] = tool->description();
        obj["parameters"] = tool->parameterSchema();
        array.append(obj);
    }
    return array;
}

QString ToolRegistry::generateSystemPromptToolsSummary() const {
    QString summary = "You have access to the following tools:\n";
    for (const auto& tool : m_tools) {
        summary += QString("- `%1`: %2\n").arg(tool->name(), tool->description());
        summary += QString("  Parameters: %1\n").arg(QString::fromUtf8(QJsonDocument(tool->parameterSchema()).toJson(QJsonDocument::Compact)));
    }
    summary += "\nTo invoke a tool, respond with ONLY a structured JSON block in this exact format:\n";
    summary += "```json\n{\n  \"tool\": \"<tool_name>\",\n  \"arguments\": { ... },\n  \"reason\": \"<why you are calling this tool>\"\n}\n```\n";
    summary += "If no further tools are needed and the task is complete, provide your final response starting with `FINAL_ANSWER:`.\n";
    return summary;
}

bool ToolRegistry::validateToolCall(const QString& name, const QJsonObject& args, QString* errorOut) const {
    auto tool = getTool(name);
    if (!tool) {
        if (errorOut) *errorOut = QString("Tool '%1' is not registered.").arg(name);
        return false;
    }
    return tool->validateArguments(args, errorOut);
}

ToolExecutionResult ToolRegistry::executeTool(const QString& name, const QJsonObject& args) {
    auto tool = getTool(name);
    if (!tool) {
        ToolExecutionResult res;
        res.success = false;
        res.errorMessage = QString("Tool '%1' not found.").arg(name);
        return res;
    }
    return tool->execute(args);
}

} // namespace Jarvis
