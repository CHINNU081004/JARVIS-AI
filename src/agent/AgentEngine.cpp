#include "AgentEngine.h"
#include <QUuid>
#include <QRegularExpression>
#include <QJsonDocument>
#include <QJsonObject>

namespace Jarvis {

AgentEngine::AgentEngine(ToolRegistry* registry, DatabaseManager* db, QObject* parent)
    : QObject(parent)
    , m_registry(registry)
    , m_db(db)
{
    m_stepTimer.setSingleShot(true);
    connect(&m_stepTimer, &QTimer::timeout, this, &AgentEngine::onStepTimeout);
}

void AgentEngine::setMaxSteps(int ms) {
    if (ms > 0 && ms != m_maxSteps) {
        m_maxSteps = ms;
        emit taskChanged();
    }
}

QString AgentEngine::pendingActionTitle() const {
    if (!m_hasPendingApproval) return QString();
    return QString("%1 (%2)").arg(m_pendingRequest.toolName, m_pendingRequest.reason);
}

void AgentEngine::startTask(const QString& goal, IProvider* provider, const QString& modelId, const QString& systemContext) {
    if (goal.trimmed().isEmpty()) return;

    m_currentTask = AgentTask();
    m_currentTask.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_currentTask.goal = goal;
    m_currentTask.status = AgentTaskStatus::Planning;
    m_currentTask.currentStep = 0;
    m_currentTask.maxSteps = m_maxSteps;
    m_currentTask.createdAt = QDateTime::currentDateTime();

    m_activeProvider = provider;
    m_activeModelId = modelId;
    m_systemContext = systemContext;
    m_turnHistory.clear();
    m_hasPendingApproval = false;
    m_activeThought = "Initializing autonomous agent planning loop...";

    if (m_db) {
        m_db->saveTask(m_currentTask);
    }

    emit statusChanged(m_currentTask.status);
    emit taskChanged();
    emit thoughtsChanged();

    // Hook provider signals
    if (m_activeProvider) {
        disconnect(m_activeProvider, &IProvider::streamChunkReceived, this, &AgentEngine::onProviderChunk);
        disconnect(m_activeProvider, &IProvider::generationFinished, this, &AgentEngine::onProviderFinished);
        disconnect(m_activeProvider, &IProvider::errorOccurred, this, &AgentEngine::onProviderError);

        connect(m_activeProvider, &IProvider::streamChunkReceived, this, &AgentEngine::onProviderChunk);
        connect(m_activeProvider, &IProvider::generationFinished, this, &AgentEngine::onProviderFinished);
        connect(m_activeProvider, &IProvider::errorOccurred, this, &AgentEngine::onProviderError);
    }

    executeNextModelTurn();
}

void AgentEngine::executeNextModelTurn() {
    if (m_currentTask.currentStep >= m_maxSteps) {
        completeTask(false, "Max step boundary reached without completion.", "Exceeded maximum allowed steps.");
        return;
    }

    m_currentTask.currentStep++;
    m_currentTask.status = AgentTaskStatus::Planning;
    m_accumulatedModelText.clear();
    emit statusChanged(m_currentTask.status);
    emit taskChanged();

    QString systemPrompt = "You are JARVIS Agent, an autonomous problem-solving AI desktop assistant.\n";
    if (!m_systemContext.isEmpty()) {
        systemPrompt += m_systemContext + "\n";
    }
    if (m_registry) {
        systemPrompt += m_registry->generateSystemPromptToolsSummary();
    }

    // Set 60-second watchdog timer per step
    m_stepTimer.start(60000);

    if (m_activeProvider) {
        m_activeProvider->generateChatResponse(m_activeModelId, m_turnHistory, systemPrompt);
    } else {
        emit requestModelInference(m_activeModelId, m_turnHistory, systemPrompt);
    }
}

void AgentEngine::onProviderChunk(const QString& chunk) {
    m_accumulatedModelText += chunk;
    m_activeThought = m_accumulatedModelText.trimmed();
    emit thoughtsChanged();
}

void AgentEngine::onProviderFinished(const ChatMessage& msg) {
    m_stepTimer.stop();
    QString fullText = msg.content.isEmpty() ? m_accumulatedModelText : msg.content;
    handleModelResponse(fullText);
}

void AgentEngine::onProviderError(const QString& title, const QString& detail) {
    m_stepTimer.stop();
    completeTask(false, QString("Provider Error: %1").arg(title), detail);
}

void AgentEngine::onStepTimeout() {
    completeTask(false, "Step execution timed out after 60 seconds.", "Watchdog timeout.");
}

bool AgentEngine::parseToolCall(const QString& text, ToolExecutionRequest* reqOut, QString* thoughtsOut) {
    // Look for ```json ... ``` blocks or raw JSON object containing "tool" and "arguments"
    static QRegularExpression jsonRegex(R"(```json\s*(\{[\s\S]*?\})\s*```)", QRegularExpression::CaseInsensitiveOption);
    auto match = jsonRegex.match(text);
    QString jsonStr;

    if (match.hasMatch()) {
        jsonStr = match.captured(1);
        if (thoughtsOut) {
            *thoughtsOut = text.left(match.capturedStart()).trimmed();
        }
    } else {
        // Attempt raw JSON detection
        int openBrace = text.indexOf('{');
        int closeBrace = text.lastIndexOf('}');
        if (openBrace != -1 && closeBrace > openBrace) {
            jsonStr = text.mid(openBrace, closeBrace - openBrace + 1);
            if (thoughtsOut) {
                *thoughtsOut = text.left(openBrace).trimmed();
            }
        }
    }

    if (jsonStr.isEmpty()) return false;

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) return false;

    QJsonObject obj = doc.object();
    if (!obj.contains("tool") || !obj["tool"].isString()) return false;

    if (reqOut) {
        reqOut->id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        reqOut->toolName = obj["tool"].toString();
        reqOut->arguments = obj["arguments"].toObject();
        reqOut->reason = obj["reason"].toString();
        reqOut->requestedAt = QDateTime::currentDateTime();

        auto tool = m_registry ? m_registry->getTool(reqOut->toolName) : nullptr;
        reqOut->permission = tool ? tool->permission() : ToolPermission::ReadOnly;

        // Pre-generate diff preview for edit_file or create_file or terminal commands
        if (reqOut->toolName == "create_file") {
            QString path = reqOut->arguments["path"].toString();
            QString content = reqOut->arguments["content"].toString();
            reqOut->diffPreview = QString("+++ [NEW FILE] %1 (%2 bytes)\n%3").arg(path).arg(content.size()).arg(content);
        } else if (reqOut->toolName == "edit_file") {
            QString path = reqOut->arguments["path"].toString();
            QString newContent = reqOut->arguments["new_content"].toString();
            reqOut->diffPreview = QString("@@ Edit Target: %1 @@\n%2").arg(path, newContent);
        } else if (reqOut->toolName == "delete_file") {
            reqOut->diffPreview = QString("--- [DELETION TARGET]: %1\n(Recovery backup will be preserved)").arg(reqOut->arguments["path"].toString());
        } else if (reqOut->toolName == "execute_terminal_command") {
            QString cmd = reqOut->arguments["command"].toString();
            QJsonArray args = reqOut->arguments["arguments"].toArray();
            QStringList argList;
            for (const auto& a : args) argList.append(a.toString());
            QString dir = reqOut->arguments["working_directory"].toString();
            reqOut->diffPreview = QString("Command: %1 %2\nDirectory: %3\n[Security Risk: Host Execution]")
                                      .arg(cmd, argList.join(" "), dir.isEmpty() ? "(workspace root)" : dir);
        } else if (reqOut->toolName == "terminate_process") {
            qint64 pid = reqOut->arguments["pid"].toInteger();
            bool force = reqOut->arguments["force"].toBool();
            reqOut->diffPreview = QString("Process Action: Terminate PID %1 (%2)")
                                      .arg(pid).arg(force ? "SIGKILL - Force" : "SIGTERM - Graceful");
        } else if (reqOut->toolName == "launch_application") {
            QString app = reqOut->arguments["application"].toString();
            reqOut->diffPreview = QString("Launch Installed Application: %1").arg(app);
        } else if (reqOut->toolName == "open_resource") {
            QString target = reqOut->arguments["target"].toString();
            reqOut->diffPreview = QString("Open System Resource: %1").arg(target);
        }
    }
    return true;
}

void AgentEngine::handleModelResponse(const QString& responseText) {
    m_stepTimer.stop();

    // Check for explicit completion
    if (responseText.contains("FINAL_ANSWER:")) {
        int idx = responseText.indexOf("FINAL_ANSWER:");
        QString finalAns = responseText.mid(idx + 13).trimmed();
        completeTask(true, finalAns);
        return;
    }

    ToolExecutionRequest req;
    QString thoughts;
    if (parseToolCall(responseText, &req, &thoughts)) {
        m_activeThought = thoughts.isEmpty() ? "Tool invocation requested." : thoughts;
        emit thoughtsChanged();

        // Validate arguments
        QString valErr;
        if (m_registry && !m_registry->validateToolCall(req.toolName, req.arguments, &valErr)) {
            // Append validation failure to turn history and allow model to correct itself
            ChatMessage assistMsg;
            assistMsg.role = "assistant";
            assistMsg.content = responseText;
            m_turnHistory.append(assistMsg);

            ChatMessage toolErr;
            toolErr.role = "user";
            toolErr.content = QString("[Tool Error]: Argument validation failed for '%1': %2. Please correct and retry.").arg(req.toolName, valErr);
            m_turnHistory.append(toolErr);

            executeNextModelTurn();
            return;
        }

        // Check permission policy
        if (req.permission == ToolPermission::RequiresApproval || req.permission == ToolPermission::DestructiveConfirm) {
            m_pendingRequest = req;
            m_hasPendingApproval = true;
            m_currentTask.status = AgentTaskStatus::AwaitingApproval;
            emit statusChanged(m_currentTask.status);
            emit approvalNeededChanged();
            return; // Wait for user approval
        }

        // Auto-run read-only tools
        proceedWithToolExecution(req);
    } else {
        // If neither a valid tool call nor FINAL_ANSWER was given, finish with the response
        completeTask(true, responseText.trimmed());
    }
}

void AgentEngine::approveAction() {
    if (!m_hasPendingApproval) return;
    m_hasPendingApproval = false;
    emit approvalNeededChanged();

    proceedWithToolExecution(m_pendingRequest);
}

void AgentEngine::rejectAction(const QString& reason) {
    if (!m_hasPendingApproval) return;
    m_hasPendingApproval = false;
    emit approvalNeededChanged();

    // Notify model of user rejection
    ChatMessage toolRejection;
    toolRejection.role = "user";
    toolRejection.content = QString("[Action Rejected by User]: %1. Reason: %2").arg(m_pendingRequest.toolName, reason);
    m_turnHistory.append(toolRejection);

    executeNextModelTurn();
}

void AgentEngine::proceedWithToolExecution(const ToolExecutionRequest& req) {
    m_currentTask.status = AgentTaskStatus::ExecutingTool;
    emit statusChanged(m_currentTask.status);

    ToolExecutionResult result;
    if (m_registry) {
        result = m_registry->executeTool(req.toolName, req.arguments);
    } else {
        result.success = false;
        result.errorMessage = "Tool registry not available.";
    }

    AgentTaskStep step;
    step.index = m_currentTask.currentStep;
    step.description = QString("Execute %1").arg(req.toolName);
    step.thoughts = m_activeThought;
    step.toolName = req.toolName;
    step.toolArguments = req.arguments;
    step.toolResult = result;
    step.approvalStatus = (req.permission == ToolPermission::ReadOnly) ? "AutoApproved" : "Approved";
    step.timestamp = QDateTime::currentDateTime();

    m_currentTask.steps.append(step);
    for (const auto& f : result.modifiedFiles) {
        if (!m_currentTask.filesChanged.contains(f)) {
            m_currentTask.filesChanged.append(f);
        }
        if (m_db) {
            QString backup = result.backupIds.isEmpty() ? QString() : result.backupIds.first();
            m_db->recordFileAudit(m_currentTask.id, f, req.toolName, backup);
        }
    }

    if (m_db && req.toolName == "execute_terminal_command") {
        QString cmd = req.arguments["command"].toString();
        QString dir = req.arguments["working_directory"].toString();
        m_db->recordTerminalAudit(m_currentTask.id, cmd, dir, result.success ? 0 : 1, 0, step.approvalStatus, result.output);
    }

    if (m_db) {
        m_db->recordTaskStep(m_currentTask.id, step);
    }

    emit stepCompleted(step.index, req.toolName, result.success ? result.output : result.errorMessage);

    // Append to turn history
    ChatMessage toolMsg;
    toolMsg.role = "user";
    if (result.success) {
        toolMsg.content = QString("[Tool '%1' Result]:\n%2").arg(req.toolName, result.output);
    } else {
        toolMsg.content = QString("[Tool '%1' Failed]: %2").arg(req.toolName, result.errorMessage);
    }
    m_turnHistory.append(toolMsg);

    // Proceed to next turn
    executeNextModelTurn();
}

void AgentEngine::cancelTask() {
    m_stepTimer.stop();
    m_hasPendingApproval = false;
    emit approvalNeededChanged();

    if (m_activeProvider) {
        m_activeProvider->cancelActiveRequest();
    }
    completeTask(false, "Task cancelled by user.", "Cancelled");
}

void AgentEngine::completeTask(bool success, const QString& summary, const QString& error) {
    m_stepTimer.stop();
    m_currentTask.finishedAt = QDateTime::currentDateTime();
    m_currentTask.status = success ? AgentTaskStatus::Completed : (error == "Cancelled" ? AgentTaskStatus::Cancelled : AgentTaskStatus::Failed);
    m_currentTask.summary = summary;
    m_currentTask.errorMessage = error;

    if (m_db) {
        m_db->updateTaskStatus(m_currentTask.id, m_currentTask.status, error, summary);
    }

    emit statusChanged(m_currentTask.status);
    emit taskChanged();
    emit taskFinished(success, summary);
}

} // namespace Jarvis
