#include "DatabaseManager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QStandardPaths>
#include <QDir>
#include <QUuid>
#include <QVariant>
#include <algorithm>

namespace Jarvis {

DatabaseManager::DatabaseManager(const QString& dbPath, QObject* parent)
    : QObject(parent)
    , m_dbPath(dbPath)
{
    if (m_dbPath.isEmpty()) {
        QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir().mkpath(dataDir);
        m_dbPath = dataDir + "/jarvis.db";
    }
}

DatabaseManager::~DatabaseManager() {
    if (m_db.isOpen()) {
        m_db.close();
    }
}

bool DatabaseManager::initialize() {
    QMutexLocker locker(&m_mutex);

    QString connectionName = "jarvis_db_" + QUuid::createUuid().toString();
    m_db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    m_db.setDatabaseName(m_dbPath);

    if (!m_db.open()) {
        return false;
    }

    QSqlQuery q(m_db);
    // Schema creation
    q.exec("CREATE TABLE IF NOT EXISTS conversations ("
           "id TEXT PRIMARY KEY, "
           "title TEXT, "
           "model_id TEXT, "
           "provider_id TEXT, "
           "created_at DATETIME, "
           "updated_at DATETIME"
           ")");

    q.exec("CREATE TABLE IF NOT EXISTS messages ("
           "id TEXT PRIMARY KEY, "
           "conversation_id TEXT, "
           "role TEXT, "
           "content TEXT, "
           "model_used TEXT, "
           "provider_used TEXT, "
           "created_at DATETIME, "
           "prompt_tokens INTEGER, "
           "completion_tokens INTEGER, "
           "duration_ms INTEGER, "
           "tokens_per_sec REAL, "
           "is_exact_metrics INTEGER, "
           "is_error INTEGER, "
           "is_cancelled INTEGER, "
           "FOREIGN KEY(conversation_id) REFERENCES conversations(id)"
           ")");

    q.exec("CREATE TABLE IF NOT EXISTS memories ("
           "id TEXT PRIMARY KEY, "
           "mem_key TEXT, "
           "content TEXT, "
           "category TEXT, "
           "created_at DATETIME, "
           "is_pinned INTEGER"
           ")");

    q.exec("CREATE TABLE IF NOT EXISTS activities ("
           "id TEXT PRIMARY KEY, "
           "created_at DATETIME, "
           "level TEXT, "
           "title TEXT, "
           "detail TEXT, "
           "category TEXT"
           ")");

    q.exec("CREATE TABLE IF NOT EXISTS settings ("
           "key TEXT PRIMARY KEY, "
           "value TEXT"
           ")");

    return true;
}

QString DatabaseManager::createConversation(const QString& title, const QString& modelId, const QString& providerId) {
    QMutexLocker locker(&m_mutex);
    QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO conversations (id, title, model_id, provider_id, created_at, updated_at) "
              "VALUES (:id, :title, :model, :provider, :now, :now)");
    q.bindValue(":id", id);
    q.bindValue(":title", title);
    q.bindValue(":model", modelId);
    q.bindValue(":provider", providerId);
    q.bindValue(":now", QDateTime::currentDateTime());
    q.exec();
    return id;
}

QList<QPair<QString, QString>> DatabaseManager::getConversations() {
    QMutexLocker locker(&m_mutex);
    QList<QPair<QString, QString>> list;
    QSqlQuery q("SELECT id, title FROM conversations ORDER BY updated_at DESC", m_db);
    while (q.next()) {
        list.append({q.value(0).toString(), q.value(1).toString()});
    }
    return list;
}

bool DatabaseManager::updateConversationTitle(const QString& convId, const QString& title) {
    QMutexLocker locker(&m_mutex);
    QSqlQuery q(m_db);
    q.prepare("UPDATE conversations SET title = :title, updated_at = :now WHERE id = :id");
    q.bindValue(":title", title);
    q.bindValue(":now", QDateTime::currentDateTime());
    q.bindValue(":id", convId);
    return q.exec();
}

bool DatabaseManager::deleteConversation(const QString& convId) {
    QMutexLocker locker(&m_mutex);
    QSqlQuery q1(m_db);
    q1.prepare("DELETE FROM messages WHERE conversation_id = :id");
    q1.bindValue(":id", convId);
    q1.exec();

    QSqlQuery q2(m_db);
    q2.prepare("DELETE FROM conversations WHERE id = :id");
    q2.bindValue(":id", convId);
    return q2.exec();
}

bool DatabaseManager::saveMessage(const ChatMessage& msg) {
    QMutexLocker locker(&m_mutex);
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO messages (id, conversation_id, role, content, model_used, provider_used, "
              "created_at, prompt_tokens, completion_tokens, duration_ms, tokens_per_sec, "
              "is_exact_metrics, is_error, is_cancelled) VALUES "
              "(:id, :cid, :role, :content, :model, :provider, :created, :p_tok, :c_tok, :dur, :tps, :exact, :err, :canc)");
    QString msgId = msg.id.isEmpty() ? QUuid::createUuid().toString(QUuid::WithoutBraces) : msg.id;
    q.bindValue(":id", msgId);
    q.bindValue(":cid", msg.conversationId);
    q.bindValue(":role", msg.role);
    q.bindValue(":content", msg.content);
    q.bindValue(":model", msg.modelUsed);
    q.bindValue(":provider", msg.providerUsed);
    q.bindValue(":created", msg.timestamp.isValid() ? msg.timestamp : QDateTime::currentDateTime());
    q.bindValue(":p_tok", msg.promptTokens);
    q.bindValue(":c_tok", msg.completionTokens);
    q.bindValue(":dur", msg.durationMs);
    q.bindValue(":tps", msg.tokensPerSec);
    q.bindValue(":exact", msg.isExactMetrics ? 1 : 0);
    q.bindValue(":err", msg.isError ? 1 : 0);
    q.bindValue(":canc", msg.isCancelled ? 1 : 0);

    bool ok = q.exec();
    if (ok) {
        QSqlQuery updateConv(m_db);
        updateConv.prepare("UPDATE conversations SET updated_at = :now WHERE id = :cid");
        updateConv.bindValue(":now", QDateTime::currentDateTime());
        updateConv.bindValue(":cid", msg.conversationId);
        updateConv.exec();
    }
    return ok;
}

QList<ChatMessage> DatabaseManager::getMessages(const QString& convId) {
    QMutexLocker locker(&m_mutex);
    QList<ChatMessage> list;
    QSqlQuery q(m_db);
    q.prepare("SELECT id, conversation_id, role, content, model_used, provider_used, created_at, "
              "prompt_tokens, completion_tokens, duration_ms, tokens_per_sec, is_exact_metrics, "
              "is_error, is_cancelled FROM messages WHERE conversation_id = :cid ORDER BY created_at ASC");
    q.bindValue(":cid", convId);
    q.exec();

    while (q.next()) {
        ChatMessage msg;
        msg.id = q.value(0).toString();
        msg.conversationId = q.value(1).toString();
        msg.role = q.value(2).toString();
        msg.content = q.value(3).toString();
        msg.modelUsed = q.value(4).toString();
        msg.providerUsed = q.value(5).toString();
        msg.timestamp = q.value(6).toDateTime();
        msg.promptTokens = q.value(7).toLongLong();
        msg.completionTokens = q.value(8).toLongLong();
        msg.durationMs = q.value(9).toLongLong();
        msg.tokensPerSec = q.value(10).toDouble();
        msg.isExactMetrics = q.value(11).toInt() != 0;
        msg.isError = q.value(12).toInt() != 0;
        msg.isCancelled = q.value(13).toInt() != 0;
        list.append(msg);
    }
    return list;
}

QList<ChatMessage> DatabaseManager::searchHistory(const QString& query) {
    QMutexLocker locker(&m_mutex);
    QList<ChatMessage> list;
    QSqlQuery q(m_db);
    q.prepare("SELECT id, conversation_id, role, content, model_used, provider_used, created_at "
              "FROM messages WHERE content LIKE :q ORDER BY created_at DESC LIMIT 50");
    q.bindValue(":q", "%" + query + "%");
    q.exec();
    while (q.next()) {
        ChatMessage msg;
        msg.id = q.value(0).toString();
        msg.conversationId = q.value(1).toString();
        msg.role = q.value(2).toString();
        msg.content = q.value(3).toString();
        msg.modelUsed = q.value(4).toString();
        msg.providerUsed = q.value(5).toString();
        msg.timestamp = q.value(6).toDateTime();
        list.append(msg);
    }
    return list;
}

bool DatabaseManager::saveMemory(const MemoryItem& item) {
    QMutexLocker locker(&m_mutex);
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO memories (id, mem_key, content, category, created_at, is_pinned) "
              "VALUES (:id, :key, :content, :cat, :created, :pinned)");
    q.bindValue(":id", item.id.isEmpty() ? QUuid::createUuid().toString(QUuid::WithoutBraces) : item.id);
    q.bindValue(":key", item.key);
    q.bindValue(":content", item.content);
    q.bindValue(":cat", item.category);
    q.bindValue(":created", item.timestamp.isValid() ? item.timestamp : QDateTime::currentDateTime());
    q.bindValue(":pinned", item.isPinned ? 1 : 0);
    return q.exec();
}

bool DatabaseManager::updateMemory(const QString& id, const QString& content, bool isPinned) {
    QMutexLocker locker(&m_mutex);
    QSqlQuery q(m_db);
    q.prepare("UPDATE memories SET content = :content, is_pinned = :pinned WHERE id = :id");
    q.bindValue(":content", content);
    q.bindValue(":pinned", isPinned ? 1 : 0);
    q.bindValue(":id", id);
    return q.exec();
}

bool DatabaseManager::deleteMemory(const QString& id) {
    QMutexLocker locker(&m_mutex);
    QSqlQuery q(m_db);
    q.prepare("DELETE FROM memories WHERE id = :id");
    q.bindValue(":id", id);
    return q.exec();
}

QList<MemoryItem> DatabaseManager::getAllMemories() {
    QMutexLocker locker(&m_mutex);
    QList<MemoryItem> list;
    QSqlQuery q("SELECT id, mem_key, content, category, created_at, is_pinned FROM memories ORDER BY is_pinned DESC, created_at DESC", m_db);
    while (q.next()) {
        MemoryItem mem;
        mem.id = q.value(0).toString();
        mem.key = q.value(1).toString();
        mem.content = q.value(2).toString();
        mem.category = q.value(3).toString();
        mem.timestamp = q.value(4).toDateTime();
        mem.isPinned = q.value(5).toInt() != 0;
        list.append(mem);
    }
    return list;
}

QList<MemoryItem> DatabaseManager::getRelevantMemories(const QString& query, int maxResults) {
    // Lightweight keyword & recency relevance scoring
    QList<MemoryItem> all = getAllMemories();
    if (all.isEmpty()) return {};

    QStringList keywords = query.toLower().split(QRegularExpression("[\\s,;:.!?]+"), Qt::SkipEmptyParts);

    for (auto& item : all) {
        double score = 0.0;
        if (item.isPinned) score += 3.0; // Pinned memories receive high priority

        QString itemText = (item.key + " " + item.content).toLower();
        for (const QString& kw : keywords) {
            if (kw.length() < 3) continue;
            if (itemText.contains(kw)) {
                score += 1.5;
            }
        }
        item.relevanceScore = score;
    }

    // Sort by score descending
    std::sort(all.begin(), all.end(), [](const MemoryItem& a, const MemoryItem& b) {
        return a.relevanceScore > b.relevanceScore;
    });

    QList<MemoryItem> filtered;
    for (const auto& item : all) {
        if (item.relevanceScore > 0.0 || item.isPinned) {
            filtered.append(item);
            if (filtered.size() >= maxResults) break;
        }
    }
    return filtered;
}

bool DatabaseManager::logActivity(const ActivityEvent& event) {
    QMutexLocker locker(&m_mutex);
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO activities (id, created_at, level, title, detail, category) "
              "VALUES (:id, :now, :level, :title, :detail, :cat)");
    q.bindValue(":id", event.id.isEmpty() ? QUuid::createUuid().toString(QUuid::WithoutBraces) : event.id);
    q.bindValue(":now", event.timestamp.isValid() ? event.timestamp : QDateTime::currentDateTime());
    q.bindValue(":level", event.level);
    q.bindValue(":title", event.title);
    q.bindValue(":detail", event.detail);
    q.bindValue(":cat", event.category);
    return q.exec();
}

QList<ActivityEvent> DatabaseManager::getRecentActivities(int limit) {
    QMutexLocker locker(&m_mutex);
    QList<ActivityEvent> list;
    QSqlQuery q(m_db);
    q.prepare("SELECT id, created_at, level, title, detail, category FROM activities ORDER BY created_at DESC LIMIT :lim");
    q.bindValue(":lim", limit);
    q.exec();
    while (q.next()) {
        ActivityEvent act;
        act.id = q.value(0).toString();
        act.timestamp = q.value(1).toDateTime();
        act.level = q.value(2).toString();
        act.title = q.value(3).toString();
        act.detail = q.value(4).toString();
        act.category = q.value(5).toString();
        list.append(act);
    }
    return list;
}

bool DatabaseManager::saveSetting(const QString& key, const QString& value) {
    QMutexLocker locker(&m_mutex);
    QSqlQuery q(m_db);
    q.prepare("INSERT OR REPLACE INTO settings (key, value) VALUES (:key, :value)");
    q.bindValue(":key", key);
    q.bindValue(":value", value);
    return q.exec();
}

QString DatabaseManager::getSetting(const QString& key, const QString& defaultValue) {
    QMutexLocker locker(&m_mutex);
    QSqlQuery q(m_db);
    q.prepare("SELECT value FROM settings WHERE key = :key");
    q.bindValue(":key", key);
    if (q.exec() && q.next()) {
        return q.value(0).toString();
    }
    return defaultValue;
}

} // namespace Jarvis
