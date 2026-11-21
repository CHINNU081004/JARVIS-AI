#pragma once
#include <QAbstractListModel>
#include <QList>
#include "Types.h"

namespace Jarvis {

class ModelListModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        DisplayNameRole,
        ProviderIdRole,
        ParameterSizeRole,
        QuantizationRole,
        FormatRole,
        HasVisionRole,
        HasStreamingRole,
        HasToolsRole
    };

    explicit ModelListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setModels(const QList<ModelInfo>& models);
    Q_INVOKABLE int indexOf(const QString& modelId) const;
    Q_INVOKABLE QString idAt(int index) const;
    Q_INVOKABLE QString nameAt(int index) const;

private:
    QList<ModelInfo> m_models;
};

} // namespace Jarvis
