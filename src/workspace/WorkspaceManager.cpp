#include "WorkspaceManager.h"
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QTextStream>
#include <QVariantMap>

namespace Jarvis {

WorkspaceManager::WorkspaceManager(QObject* parent)
    : QObject(parent)
{
}

void WorkspaceManager::setCurrentPath(const QString& path) {
    QFileInfo info(path);
    if (info.exists() && info.isDir()) {
        m_currentPath = info.canonicalFilePath();
        emit currentPathChanged();
        emit isValidChanged();
    } else {
        emit accessDenied(path, "Target directory does not exist or is not a folder.");
    }
}

bool WorkspaceManager::isPathSafe(const QString& relativePath, QString* canonicalOut) const {
    if (m_currentPath.isEmpty()) return false;

    QDir rootDir(m_currentPath);
    QString cleanSub = QDir::cleanPath(relativePath);

    // Prevent direct absolute jump or parent directory traversal escape
    if (cleanSub.startsWith('/') || cleanSub.startsWith("../") || cleanSub == "..") {
        return false;
    }

    QString fullPath = rootDir.filePath(cleanSub);
    QFileInfo targetInfo(fullPath);

    // If file doesn't exist yet, verify parent directory is inside root
    if (!targetInfo.exists()) {
        QFileInfo parentInfo(targetInfo.dir().canonicalPath());
        if (!parentInfo.canonicalFilePath().startsWith(m_currentPath)) {
            return false;
        }
        if (canonicalOut) *canonicalOut = fullPath;
        return true;
    }

    // Canonical link resolution to prevent symlink traversal outside sandbox
    QString canonicalTarget = targetInfo.canonicalFilePath();
    if (!canonicalTarget.startsWith(m_currentPath)) {
        return false;
    }

    if (canonicalOut) *canonicalOut = canonicalTarget;
    return true;
}

QList<QVariantMap> WorkspaceManager::listFiles(const QString& subDir) {
    QList<QVariantMap> results;
    if (!isValid()) return results;

    QString safePath;
    if (!subDir.isEmpty() && !isPathSafe(subDir, &safePath)) {
        emit accessDenied(subDir, "Directory path traverses outside the workspace sandbox.");
        return results;
    } else if (subDir.isEmpty()) {
        safePath = m_currentPath;
    }

    QDir dir(safePath);
    QFileInfoList entries = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot, QDir::DirsFirst | QDir::Name);

    for (const auto& item : entries) {
        // Enforce symlink safety
        if (item.isSymLink() && !item.canonicalFilePath().startsWith(m_currentPath)) {
            continue; // Skip dangerous symlinks
        }

        QVariantMap map;
        map["name"] = item.fileName();
        map["relativePath"] = QDir(m_currentPath).relativeFilePath(item.absoluteFilePath());
        map["isDirectory"] = item.isDir();
        map["size"] = item.size();
        results.append(map);
    }
    return results;
}

QString WorkspaceManager::readFileContent(const QString& relativePath, qint64 maxBytes) {
    QString safePath;
    if (!isPathSafe(relativePath, &safePath)) {
        emit accessDenied(relativePath, "Attempted to access file outside permitted workspace boundary.");
        return "[Security Error: Access Denied]";
    }

    QFile file(safePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return "[Error: Unable to open file]";
    }

    QByteArray data = file.read(maxBytes);
    file.close();
    return QString::fromUtf8(data);
}

QList<QVariantMap> WorkspaceManager::searchText(const QString& query, int maxResults) {
    QList<QVariantMap> results;
    if (!isValid() || query.trimmed().isEmpty()) return results;

    QDirIterator it(m_currentPath, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext() && results.size() < maxResults) {
        QString filePath = it.next();
        QFileInfo fi(filePath);

        // Skip hidden git folders, build dirs, binary files, symlink escapes
        if (filePath.contains("/.git/") || filePath.contains("/build/") || !fi.canonicalFilePath().startsWith(m_currentPath)) {
            continue;
        }

        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream stream(&file);
            int lineNum = 1;
            while (!stream.atEnd()) {
                QString line = stream.readLine();
                if (line.contains(query, Qt::CaseInsensitive)) {
                    QVariantMap match;
                    match["file"] = QDir(m_currentPath).relativeFilePath(filePath);
                    match["line"] = lineNum;
                    match["text"] = line.trimmed();
                    results.append(match);
                    if (results.size() >= maxResults) break;
                }
                lineNum++;
            }
            file.close();
        }
    }
    return results;
}

} // namespace Jarvis
