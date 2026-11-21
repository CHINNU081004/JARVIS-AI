#pragma once
#include <QObject>
#include <QElapsedTimer>
#include <QTimer>
#include <QString>

namespace Jarvis {

struct SystemMemoryStats {
    qint64 totalBytes = 0;
    qint64 availableBytes = 0;
    qint64 usedBytes = 0;
    double usedPercent = 0.0;
    bool isValid = false;
};

struct NetworkStats {
    qint64 rxBytes = 0;
    qint64 txBytes = 0;
    double rxKbps = 0.0;
    double txKbps = 0.0;
    double totalKbps = 0.0;
    QString activeInterface;
    bool isConnected = false;
    bool isValid = false;
};

class MetricsCollector : public QObject {
    Q_OBJECT
    // Telemetry for Model Generation
    Q_PROPERTY(qint64 lastLatencyMs READ lastLatencyMs NOTIFY metricsUpdated)
    Q_PROPERTY(qint64 promptTokens READ promptTokens NOTIFY metricsUpdated)
    Q_PROPERTY(qint64 completionTokens READ completionTokens NOTIFY metricsUpdated)
    Q_PROPERTY(double tokensPerSecond READ tokensPerSecond NOTIFY metricsUpdated)
    Q_PROPERTY(qint64 durationMs READ durationMs NOTIFY metricsUpdated)
    Q_PROPERTY(bool isExact READ isExact NOTIFY metricsUpdated)

    // CPU Metrics
    Q_PROPERTY(int cpuUsagePercent READ cpuUsagePercent NOTIFY systemMetricsUpdated)

    // RAM Metrics
    Q_PROPERTY(double ramUsageGb READ ramUsageGb NOTIFY systemMetricsUpdated)
    Q_PROPERTY(double ramTotalGb READ ramTotalGb NOTIFY systemMetricsUpdated)
    Q_PROPERTY(double ramAvailableGb READ ramAvailableGb NOTIFY systemMetricsUpdated)
    Q_PROPERTY(int ramUsagePercent READ ramUsagePercent NOTIFY systemMetricsUpdated)
    Q_PROPERTY(QString ramDisplayStr READ ramDisplayStr NOTIFY systemMetricsUpdated)
    Q_PROPERTY(bool isRamValid READ isRamValid NOTIFY systemMetricsUpdated)

    // Network Metrics
    Q_PROPERTY(double networkRxKbps READ networkRxKbps NOTIFY systemMetricsUpdated)
    Q_PROPERTY(double networkTxKbps READ networkTxKbps NOTIFY systemMetricsUpdated)
    Q_PROPERTY(double networkKbps READ networkKbps NOTIFY systemMetricsUpdated)
    Q_PROPERTY(QString networkDisplayStr READ networkDisplayStr NOTIFY systemMetricsUpdated)
    Q_PROPERTY(QString activeInterface READ activeInterface NOTIFY systemMetricsUpdated)
    Q_PROPERTY(bool isNetworkConnected READ isNetworkConnected NOTIFY systemMetricsUpdated)

    // GPU & Hardware Acceleration Metrics
    Q_PROPERTY(QString gpuName READ gpuName NOTIFY systemMetricsUpdated)
    Q_PROPERTY(bool isGpuDedicated READ isGpuDedicated NOTIFY systemMetricsUpdated)
    Q_PROPERTY(QString gpuTypeStr READ gpuTypeStr NOTIFY systemMetricsUpdated)
    Q_PROPERTY(bool isRemoteGpuConfigured READ isRemoteGpuConfigured WRITE setRemoteGpuConfigured NOTIFY systemMetricsUpdated)

public:
    explicit MetricsCollector(QObject* parent = nullptr);

    // Generation Telemetry
    qint64 lastLatencyMs() const { return m_lastLatencyMs; }
    qint64 promptTokens() const { return m_promptTokens; }
    qint64 completionTokens() const { return m_completionTokens; }
    double tokensPerSecond() const { return m_tokensPerSecond; }
    qint64 durationMs() const { return m_durationMs; }
    bool isExact() const { return m_isExact; }

    // System Metrics
    int cpuUsagePercent() const { return m_cpuUsagePercent; }

    double ramUsageGb() const { return m_memStats.usedBytes / (1024.0 * 1024.0 * 1024.0); }
    double ramTotalGb() const { return m_memStats.totalBytes / (1024.0 * 1024.0 * 1024.0); }
    double ramAvailableGb() const { return m_memStats.availableBytes / (1024.0 * 1024.0 * 1024.0); }
    int ramUsagePercent() const { return static_cast<int>(m_memStats.usedPercent); }
    QString ramDisplayStr() const;
    bool isRamValid() const { return m_memStats.isValid; }

    double networkRxKbps() const { return m_netStats.rxKbps; }
    double networkTxKbps() const { return m_netStats.txKbps; }
    double networkKbps() const { return m_netStats.totalKbps; }
    QString networkDisplayStr() const;
    QString activeInterface() const { return m_netStats.activeInterface; }
    bool isNetworkConnected() const { return m_netStats.isConnected; }

    // GPU properties
    QString gpuName() const { return m_gpuName; }
    bool isGpuDedicated() const { return m_isGpuDedicated; }
    QString gpuTypeStr() const { return m_isGpuDedicated ? "Dedicated GPU" : "Integrated iGPU (Shared VRAM)"; }
    bool isRemoteGpuConfigured() const { return m_isRemoteGpuConfigured; }
    void setRemoteGpuConfigured(bool val) {
        if (m_isRemoteGpuConfigured != val) {
            m_isRemoteGpuConfigured = val;
            emit systemMetricsUpdated();
        }
    }

    void recordGenerationMetrics(qint64 pTokens, qint64 cTokens, qint64 duration, double tps, bool exact);
    void recordLatency(qint64 latencyMs);
    void resetGeneration();

    // Testing / Validation helpers
    static SystemMemoryStats parseMemInfo(const QString& memInfoContent);
    static NetworkStats parseNetworkDev(const QString& netDevContent,
                                       qint64 prevRx, qint64 prevTx,
                                       qint64 elapsedMs);

signals:
    void metricsUpdated();
    void systemMetricsUpdated();

public slots:
    void sampleSystemMetrics();

private:
    qint64 m_lastLatencyMs = 0;
    qint64 m_promptTokens = 0;
    qint64 m_completionTokens = 0;
    double m_tokensPerSecond = 0.0;
    qint64 m_durationMs = 0;
    bool m_isExact = false;

    int m_cpuUsagePercent = 0;
    SystemMemoryStats m_memStats;
    NetworkStats m_netStats;

    QTimer* m_sysTimer;
    QString m_gpuName = "Intel HD Graphics 620";
    bool m_isGpuDedicated = false;
    bool m_isRemoteGpuConfigured = false;
    qint64 m_prevCpuIdle = 0;
    qint64 m_prevCpuTotal = 0;
    qint64 m_prevNetRxBytes = 0;
    qint64 m_prevNetTxBytes = 0;
    qint64 m_prevNetTimeMs = 0;
};

} // namespace Jarvis
