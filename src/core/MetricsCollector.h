#pragma once
#include <QObject>
#include <QElapsedTimer>
#include <QTimer>

namespace Jarvis {

class MetricsCollector : public QObject {
    Q_OBJECT
    Q_PROPERTY(qint64 lastLatencyMs READ lastLatencyMs NOTIFY metricsUpdated)
    Q_PROPERTY(qint64 promptTokens READ promptTokens NOTIFY metricsUpdated)
    Q_PROPERTY(qint64 completionTokens READ completionTokens NOTIFY metricsUpdated)
    Q_PROPERTY(double tokensPerSecond READ tokensPerSecond NOTIFY metricsUpdated)
    Q_PROPERTY(qint64 durationMs READ durationMs NOTIFY metricsUpdated)
    Q_PROPERTY(bool isExact READ isExact NOTIFY metricsUpdated)
    Q_PROPERTY(int cpuUsagePercent READ cpuUsagePercent NOTIFY systemMetricsUpdated)
    Q_PROPERTY(double ramUsageGb READ ramUsageGb NOTIFY systemMetricsUpdated)
    Q_PROPERTY(double networkKbps READ networkKbps NOTIFY systemMetricsUpdated)

public:
    explicit MetricsCollector(QObject* parent = nullptr);

    qint64 lastLatencyMs() const { return m_lastLatencyMs; }
    qint64 promptTokens() const { return m_promptTokens; }
    qint64 completionTokens() const { return m_completionTokens; }
    double tokensPerSecond() const { return m_tokensPerSecond; }
    qint64 durationMs() const { return m_durationMs; }
    bool isExact() const { return m_isExact; }

    int cpuUsagePercent() const { return m_cpuUsagePercent; }
    double ramUsageGb() const { return m_ramUsageGb; }
    double networkKbps() const { return m_networkKbps; }

    void recordGenerationMetrics(qint64 pTokens, qint64 cTokens, qint64 duration, double tps, bool exact);
    void recordLatency(qint64 latencyMs);
    void resetGeneration();

signals:
    void metricsUpdated();
    void systemMetricsUpdated();

private slots:
    void sampleSystemMetrics();

private:
    qint64 m_lastLatencyMs = 0;
    qint64 m_promptTokens = 0;
    qint64 m_completionTokens = 0;
    double m_tokensPerSecond = 0.0;
    qint64 m_durationMs = 0;
    bool m_isExact = false;

    int m_cpuUsagePercent = 0;
    double m_ramUsageGb = 0.0;
    double m_networkKbps = 0.0;

    QTimer* m_sysTimer;
    qint64 m_prevCpuIdle = 0;
    qint64 m_prevCpuTotal = 0;
    qint64 m_prevNetBytes = 0;
    qint64 m_prevNetTimeMs = 0;
};

} // namespace Jarvis
