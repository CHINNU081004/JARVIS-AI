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

    bool isPathSafe(const QString& relativePath, QString* canonicalOut = nullptr) const;

signals:
    void currentPathChanged();
    void isValidChanged();
    void accessDenied(const QString& attemptedPath, const QString& reason);

private:
    QString m_currentPath;
};

} // namespace Jarvis
