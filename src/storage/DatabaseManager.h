#pragma once
#include "Types.h"
#include <QObject>
#include <QSqlDatabase>
#include <QList>
#include <QMutex>

namespace Jarvis {

class DatabaseManager : public QObject {
    Q_OBJECT
public:
    explicit DatabaseManager(const QString& dbPath = QString(), QObject* parent = nullptr);
    ~DatabaseManager() override;

    bool initialize();

    // Conversations
    QString createConversation(const QString& title, const QString& modelId, const QString& providerId);
    QList<QPair<QString, QString>> getConversations(); // id, title
    bool updateConversationTitle(const QString& convId, const QString& title);
    bool deleteConversation(const QString& convId);

    // Messages
    bool saveMessage(const ChatMessage& msg);
    QList<ChatMessage> getMessages(const QString& convId);
    QList<ChatMessage> searchHistory(const QString& query);

    // Memories (Bounded, relevant)
    bool saveMemory(const MemoryItem& item);
    bool updateMemory(const QString& id, const QString& content, bool isPinned);
    bool deleteMemory(const QString& id);
    QList<MemoryItem> getAllMemories();
    QList<MemoryItem> getRelevantMemories(const QString& query, int maxResults = 5);

    // Activity Log
    bool logActivity(const ActivityEvent& event);
    QList<ActivityEvent> getRecentActivities(int limit = 50);

    // Settings
    bool saveSetting(const QString& key, const QString& value);
    QString getSetting(const QString& key, const QString& defaultValue = QString());

private:
    QString m_dbPath;
    QSqlDatabase m_db;
    QMutex m_mutex;
};

} // namespace Jarvis
