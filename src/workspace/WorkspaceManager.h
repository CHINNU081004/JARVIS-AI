#pragma once
#include <QObject>
#include <QString>
#include <QFileInfo>
#include <QList>

namespace Jarvis {

struct WorkspaceFileItem {
    QString name;
    QString relativePath;
    bool isDirectory = false;
    qint64 size = 0;
};

class WorkspaceManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString currentPath READ currentPath WRITE setCurrentPath NOTIFY currentPathChanged)
    Q_PROPERTY(bool isValid READ isValid NOTIFY isValidChanged)

public:
    explicit WorkspaceManager(QObject* parent = nullptr);

    QString currentPath() const { return m_currentPath; }
    void setCurrentPath(const QString& path);

    bool isValid() const { return !m_currentPath.isEmpty() && QFileInfo::exists(m_currentPath); }

    Q_INVOKABLE QList<QVariantMap> listFiles(const QString& subDir = QString());
    Q_INVOKABLE QString readFileContent(const QString& relativePath, qint64 maxBytes = 100000);
    Q_INVOKABLE QList<QVariantMap> searchText(const QString& query, int maxResults = 50);

    // Phase 2.1 File Operations, Diffs, Backups & Recovery
    bool createFile(const QString& relativePath, const QString& content, QString* errorOut = nullptr);
    bool editFile(const QString& relativePath, const QString& newContent, QString* diffOut = nullptr, QString* backupIdOut = nullptr, QString* errorOut = nullptr);
    bool moveFile(const QString& oldRelativePath, const QString& newRelativePath, QString* errorOut = nullptr);
    bool deleteFile(const QString& relativePath, QString* backupIdOut = nullptr, QString* errorOut = nullptr);
    bool undoFileAction(const QString& backupId, QString* errorOut = nullptr);
    QString generateDiff(const QString& relativePath, const QString& newContent) const;
    QString computeFileHash(const QString& relativePath) const;

    bool isPathSafe(const QString& relativePath, QString* canonicalOut = nullptr) const;

signals:
    void currentPathChanged();
    void isValidChanged();
    void accessDenied(const QString& attemptedPath, const QString& reason);
    void fileModified(const QString& relativePath, const QString& action);

private:
    QString backupDir() const;
    QString m_currentPath;
};

} // namespace Jarvis
