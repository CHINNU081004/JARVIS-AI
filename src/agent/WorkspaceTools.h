#pragma once
#include "ITool.h"
#include "workspace/WorkspaceManager.h"
#include <QJsonDocument>
#include <QJsonArray>

namespace Jarvis {

// 1. List Files
class ListWorkspaceFilesTool : public ITool {
public:
    explicit ListWorkspaceFilesTool(WorkspaceManager* wm) : m_wm(wm) {}
    QString name() const override { return "list_workspace_files"; }
    QString description() const override { return "List files and subdirectories inside the authorized workspace directory."; }
    ToolPermission permission() const override { return ToolPermission::ReadOnly; }

    QJsonObject parameterSchema() const override {
        QJsonObject schema;
        schema["type"] = "object";
        QJsonObject props;
        QJsonObject subDir;
        subDir["type"] = "string";
        subDir["description"] = "Relative subdirectory to inspect (empty for workspace root).";
        props["sub_dir"] = subDir;
        schema["properties"] = props;
        return schema;
    }

    bool validateArguments(const QJsonObject& args, QString* errorOut) const override {
        Q_UNUSED(args);
        Q_UNUSED(errorOut);
        return true;
    }

    ToolExecutionResult execute(const QJsonObject& args) override {
        ToolExecutionResult res;
        if (!m_wm || !m_wm->isValid()) {
            res.success = false;
            res.errorMessage = "No valid workspace directory is open.";
            return res;
        }

        QString subDir = args["sub_dir"].toString();
        auto entries = m_wm->listFiles(subDir);
        QJsonArray arr;
        for (const auto& item : entries) {
            arr.append(QJsonObject::fromVariantMap(item));
        }

        res.success = true;
        res.output = QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Indented));
        return res;
    }

private:
    WorkspaceManager* m_wm;
};

// 2. Read File
class ReadWorkspaceFileTool : public ITool {
public:
    explicit ReadWorkspaceFileTool(WorkspaceManager* wm) : m_wm(wm) {}
    QString name() const override { return "read_workspace_file"; }
    QString description() const override { return "Read text content of a file within the workspace."; }
    ToolPermission permission() const override { return ToolPermission::ReadOnly; }

    QJsonObject parameterSchema() const override {
        QJsonObject schema;
        schema["type"] = "object";
        QJsonObject props;
        QJsonObject pathObj;
        pathObj["type"] = "string";
        pathObj["description"] = "Relative path of the file to read.";
        props["path"] = pathObj;
        schema["properties"] = props;
        schema["required"] = QJsonArray{"path"};
        return schema;
    }

    bool validateArguments(const QJsonObject& args, QString* errorOut) const override {
        if (!args.contains("path") || args["path"].toString().trimmed().isEmpty()) {
            if (errorOut) *errorOut = "Missing required argument 'path'.";
            return false;
        }
        return true;
    }

    ToolExecutionResult execute(const QJsonObject& args) override {
        ToolExecutionResult res;
        if (!m_wm || !m_wm->isValid()) {
            res.success = false;
            res.errorMessage = "No valid workspace open.";
            return res;
        }

        QString path = args["path"].toString().trimmed();
        QString content = m_wm->readFileContent(path);
        if (content.startsWith("[Security Error") || content.startsWith("[Error")) {
            res.success = false;
            res.errorMessage = content;
        } else {
            res.success = true;
            res.output = content;
        }
        return res;
    }

private:
    WorkspaceManager* m_wm;
};

// 3. Search Files
class SearchWorkspaceFilesTool : public ITool {
public:
    explicit SearchWorkspaceFilesTool(WorkspaceManager* wm) : m_wm(wm) {}
    QString name() const override { return "search_workspace_files"; }
    QString description() const override { return "Search for text occurrences across files in the workspace."; }
    ToolPermission permission() const override { return ToolPermission::ReadOnly; }

    QJsonObject parameterSchema() const override {
        QJsonObject schema;
        schema["type"] = "object";
        QJsonObject props;
        QJsonObject qObj;
        qObj["type"] = "string";
        qObj["description"] = "Substring query to search for.";
        props["query"] = qObj;
        schema["properties"] = props;
        schema["required"] = QJsonArray{"query"};
        return schema;
    }

    bool validateArguments(const QJsonObject& args, QString* errorOut) const override {
        if (!args.contains("query") || args["query"].toString().trimmed().isEmpty()) {
            if (errorOut) *errorOut = "Missing required argument 'query'.";
            return false;
        }
        return true;
    }

    ToolExecutionResult execute(const QJsonObject& args) override {
        ToolExecutionResult res;
        if (!m_wm || !m_wm->isValid()) {
            res.success = false;
            res.errorMessage = "No valid workspace open.";
            return res;
        }

        QString query = args["query"].toString().trimmed();
        auto matches = m_wm->searchText(query);
        QJsonArray arr;
        for (const auto& m : matches) {
            arr.append(QJsonObject::fromVariantMap(m));
        }

        res.success = true;
        res.output = QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Indented));
        return res;
    }

private:
    WorkspaceManager* m_wm;
};

// 4. Create File (Requires Approval)
class CreateFileTool : public ITool {
public:
    explicit CreateFileTool(WorkspaceManager* wm) : m_wm(wm) {}
    QString name() const override { return "create_file"; }
    QString description() const override { return "Create a new text file at the specified relative workspace path with initial content."; }
    ToolPermission permission() const override { return ToolPermission::RequiresApproval; }

    QJsonObject parameterSchema() const override {
        QJsonObject schema;
        schema["type"] = "object";
        QJsonObject props;
        QJsonObject pObj;
        pObj["type"] = "string";
        pObj["description"] = "Relative path where the file should be created.";
        props["path"] = pObj;
        QJsonObject cObj;
        cObj["type"] = "string";
        cObj["description"] = "Content to write into the new file.";
        props["content"] = cObj;
        schema["properties"] = props;
        schema["required"] = QJsonArray{"path", "content"};
        return schema;
    }

    bool validateArguments(const QJsonObject& args, QString* errorOut) const override {
        if (!args.contains("path") || args["path"].toString().trimmed().isEmpty()) {
            if (errorOut) *errorOut = "Missing required argument 'path'.";
            return false;
        }
        return true;
    }

    ToolExecutionResult execute(const QJsonObject& args) override {
        ToolExecutionResult res;
        if (!m_wm || !m_wm->isValid()) {
            res.success = false;
            res.errorMessage = "No valid workspace open.";
            return res;
        }

        QString path = args["path"].toString().trimmed();
        QString content = args["content"].toString();
        QString err;

        if (m_wm->createFile(path, content, &err)) {
            res.success = true;
            res.output = QString("Successfully created file: %1 (%2 bytes)").arg(path).arg(content.toUtf8().size());
            res.modifiedFiles.append(path);
        } else {
            res.success = false;
            res.errorMessage = err;
        }
        return res;
    }

private:
    WorkspaceManager* m_wm;
};

// 5. Edit File (Requires Approval & Diff Preview)
class EditFileTool : public ITool {
public:
    explicit EditFileTool(WorkspaceManager* wm) : m_wm(wm) {}
    QString name() const override { return "edit_file"; }
    QString description() const override { return "Modify an existing text file in the workspace with new content."; }
    ToolPermission permission() const override { return ToolPermission::RequiresApproval; }

    QJsonObject parameterSchema() const override {
        QJsonObject schema;
        schema["type"] = "object";
        QJsonObject props;
        QJsonObject pObj;
        pObj["type"] = "string";
        pObj["description"] = "Relative path of existing file to modify.";
        props["path"] = pObj;
        QJsonObject cObj;
        cObj["type"] = "string";
        cObj["description"] = "Full updated content for the file.";
        props["new_content"] = cObj;
        schema["properties"] = props;
        schema["required"] = QJsonArray{"path", "new_content"};
        return schema;
    }

    bool validateArguments(const QJsonObject& args, QString* errorOut) const override {
        if (!args.contains("path") || args["path"].toString().trimmed().isEmpty()) {
            if (errorOut) *errorOut = "Missing required argument 'path'.";
            return false;
        }
        if (!args.contains("new_content")) {
            if (errorOut) *errorOut = "Missing required argument 'new_content'.";
            return false;
        }
        return true;
    }

    ToolExecutionResult execute(const QJsonObject& args) override {
        ToolExecutionResult res;
        if (!m_wm || !m_wm->isValid()) {
            res.success = false;
            res.errorMessage = "No valid workspace open.";
            return res;
        }

        QString path = args["path"].toString().trimmed();
        QString newContent = args["new_content"].toString();
        QString diff, backupId, err;

        if (m_wm->editFile(path, newContent, &diff, &backupId, &err)) {
            res.success = true;
            res.output = QString("Successfully updated %1.\nUnified Diff:\n%2").arg(path, diff);
            res.modifiedFiles.append(path);
            if (!backupId.isEmpty()) res.backupIds.append(backupId);
        } else {
            res.success = false;
            res.errorMessage = err;
        }
        return res;
    }

private:
    WorkspaceManager* m_wm;
};

// 6. Delete File (Always requires destructive confirmation)
class DeleteFileTool : public ITool {
public:
    explicit DeleteFileTool(WorkspaceManager* wm) : m_wm(wm) {}
    QString name() const override { return "delete_file"; }
    QString description() const override { return "Permanently delete a file in the workspace (creates backup for recovery)."; }
    ToolPermission permission() const override { return ToolPermission::DestructiveConfirm; }

    QJsonObject parameterSchema() const override {
        QJsonObject schema;
        schema["type"] = "object";
        QJsonObject props;
        QJsonObject pObj;
        pObj["type"] = "string";
        pObj["description"] = "Relative path of file to delete.";
        props["path"] = pObj;
        schema["properties"] = props;
        schema["required"] = QJsonArray{"path"};
        return schema;
    }

    bool validateArguments(const QJsonObject& args, QString* errorOut) const override {
        if (!args.contains("path") || args["path"].toString().trimmed().isEmpty()) {
            if (errorOut) *errorOut = "Missing required argument 'path'.";
            return false;
        }
        return true;
    }

    ToolExecutionResult execute(const QJsonObject& args) override {
        ToolExecutionResult res;
        if (!m_wm || !m_wm->isValid()) {
            res.success = false;
            res.errorMessage = "No valid workspace open.";
            return res;
        }

        QString path = args["path"].toString().trimmed();
        QString backupId, err;

        if (m_wm->deleteFile(path, &backupId, &err)) {
            res.success = true;
            res.output = QString("File %1 deleted (backup id: %2 saved for recovery).").arg(path, backupId);
            res.modifiedFiles.append(path);
            res.backupIds.append(backupId);
        } else {
            res.success = false;
            res.errorMessage = err;
        }
        return res;
    }

private:
    WorkspaceManager* m_wm;
};

} // namespace Jarvis
