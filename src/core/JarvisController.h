#pragma once
#include "Types.h"
#include "providers/IProvider.h"
#include "providers/OllamaAdapter.h"
#include "providers/GenericApiAdapter.h"
#include "storage/DatabaseManager.h"
#include "workspace/WorkspaceManager.h"
#include "core/MetricsCollector.h"
#include "core/RoutingEngine.h"

#include <QObject>
#include <QAbstractListModel>
#include <QList>
#include <QMap>

namespace Jarvis {

class ChatMessageModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles {
        RoleRole = Qt::UserRole + 1,
        ContentRole,
        TimestampRole,
        ModelRole,
        DurationRole,
        TpsRole,
        IsExactRole,
        IsErrorRole,
        IsCancelledRole
    };

    explicit ChatMessageModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setMessages(const QList<ChatMessage>& messages);
    void appendMessage(const ChatMessage& message);
    void updateLastAssistantChunk(const QString& chunk);
    void updateLastAssistantComplete(const ChatMessage& completeMessage);
    void clear();

private:
    QList<ChatMessage> m_messages;
};

class ActivityModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles {
        TimestampRole = Qt::UserRole + 1,
        LevelRole,
        TitleRole,
        DetailRole,
        CategoryRole
    };

    explicit ActivityModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void addActivity(const ActivityEvent& event);
    void setActivities(const QList<ActivityEvent>& list);

private:
    QList<ActivityEvent> m_events;
};

class JarvisController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString appVersion READ appVersion CONSTANT)
    Q_PROPERTY(bool isGenerating READ isGenerating NOTIFY isGeneratingChanged)
    Q_PROPERTY(QString currentConversationId READ currentConversationId NOTIFY currentConversationChanged)
    Q_PROPERTY(QString currentConversationTitle READ currentConversationTitle NOTIFY currentConversationChanged)
    Q_PROPERTY(QString selectedProviderId READ selectedProviderId WRITE selectProvider NOTIFY providerChanged)
    Q_PROPERTY(QString selectedModelId READ selectedModelId WRITE selectModel NOTIFY modelChanged)
    Q_PROPERTY(bool isProviderConnected READ isProviderConnected NOTIFY providerConnectionChanged)
    Q_PROPERTY(QString activeTaskDescription READ activeTaskDescription NOTIFY activeTaskChanged)
    Q_PROPERTY(QString activeTaskStep READ activeTaskStep NOTIFY activeTaskChanged)
    Q_PROPERTY(int animationIntensity READ animationIntensity WRITE setAnimationIntensity NOTIFY animationIntensityChanged)
    Q_PROPERTY(bool particlesEnabled READ particlesEnabled WRITE setParticlesEnabled NOTIFY animationIntensityChanged)

    Q_PROPERTY(ChatMessageModel* chatModel READ chatModel CONSTANT)
    Q_PROPERTY(ActivityModel* activityModel READ activityModel CONSTANT)
    Q_PROPERTY(WorkspaceManager* workspace READ workspace CONSTANT)
    Q_PROPERTY(MetricsCollector* metrics READ metrics CONSTANT)
    Q_PROPERTY(RoutingEngine* router READ router CONSTANT)

public:
    explicit JarvisController(QObject* parent = nullptr);
    ~JarvisController() override;

    bool initialize();

    QString appVersion() const { return "1.0.0"; }
    bool isGenerating() const { return m_isGenerating; }
    QString currentConversationId() const { return m_currentConversationId; }
    QString currentConversationTitle() const { return m_currentConversationTitle; }
    QString selectedProviderId() const { return m_selectedProviderId; }
    QString selectedModelId() const { return m_selectedModelId; }
    bool isProviderConnected() const;

    QString activeTaskDescription() const { return m_activeTaskDescription; }
    QString activeTaskStep() const { return m_activeTaskStep; }

    int animationIntensity() const { return m_animationIntensity; }
    void setAnimationIntensity(int level);

    bool particlesEnabled() const { return m_particlesEnabled; }
    void setParticlesEnabled(bool enabled);

    ChatMessageModel* chatModel() { return &m_chatModel; }
    ActivityModel* activityModel() { return &m_activityModel; }
    WorkspaceManager* workspace() { return &m_workspace; }
    MetricsCollector* metrics() { return &m_metrics; }
    RoutingEngine* router() { return &m_router; }

    Q_INVOKABLE QVariantList getDiscoveredModels();
    Q_INVOKABLE QVariantList getConversations();
    Q_INVOKABLE void selectProvider(const QString& providerId);
    Q_INVOKABLE void selectModel(const QString& modelId);
    Q_INVOKABLE void testActiveProvider();
    Q_INVOKABLE void refreshModels();
    Q_INVOKABLE void updateProviderEndpoint(const QString& providerId, const QString& newUrl, const QString& apiKey = QString());

    Q_INVOKABLE void createNewConversation(const QString& title = "New Chat");
    Q_INVOKABLE void loadConversation(const QString& convId);
    Q_INVOKABLE void deleteConversation(const QString& convId);

    Q_INVOKABLE void sendMessage(const QString& text);
    Q_INVOKABLE void cancelGeneration();

    // Memory APIs
    Q_INVOKABLE QVariantList getAllMemories();
    Q_INVOKABLE void addMemory(const QString& key, const QString& content, const QString& category, bool pinned);
    Q_INVOKABLE void deleteMemory(const QString& id);

    // Activity Log
    void logEvent(const QString& level, const QString& title, const QString& detail, const QString& category);

signals:
    void isGeneratingChanged();
    void currentConversationChanged();
    void providerChanged();
    void modelChanged();
    void providerConnectionChanged();
    void activeTaskChanged();
    void animationIntensityChanged();
    void modelsListChanged();
    void errorNotice(const QString& title, const QString& message);

private slots:
    void onConnectionTested(bool success, qint64 latencyMs, const QString& error);
    void onModelsDiscovered(const QList<ModelInfo>& models);
    void onStreamChunk(const QString& chunk);
    void onGenerationFinished(const ChatMessage& message);
    void onGenerationCancelled();
    void onProviderError(const QString& title, const QString& detail);

private:
    IProvider* currentProvider() const;

    ChatMessageModel m_chatModel;
    ActivityModel m_activityModel;
    WorkspaceManager m_workspace;
    MetricsCollector m_metrics;
    RoutingEngine m_router;
    DatabaseManager m_db;

    QMap<QString, IProvider*> m_providers;
    QList<ModelInfo> m_allModels;

    QString m_selectedProviderId = "ollama_local";
    QString m_selectedModelId;
    QString m_currentConversationId;
    QString m_currentConversationTitle = "New Chat";
    QString m_activeTaskDescription = "Ready";
    QString m_activeTaskStep = "Idle";

    bool m_isGenerating = false;
    int m_animationIntensity = 2; // 0=Off, 1=Reduced, 2=Full
    bool m_particlesEnabled = true;
};

} // namespace Jarvis
