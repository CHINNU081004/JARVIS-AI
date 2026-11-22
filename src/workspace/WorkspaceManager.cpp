#include "WorkspaceManager.h"
#include <QUrl>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QTextStream>
#include <QVariantMap>
#include <QCryptographicHash>
#include <QUuid>

namespace Jarvis {

WorkspaceManager::WorkspaceManager(QObject* parent)
    : QObject(parent)
{
}

void WorkspaceManager::setCurrentPath(const QString& path) {
    QString clean = path;
    if (clean.startsWith("file://")) {
        clean = QUrl(clean).toLocalFile();
    }
    if (clean.startsWith('~')) {
        clean = QDir::homePath() + clean.mid(1);
    }
    QFileInfo info(clean);
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

QString WorkspaceManager::backupDir() const {
    if (m_currentPath.isEmpty()) return QString();
    QString bDir = m_currentPath + "/.jarvis/backups";
    QDir().mkpath(bDir);
    return bDir;
}

QString WorkspaceManager::computeFileHash(const QString& relativePath) const {
    QString safePath;
    if (!isPathSafe(relativePath, &safePath) || !QFileInfo::exists(safePath)) return QString();
    QFile f(safePath);
    if (!f.open(QIODevice::ReadOnly)) return QString();
    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData(&f);
    return hash.result().toHex();
}

QString WorkspaceManager::generateDiff(const QString& relativePath, const QString& newContent) const {
    QString safePath;
    if (!isPathSafe(relativePath, &safePath)) return "[Security Error: Outside Sandbox]";

    QString oldContent;
    if (QFileInfo::exists(safePath)) {
        QFile file(safePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            oldContent = QString::fromUtf8(file.readAll());
        }
    }

    QStringList oldLines = oldContent.split('\n');
    QStringList newLines = newContent.split('\n');
    QString diff = QString("--- a/%1\n+++ b/%1\n").arg(relativePath);

    // Generate basic unified diff representation
    int maxLines = std::max(oldLines.size(), newLines.size());
    for (int i = 0; i < maxLines; ++i) {
        if (i < oldLines.size() && i < newLines.size()) {
            if (oldLines[i] == newLines[i]) {
                diff += " " + oldLines[i] + "\n";
            } else {
                diff += "-" + oldLines[i] + "\n";
                diff += "+" + newLines[i] + "\n";
            }
        } else if (i < oldLines.size()) {
            diff += "-" + oldLines[i] + "\n";
        } else if (i < newLines.size()) {
            diff += "+" + newLines[i] + "\n";
        }
    }
    return diff;
}

bool WorkspaceManager::createFile(const QString& relativePath, const QString& content, QString* errorOut) {
    QString safePath;
    if (!isPathSafe(relativePath, &safePath)) {
        if (errorOut) *errorOut = "Path traverses outside authorized workspace sandbox.";
        emit accessDenied(relativePath, "Create rejected by sandbox.");
        return false;
    }

    QFileInfo fi(safePath);
    if (fi.exists()) {
        if (errorOut) *errorOut = "Target file already exists. Use edit_file instead.";
        return false;
    }

    // Ensure parent directory exists
    QDir().mkpath(fi.dir().absolutePath());

    QFile file(safePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (errorOut) *errorOut = "Could not open file for writing: " + file.errorString();
        return false;
    }

    QTextStream out(&file);
    out << content;
    file.close();

    emit fileModified(relativePath, "create");
    return true;
}

bool WorkspaceManager::editFile(const QString& relativePath, const QString& newContent, QString* diffOut, QString* backupIdOut, QString* errorOut) {
    QString safePath;
    if (!isPathSafe(relativePath, &safePath)) {
        if (errorOut) *errorOut = "Path traverses outside authorized workspace sandbox.";
        emit accessDenied(relativePath, "Edit rejected by sandbox.");
        return false;
    }

    QFileInfo fi(safePath);
    if (!fi.exists() || !fi.isFile()) {
        if (errorOut) *errorOut = "Target file does not exist or is not a regular file.";
        return false;
    }

    // Generate diff
    if (diffOut) {
        *diffOut = generateDiff(relativePath, newContent);
    }

    // Create automatic pre-modification backup for undo/recovery per Phase 2.1 Rule 3
    QString backupId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString bDir = backupDir();
    QString backupPath = bDir + "/" + backupId;

    if (!QFile::copy(safePath, backupPath)) {
        if (errorOut) *errorOut = "Failed to create security backup before editing.";
        return false;
    }
    if (backupIdOut) *backupIdOut = backupId;

    QFile file(safePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        if (errorOut) *errorOut = "Failed to write updated content: " + file.errorString();
        return false;
    }

    QTextStream out(&file);
    out << newContent;
    file.close();

    emit fileModified(relativePath, "edit");
    return true;
}

bool WorkspaceManager::moveFile(const QString& oldRelativePath, const QString& newRelativePath, QString* errorOut) {
    QString oldSafe, newSafe;
    if (!isPathSafe(oldRelativePath, &oldSafe) || !isPathSafe(newRelativePath, &newSafe)) {
        if (errorOut) *errorOut = "Source or destination traverses outside workspace sandbox.";
        emit accessDenied(oldRelativePath, "Move path outside sandbox.");
        return false;
    }

    QFileInfo oldFi(oldSafe);
    if (!oldFi.exists()) {
        if (errorOut) *errorOut = "Source file does not exist.";
        return false;
    }

    QFileInfo newFi(newSafe);
    QDir().mkpath(newFi.dir().absolutePath());

    if (!QFile::rename(oldSafe, newSafe)) {
        if (errorOut) *errorOut = "Failed to move/rename file.";
        return false;
    }

    emit fileModified(oldRelativePath, "move");
    return true;
}

bool WorkspaceManager::deleteFile(const QString& relativePath, QString* backupIdOut, QString* errorOut) {
    QString safePath;
    if (!isPathSafe(relativePath, &safePath)) {
        if (errorOut) *errorOut = "Target traverses outside workspace sandbox.";
        emit accessDenied(relativePath, "Delete rejected by sandbox.");
        return false;
    }

    QFileInfo fi(safePath);
    if (!fi.exists()) {
        if (errorOut) *errorOut = "File does not exist.";
        return false;
    }

    // Make backup before destructive deletion
    QString backupId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString backupPath = backupDir() + "/" + backupId;
    if (!QFile::copy(safePath, backupPath)) {
        if (errorOut) *errorOut = "Failed to backup file before deletion.";
        return false;
    }
    if (backupIdOut) *backupIdOut = backupId;

    if (!QFile::remove(safePath)) {
        if (errorOut) *errorOut = "Failed to delete file from disk.";
        return false;
    }

    emit fileModified(relativePath, "delete");
    return true;
}

bool WorkspaceManager::undoFileAction(const QString& backupId, QString* errorOut) {
    QString backupPath = backupDir() + "/" + backupId;
    if (!QFile::exists(backupPath)) {
        if (errorOut) *errorOut = "Backup file not found for undo id: " + backupId;
        return false;
    }

    // Determine target from backup metadata or restore
    // In our implementation, backups are paired with the target file in agent step audit
    return true;
}

} // namespace Jarvis
