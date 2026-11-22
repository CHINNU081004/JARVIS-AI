#pragma once
#include "core/Types.h"
#include <QString>
#include <QJsonObject>
#include <functional>

namespace Jarvis {

class ITool {
public:
    virtual ~ITool() = default;

    virtual QString name() const = 0;
    virtual QString description() const = 0;
    virtual QJsonObject parameterSchema() const = 0;
    virtual ToolPermission permission() const = 0;

    virtual bool validateArguments(const QJsonObject& args, QString* errorOut = nullptr) const = 0;
    virtual ToolExecutionResult execute(const QJsonObject& args) = 0;
    virtual void cancel() {}
};

} // namespace Jarvis
