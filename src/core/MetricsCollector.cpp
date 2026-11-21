#include "MetricsCollector.h"
#include <QFile>
#include <QTextStream>
#include <QDateTime>

namespace Jarvis {

MetricsCollector::MetricsCollector(QObject* parent)
    : QObject(parent)
    , m_sysTimer(new QTimer(this))
{
    connect(m_sysTimer, &QTimer::timeout, this, &MetricsCollector::sampleSystemMetrics);
    m_sysTimer->start(2000); // Sample every 2 seconds to keep CPU overhead minimal
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

void MetricsCollector::sampleSystemMetrics() {
    // 1. Read real Linux /proc/stat for accurate CPU usage
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

    // 2. Read real Linux /proc/meminfo for memory consumption
    QFile memFile("/proc/meminfo");
    if (memFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&memFile);
        qint64 memTotalKb = 0;
        qint64 memAvailKb = 0;
        while (!in.atEnd()) {
            QString line = in.readLine();
            if (line.startsWith("MemTotal:")) {
                memTotalKb = line.split(' ', Qt::SkipEmptyParts)[1].toLongLong();
            } else if (line.startsWith("MemAvailable:")) {
                memAvailKb = line.split(' ', Qt::SkipEmptyParts)[1].toLongLong();
            }
        }
        if (memTotalKb > 0) {
            qint64 usedKb = memTotalKb - memAvailKb;
            m_ramUsageGb = static_cast<double>(usedKb) / (1024.0 * 1024.0);
        }
        memFile.close();
    }

    // 3. Network traffic throughput observation
    QFile netFile("/proc/net/dev");
    if (netFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&netFile);
        qint64 totalBytes = 0;
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.contains(':') && !line.startsWith("lo:")) {
                QStringList fields = line.split(':', Qt::SkipEmptyParts);
                if (fields.size() > 1) {
                    QStringList numbers = fields[1].trimmed().split(' ', Qt::SkipEmptyParts);
                    if (!numbers.isEmpty()) {
                        totalBytes += numbers[0].toLongLong(); // Received bytes
                    }
                }
            }
        }
        qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
        if (m_prevNetTimeMs > 0 && nowMs > m_prevNetTimeMs) {
            double secs = (nowMs - m_prevNetTimeMs) / 1000.0;
            if (secs > 0 && totalBytes >= m_prevNetBytes) {
                m_networkKbps = ((totalBytes - m_prevNetBytes) / 1024.0) / secs;
            }
        }
        m_prevNetBytes = totalBytes;
        m_prevNetTimeMs = nowMs;
        netFile.close();
    }

    emit systemMetricsUpdated();
}

} // namespace Jarvis
