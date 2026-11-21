#include "ModelListModel.h"

namespace Jarvis {

ModelListModel::ModelListModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int ModelListModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return m_models.size();
}

QVariant ModelListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_models.size()) {
        return QVariant();
    }

    const auto& item = m_models.at(index.row());
    switch (role) {
        case IdRole: return item.id;
        case NameRole: return item.name;
        case DisplayNameRole: {
            QString display = item.name.isEmpty() ? item.id : item.name;
            if (!item.quantization.isEmpty()) {
                display += QString(" (%1)").arg(item.quantization);
            }
            return display;
        }
        case ProviderIdRole: return item.providerId;
        case ParameterSizeRole: return item.parameterSize;
        case QuantizationRole: return item.quantization;
        case FormatRole: return item.format;
        case HasVisionRole: return (item.capabilities.vision == CapabilityState::Supported);
        case HasStreamingRole: return (item.capabilities.streaming == CapabilityState::Supported);
        case HasToolsRole: return (item.capabilities.toolCalling == CapabilityState::Supported);
        default: return QVariant();
    }
}

QHash<int, QByteArray> ModelListModel::roleNames() const {
    return {
        {IdRole, "id"},
        {NameRole, "name"},
        {DisplayNameRole, "displayName"},
        {ProviderIdRole, "providerId"},
        {ParameterSizeRole, "parameterSize"},
        {QuantizationRole, "quantization"},
        {FormatRole, "format"},
        {HasVisionRole, "hasVision"},
        {HasStreamingRole, "hasStreaming"},
        {HasToolsRole, "hasTools"}
    };
}

void ModelListModel::setModels(const QList<ModelInfo>& models) {
    beginResetModel();
    m_models = models;
    endResetModel();
}

int ModelListModel::indexOf(const QString& modelId) const {
    for (int i = 0; i < m_models.size(); ++i) {
        if (m_models[i].id == modelId) return i;
    }
    return -1;
}

QString ModelListModel::idAt(int index) const {
    if (index >= 0 && index < m_models.size()) {
        return m_models[index].id;
    }
    return QString();
}

QString ModelListModel::nameAt(int index) const {
    if (index >= 0 && index < m_models.size()) {
        return m_models[index].name;
    }
    return QString();
}

} // namespace Jarvis
