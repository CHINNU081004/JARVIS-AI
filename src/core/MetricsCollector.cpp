#include "MetricsCollector.h"
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QRegularExpression>

namespace Jarvis {

MetricsCollector::MetricsCollector(QObject* parent)
    : QObject(parent)
    , m_sysTimer(new QTimer(this))
{
    // Detect local GPU device
    QFile vendorFile("/sys/class/drm/card1/device/vendor");
    QFile devFile("/sys/class/drm/card1/device/device");
    if (vendorFile.open(QIODevice::ReadOnly | QIODevice::Text) && devFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString vendor = vendorFile.readAll().trimmed();
        QString device = devFile.readAll().trimmed();
        if (vendor == "0x8086") {
            m_gpuName = "Intel HD Graphics 620";
            m_isGpuDedicated = false; // Integrated iGPU
        } else if (vendor == "0x10de") {
            m_gpuName = "NVIDIA Dedicated GPU";
            m_isGpuDedicated = true;
        } else if (vendor == "0x1002") {
            m_gpuName = "AMD Radeon GPU";
            m_isGpuDedicated = true;
        }
    }

    connect(m_sysTimer, &QTimer::timeout, this, &MetricsCollector::sampleSystemMetrics);
    m_sysTimer->start(2000); // 2-second sampling interval for minimal CPU overhead
    sampleSystemMetrics();
}

void MetricsCollector::recordGenerationMetrics(qint64 pTokens, qint64 cTokens, qint64 duration, double tps, bool exact) {
    m_promptTokens = pTokens;
    m_completionTokens = cTokens;
    m_durationMs = duration;
    m_tokensPerSecond = tps;
    m_isExact = exact;
    emit metricsUpdated();
}

void MetricsCollector::recordLatency(qint64 latencyMs) {
    m_lastLatencyMs = latencyMs;
    emit metricsUpdated();
}

void MetricsCollector::resetGeneration() {
    m_promptTokens = 0;
    m_completionTokens = 0;
    m_durationMs = 0;
    m_tokensPerSecond = 0.0;
    m_isExact = false;
    emit metricsUpdated();
}

SystemMemoryStats MetricsCollector::parseMemInfo(const QString& memInfoContent) {
    SystemMemoryStats stats;
    qint64 memTotalKb = 0;
    qint64 memAvailKb = 0;

    QStringList lines = memInfoContent.split('\n', Qt::SkipEmptyParts);
    for (const QString& line : lines) {
        if (line.startsWith("MemTotal:")) {
            QStringList parts = line.split(' ', Qt::SkipEmptyParts);
            if (parts.size() >= 2) {
                memTotalKb = parts[1].toLongLong();
            }
        } else if (line.startsWith("MemAvailable:")) {
            QStringList parts = line.split(' ', Qt::SkipEmptyParts);
            if (parts.size() >= 2) {
                memAvailKb = parts[1].toLongLong();
            }
        }
    }

    if (memTotalKb > 0) {
        stats.totalBytes = memTotalKb * 1024;
        stats.availableBytes = memAvailKb * 1024;
        stats.usedBytes = (memTotalKb >= memAvailKb) ? (memTotalKb - memAvailKb) * 1024 : 0;
        stats.usedPercent = (static_cast<double>(stats.usedBytes) / stats.totalBytes) * 100.0;
        stats.isValid = true;
    }

    return stats;
}

NetworkStats MetricsCollector::parseNetworkDev(const QString& netDevContent,
                                              qint64 prevRx, qint64 prevTx,
                                              qint64 elapsedMs)
{
    NetworkStats stats;
    qint64 currentRx = 0;
    qint64 currentTx = 0;
    QString chosenIf;

    QStringList lines = netDevContent.split('\n', Qt::SkipEmptyParts);
    for (const QString& line : lines) {
        if (!line.contains(':')) continue;

        QStringList parts = line.split(':', Qt::SkipEmptyParts);
        if (parts.size() < 2) continue;

        QString iface = parts[0].trimmed();
        if (iface == "lo") continue; // Exclude loopback

        QStringList counters = parts[1].trimmed().split(' ', Qt::SkipEmptyParts);
        // counters format:
        // 0: rx_bytes, 1: rx_packets, ..., 8: tx_bytes, 9: tx_packets
        if (counters.size() >= 9) {
            qint64 rx = counters[0].toLongLong();
            qint64 tx = counters[8].toLongLong();

            currentRx += rx;
            currentTx += tx;

            if (rx > 0 || tx > 0) {
                if (chosenIf.isEmpty() || iface.startsWith("wl") || iface.startsWith("en") || iface.startsWith("eth")) {
                    chosenIf = iface;
                }
            }
        }
    }

    stats.rxBytes = currentRx;
    stats.txBytes = currentTx;
    stats.activeInterface = chosenIf.isEmpty() ? "None" : chosenIf;
    stats.isConnected = !chosenIf.isEmpty();

    if (elapsedMs > 0 && prevRx >= 0 && prevTx >= 0) {
        double seconds = elapsedMs / 1000.0;
        // Check for counter reset / reboot / overflow
        if (currentRx >= prevRx && currentTx >= prevTx) {
            stats.rxKbps = ((currentRx - prevRx) / 1024.0) / seconds;
            stats.txKbps = ((currentTx - prevTx) / 1024.0) / seconds;
            stats.totalKbps = stats.rxKbps + stats.txKbps;
            stats.isValid = true;
        } else {
            // Counter reset occurred, reinitialize baseline
            stats.rxKbps = 0.0;
            stats.txKbps = 0.0;
            stats.totalKbps = 0.0;
            stats.isValid = true;
        }
    }

    return stats;
}

QString MetricsCollector::ramDisplayStr() const {
    if (!m_memStats.isValid) return "RAM Unavailable";
    return QString("%1 GB (%2%)").arg(QString::number(ramUsageGb(), 'f', 1)).arg(ramUsagePercent());
}

QString MetricsCollector::networkDisplayStr() const {
    if (!m_netStats.isValid) return "Offline";
    if (m_netStats.totalKbps >= 1024.0) {
        return QString("%1 MB/s").arg(QString::number(m_netStats.totalKbps / 1024.0, 'f', 2));
    }
    return QString("%1 KB/s").arg(QString::number(m_netStats.totalKbps, 'f', 1));
}

void MetricsCollector::sampleSystemMetrics() {
    // 1. CPU Usage (/proc/stat)
    QFile statFile("/proc/stat");
    if (statFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&statFile);
        QString line = in.readLine();
        if (line.startsWith("cpu ")) {
            QStringList parts = line.split(' ', Qt::SkipEmptyParts);
            if (parts.size() >= 5) {
                qint64 user = parts[1].toLongLong();
                qint64 nice = parts[2].toLongLong();
                qint64 system = parts[3].toLongLong();
                qint64 idle = parts[4].toLongLong();
                qint64 total = user + nice + system + idle;

                if (m_prevCpuTotal > 0 && total > m_prevCpuTotal) {
                    qint64 totalDiff = total - m_prevCpuTotal;
                    qint64 idleDiff = idle - m_prevCpuIdle;
                    m_cpuUsagePercent = static_cast<int>(100.0 * (totalDiff - idleDiff) / totalDiff);
                    if (m_cpuUsagePercent < 0) m_cpuUsagePercent = 0;
                    if (m_cpuUsagePercent > 100) m_cpuUsagePercent = 100;
                }
                m_prevCpuTotal = total;
                m_prevCpuIdle = idle;
            }
        }
        statFile.close();
    }

    // 2. RAM Usage (/proc/meminfo)
    QFile memFile("/proc/meminfo");
    if (memFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&memFile);
        QString content = in.readAll();
        m_memStats = parseMemInfo(content);
        memFile.close();
    } else {
        m_memStats.isValid = false;
    }

    // 3. Network Throughput (/proc/net/dev)
    QFile netFile("/proc/net/dev");
    if (netFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&netFile);
        QString content = in.readAll();
        qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
        qint64 elapsedMs = (m_prevNetTimeMs > 0) ? (nowMs - m_prevNetTimeMs) : 0;

        m_netStats = parseNetworkDev(content, m_prevNetRxBytes, m_prevNetTxBytes, elapsedMs);

        m_prevNetRxBytes = m_netStats.rxBytes;
        m_prevNetTxBytes = m_netStats.txBytes;
        m_prevNetTimeMs = nowMs;
        netFile.close();
    } else {
        m_netStats.isValid = false;
    }

    emit systemMetricsUpdated();
}

} // namespace Jarvis
