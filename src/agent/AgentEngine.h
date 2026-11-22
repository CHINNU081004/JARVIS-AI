#pragma once
#include "core/Types.h"
#include "ToolRegistry.h"
#include "providers/IProvider.h"
#include "storage/DatabaseManager.h"
#include <QObject>
#include <QTimer>
#include <memory>

namespace Jarvis {

class AgentEngine : public QObject {
    Q_OBJECT
    Q_PROPERTY(AgentTaskStatus status READ status NOTIFY statusChanged)
    Q_PROPERTY(QString currentGoal READ currentGoal NOTIFY taskChanged)
    Q_PROPERTY(int currentStep READ currentStep NOTIFY taskChanged)
    Q_PROPERTY(int maxSteps READ maxSteps WRITE setMaxSteps NOTIFY taskChanged)
    Q_PROPERTY(bool isAwaitingApproval READ isAwaitingApproval NOTIFY approvalNeededChanged)
    Q_PROPERTY(QString pendingActionTitle READ pendingActionTitle NOTIFY approvalNeededChanged)
    Q_PROPERTY(QString pendingActionDiff READ pendingActionDiff NOTIFY approvalNeededChanged)
    Q_PROPERTY(QString activeThought READ activeThought NOTIFY thoughtsChanged)

public:
    explicit AgentEngine(ToolRegistry* registry, DatabaseManager* db, QObject* parent = nullptr);

    AgentTaskStatus status() const { return m_currentTask.status; }
    QString currentGoal() const { return m_currentTask.goal; }
    int currentStep() const { return m_currentTask.currentStep; }
    int maxSteps() const { return m_maxSteps; }
    void setMaxSteps(int ms);

    bool isAwaitingApproval() const { return m_hasPendingApproval; }
    QString pendingActionTitle() const;
    QString pendingActionDiff() const { return m_pendingRequest.diffPreview; }
    QString activeThought() const { return m_activeThought; }

    Q_INVOKABLE void startTask(const QString& goal, IProvider* provider, const QString& modelId, const QString& systemContext = QString());
    Q_INVOKABLE void approveAction();
    Q_INVOKABLE void rejectAction(const QString& reason = "User rejected this tool execution.");
    Q_INVOKABLE void cancelTask();

    // Testing helper for injecting simulated model outputs
    void handleModelResponse(const QString& responseText);

signals:
    void statusChanged(AgentTaskStatus status);
    void taskChanged();
    void approvalNeededChanged();
    void thoughtsChanged();
    void stepCompleted(int stepIndex, const QString& toolName, const QString& result);
    void taskFinished(bool success, const QString& summary);
    void requestModelInference(const QString& modelId, const QList<ChatMessage>& history, const QString& systemPrompt);

private slots:
    void onProviderChunk(const QString& chunk);
    void onProviderFinished(const ChatMessage& msg);
    void onProviderError(const QString& title, const QString& detail);
    void onStepTimeout();

private:
    void executeNextModelTurn();
    bool parseToolCall(const QString& text, ToolExecutionRequest* reqOut, QString* thoughtsOut);
    void proceedWithToolExecution(const ToolExecutionRequest& req);
    void completeTask(bool success, const QString& summary, const QString& error = QString());

    ToolRegistry* m_registry;
    DatabaseManager* m_db;
    IProvider* m_activeProvider = nullptr;
    QString m_activeModelId;
    QString m_systemContext;

    AgentTask m_currentTask;
    int m_maxSteps = 10;
    QString m_accumulatedModelText;
    QString m_activeThought;

    bool m_hasPendingApproval = false;
    ToolExecutionRequest m_pendingRequest;

    QList<ChatMessage> m_turnHistory;
    QTimer m_stepTimer;
};

} // namespace Jarvis
