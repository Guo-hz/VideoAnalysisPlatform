#include "AlarmManager.h"

#include <algorithm>
#include <utility>

#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QMetaObject>
#include <QRunnable>

#include "core/storage/AlarmEventRepository.h"

namespace {
constexpr const char *kMotionEventType = "MotionDetected";
constexpr const char *kIntrusionEventType = "IntrusionDetected";

QString normalizedObjectEventType(const QString &eventType)
{
    const QString trimmed = eventType.trimmed();
    if (trimmed == QStringLiteral("PersonDetected") || trimmed == QStringLiteral("VehicleDetected")) {
        return trimmed;
    }
    return QStringLiteral("ObjectDetected");
}

class SnapshotRunnable final : public QRunnable
{
public:
    explicit SnapshotRunnable(std::function<void()> work)
        : m_work(std::move(work))
    {
        setAutoDelete(true);
    }

    void run() override
    {
        m_work();
    }

private:
    std::function<void()> m_work;
};
}

AlarmManager::AlarmManager(AlarmEventRepository *repository, QString snapshotBaseDirectory, QObject *parent)
    : QObject(parent)
    , m_repository(repository)
    , m_snapshotManager(std::move(snapshotBaseDirectory))
{
    m_snapshotPool.setMaxThreadCount(2);
}

AlarmManager::~AlarmManager()
{
    m_shuttingDown.store(true);
    waitForSnapshotWorkers();
}

int AlarmManager::cooldownSeconds() const
{
    return m_cooldownSeconds;
}

void AlarmManager::setCooldownSeconds(int seconds)
{
    const int normalizedSeconds = std::max(1, seconds);
    if (m_cooldownSeconds == normalizedSeconds) {
        return;
    }

    m_cooldownSeconds = normalizedSeconds;
    emit cooldownSecondsChanged();
}

void AlarmManager::reportMotionDetected(const QString &sourceKey,
                                        int sourceId,
                                        const QString &sourceName,
                                        const QImage &frame,
                                        int boxCount,
                                        qint64 timestamp)
{
    if (boxCount <= 0 || m_shuttingDown.load()) {
        return;
    }

    const QString normalizedSourceKey = sourceKey.trimmed().isEmpty()
                                            ? QStringLiteral("source_%1").arg(sourceId)
                                            : sourceKey.trimmed();
    const qint64 eventTimestamp = timestamp > 0 ? timestamp : QDateTime::currentMSecsSinceEpoch();
    if (isCoolingDown(normalizedSourceKey, eventTimestamp, m_cooldownSeconds)) {
        return;
    }

    if (!tryReserveSnapshotTask()) {
        emitFailure(QStringLiteral("告警处理队列已满，已丢弃运动告警。"));
        return;
    }
    reserveCooldown(normalizedSourceKey, eventTimestamp);
    const QString snapshotSourceId = sourceId > 0 ? QString::number(sourceId) : normalizedSourceKey;
    enqueueSnapshot(frame,
                    snapshotSourceId,
                    QString::fromLatin1(kMotionEventType),
                    QStringLiteral("jpg"),
                    [this, normalizedSourceKey, sourceId, sourceName, boxCount, eventTimestamp](const SnapshotResult &result) {
                        if (m_shuttingDown.load()) {
                            return;
                        }
                        QMetaObject::invokeMethod(this,
                                                  [this, normalizedSourceKey, sourceId, sourceName, boxCount, eventTimestamp, result]() {
                                                      createMotionAlarm(normalizedSourceKey,
                                                                        sourceId,
                                                                        sourceName,
                                                                        boxCount,
                                                                        eventTimestamp,
                                                                        normalizedSourceKey,
                                                                        result);
                                                  },
                                                  Qt::QueuedConnection);
                    });
}

void AlarmManager::reportIntrusionDetected(const QString &sourceKey,
                                           int sourceId,
                                           const QString &sourceName,
                                           int ruleId,
                                           const QString &ruleName,
                                           int cooldownSeconds,
                                           const QImage &frame,
                                           int boxCount,
                                           qint64 timestamp)
{
    if (boxCount <= 0 || m_shuttingDown.load()) {
        return;
    }

    const QString normalizedSourceKey = sourceKey.trimmed().isEmpty()
                                            ? QStringLiteral("source_%1").arg(sourceId)
                                            : sourceKey.trimmed();
    const QString normalizedRuleName = ruleName.trimmed().isEmpty()
                                           ? QStringLiteral("ROI %1").arg(ruleId)
                                           : ruleName.trimmed();
    const QString ruleKey = ruleId > 0 ? QString::number(ruleId) : normalizedRuleName;
    const QString cooldownKey = QStringLiteral("%1:roi:%2").arg(normalizedSourceKey, ruleKey);
    const qint64 eventTimestamp = timestamp > 0 ? timestamp : QDateTime::currentMSecsSinceEpoch();
    const int normalizedCooldownSeconds = std::max(1, cooldownSeconds);
    if (isCoolingDown(cooldownKey, eventTimestamp, normalizedCooldownSeconds)) {
        return;
    }

    if (!tryReserveSnapshotTask()) {
        emitFailure(QStringLiteral("告警处理队列已满，已丢弃入侵告警。"));
        return;
    }
    reserveCooldown(cooldownKey, eventTimestamp);
    const QString snapshotSourceId = sourceId > 0 ? QString::number(sourceId) : normalizedSourceKey;
    enqueueSnapshot(frame,
                    snapshotSourceId,
                    QString::fromLatin1(kIntrusionEventType),
                    QStringLiteral("jpg"),
                    [this, cooldownKey, normalizedSourceKey, sourceId, sourceName, normalizedRuleName, boxCount, eventTimestamp](const SnapshotResult &result) {
                        if (m_shuttingDown.load()) {
                            return;
                        }
                        QMetaObject::invokeMethod(this,
                                                  [this, cooldownKey, normalizedSourceKey, sourceId, sourceName, normalizedRuleName, boxCount, eventTimestamp, result]() {
                                                      createIntrusionAlarm(normalizedSourceKey,
                                                                           sourceId,
                                                                           sourceName,
                                                                           normalizedRuleName,
                                                                           boxCount,
                                                                           eventTimestamp,
                                                                           cooldownKey,
                                                                           result);
                                                  },
                                                  Qt::QueuedConnection);
                    });
}

void AlarmManager::reportObjectDetected(const QString &sourceKey,
                                        int sourceId,
                                        const QString &sourceName,
                                        const QString &eventType,
                                        const QString &className,
                                        double confidence,
                                        const QImage &frame,
                                        int boxCount,
                                        qint64 timestamp)
{
    if (boxCount <= 0 || m_shuttingDown.load()) {
        return;
    }

    const QString normalizedSourceKey = sourceKey.trimmed().isEmpty()
                                            ? QStringLiteral("source_%1").arg(sourceId)
                                            : sourceKey.trimmed();
    const QString normalizedEventType = normalizedObjectEventType(eventType);
    const qint64 eventTimestamp = timestamp > 0 ? timestamp : QDateTime::currentMSecsSinceEpoch();
    const QString cooldownKey = QStringLiteral("%1:ai:%2").arg(normalizedSourceKey, normalizedEventType);
    if (isCoolingDown(cooldownKey, eventTimestamp, m_cooldownSeconds)) {
        return;
    }

    if (!tryReserveSnapshotTask()) {
        emitFailure(QStringLiteral("告警处理队列已满，已丢弃 AI 告警。"));
        return;
    }
    reserveCooldown(cooldownKey, eventTimestamp);
    const QString snapshotSourceId = sourceId > 0 ? QString::number(sourceId) : normalizedSourceKey;
    const QString normalizedClassName = className.trimmed().isEmpty()
                                            ? QStringLiteral("object")
                                            : className.trimmed();
    enqueueSnapshot(frame,
                    snapshotSourceId,
                    normalizedEventType,
                    QStringLiteral("jpg"),
                    [this, cooldownKey, normalizedSourceKey, sourceId, sourceName, normalizedEventType, normalizedClassName, confidence, boxCount, eventTimestamp](const SnapshotResult &result) {
                        if (m_shuttingDown.load()) {
                            return;
                        }
                        QMetaObject::invokeMethod(this,
                                                  [this, cooldownKey, normalizedSourceKey, sourceId, sourceName, normalizedEventType, normalizedClassName, confidence, boxCount, eventTimestamp, result]() {
                                                      createObjectAlarm(normalizedSourceKey,
                                                                        sourceId,
                                                                        sourceName,
                                                                        normalizedEventType,
                                                                        normalizedClassName,
                                                                        confidence,
                                                                        boxCount,
                                                                        eventTimestamp,
                                                                        cooldownKey,
                                                                        result);
                                                  },
                                                  Qt::QueuedConnection);
                    });
}

bool AlarmManager::isCoolingDown(const QString &cooldownKey, qint64 timestamp, int cooldownSeconds) const
{
    if (cooldownSeconds < 1) {
        return false;
    }

    qint64 lastTimestamp = 0;
    const auto committed = m_lastAlarmTimeBySource.constFind(cooldownKey);
    if (committed != m_lastAlarmTimeBySource.constEnd()) {
        lastTimestamp = committed.value();
    }
    const auto pending = m_pendingAlarmTimes.constFind(cooldownKey);
    if (pending != m_pendingAlarmTimes.constEnd()) {
        lastTimestamp = std::max(lastTimestamp, pending.value());
    }
    if (lastTimestamp <= 0) {
        return false;
    }

    return timestamp - lastTimestamp < static_cast<qint64>(cooldownSeconds) * 1000;
}

void AlarmManager::reserveCooldown(const QString &cooldownKey, qint64 timestamp)
{
    m_pendingAlarmTimes.insert(cooldownKey, timestamp);
}

void AlarmManager::releaseCooldown(const QString &cooldownKey)
{
    m_pendingAlarmTimes.remove(cooldownKey);
}

void AlarmManager::commitCooldown(const QString &cooldownKey, qint64 timestamp)
{
    m_pendingAlarmTimes.remove(cooldownKey);
    m_lastAlarmTimeBySource.insert(cooldownKey, timestamp);
}

bool AlarmManager::tryReserveSnapshotTask()
{
    const int current = m_snapshotTaskCount.fetch_add(1);
    if (current >= 16) {
        m_snapshotTaskCount.fetch_sub(1);
        return false;
    }
    return true;
}

void AlarmManager::enqueueSnapshot(const QImage &frame,
                                   const QString &sourceId,
                                   const QString &eventType,
                                   const QString &format,
                                   std::function<void(const SnapshotResult &)> callback)
{
    const SnapshotManager snapshotManager = m_snapshotManager;
    auto *task = new SnapshotRunnable([this,
                                       snapshotManager,
                                       frame,
                                       sourceId,
                                       eventType,
                                       format,
                                       callback = std::move(callback)]() mutable {
        const SnapshotResult result = snapshotManager.saveSnapshot(frame, sourceId, eventType, format);
        callback(result);
        m_snapshotTaskCount.fetch_sub(1);
    });
    m_snapshotPool.start(task);
}

void AlarmManager::createMotionAlarm(const QString &sourceKey,
                                     int sourceId,
                                     const QString &sourceName,
                                     int boxCount,
                                     qint64 timestamp,
                                     const QString &cooldownKey,
                                     const SnapshotResult &snapshotResult)
{
    if (!m_repository) {
        releaseCooldown(cooldownKey);
        emitFailure(QStringLiteral("生成运动检测告警失败：告警事件仓库未初始化。"));
        return;
    }

    if (!snapshotResult.success) {
        emitFailure(QStringLiteral("运动检测告警截图失败：%1").arg(snapshotResult.errorMessage));
    }

    AlarmEvent event;
    event.sourceId = sourceId;
    event.videoSourceId = sourceId > 0 ? sourceId : 0;
    event.legacySourceKey = sourceKey;
    event.sourceName = sourceName.trimmed().isEmpty() ? sourceKey : sourceName.trimmed();
    event.eventType = QString::fromLatin1(kMotionEventType);
    event.level = QStringLiteral("Warning");
    event.message = QStringLiteral("检测到运动目标，区域数量：%1").arg(boxCount);
    event.snapshotPath = snapshotResult.success ? snapshotResult.filePath : QString();
    event.videoClipPath = QString();
    event.confirmed = false;
    event.timestamp = QDateTime::fromMSecsSinceEpoch(timestamp, Qt::UTC).toString(Qt::ISODate);

    if (!m_repository->add(&event)) {
        releaseCooldown(cooldownKey);
        if (snapshotResult.success) {
            QFile::remove(snapshotResult.filePath);
        }
        emitFailure(m_repository->lastError());
        return;
    }

    commitCooldown(cooldownKey, timestamp);
    emit alarmCreated(event);
}

void AlarmManager::createIntrusionAlarm(const QString &sourceKey,
                                        int sourceId,
                                        const QString &sourceName,
                                        const QString &ruleName,
                                        int boxCount,
                                        qint64 timestamp,
                                        const QString &cooldownKey,
                                        const SnapshotResult &snapshotResult)
{
    if (!m_repository) {
        releaseCooldown(cooldownKey);
        emitFailure(QStringLiteral("生成入侵告警失败：告警事件仓库未初始化。"));
        return;
    }

    if (!snapshotResult.success) {
        emitFailure(QStringLiteral("入侵告警截图失败：%1").arg(snapshotResult.errorMessage));
    }

    AlarmEvent event;
    event.sourceId = sourceId;
    event.videoSourceId = sourceId > 0 ? sourceId : 0;
    event.legacySourceKey = sourceKey;
    event.sourceName = sourceName.trimmed().isEmpty() ? sourceKey : sourceName.trimmed();
    event.eventType = QString::fromLatin1(kIntrusionEventType);
    event.level = QStringLiteral("Warning");
    event.message = QStringLiteral("目标进入 ROI 规则“%1”，检测框数量：%2").arg(ruleName, QString::number(boxCount));
    event.snapshotPath = snapshotResult.success ? snapshotResult.filePath : QString();
    event.videoClipPath = QString();
    event.confirmed = false;
    event.timestamp = QDateTime::fromMSecsSinceEpoch(timestamp, Qt::UTC).toString(Qt::ISODate);

    if (!m_repository->add(&event)) {
        releaseCooldown(cooldownKey);
        if (snapshotResult.success) {
            QFile::remove(snapshotResult.filePath);
        }
        emitFailure(m_repository->lastError());
        return;
    }

    commitCooldown(cooldownKey, timestamp);
    emit alarmCreated(event);
}

void AlarmManager::createObjectAlarm(const QString &sourceKey,
                                     int sourceId,
                                     const QString &sourceName,
                                     const QString &eventType,
                                     const QString &className,
                                     double confidence,
                                     int boxCount,
                                     qint64 timestamp,
                                     const QString &cooldownKey,
                                     const SnapshotResult &snapshotResult)
{
    if (!m_repository) {
        releaseCooldown(cooldownKey);
        emitFailure(QStringLiteral("生成 AI 检测告警失败：告警事件仓库未初始化。"));
        return;
    }

    if (!snapshotResult.success) {
        emitFailure(QStringLiteral("AI 检测告警截图失败：%1").arg(snapshotResult.errorMessage));
    }

    AlarmEvent event;
    event.sourceId = sourceId;
    event.videoSourceId = sourceId > 0 ? sourceId : 0;
    event.legacySourceKey = sourceKey;
    event.sourceName = sourceName.trimmed().isEmpty() ? sourceKey : sourceName.trimmed();
    event.eventType = eventType;
    event.level = QStringLiteral("Warning");
    event.message = QStringLiteral("AI 检测到 %1，数量：%2，最高置信度：%3")
                        .arg(className, QString::number(boxCount), QString::number(confidence, 'f', 2));
    event.snapshotPath = snapshotResult.success ? snapshotResult.filePath : QString();
    event.videoClipPath = QString();
    event.confirmed = false;
    event.timestamp = QDateTime::fromMSecsSinceEpoch(timestamp, Qt::UTC).toString(Qt::ISODate);

    if (!m_repository->add(&event)) {
        releaseCooldown(cooldownKey);
        if (snapshotResult.success) {
            QFile::remove(snapshotResult.filePath);
        }
        emitFailure(m_repository->lastError());
        return;
    }

    commitCooldown(cooldownKey, timestamp);
    emit alarmCreated(event);
}

void AlarmManager::emitFailure(const QString &message)
{
    if (message.trimmed().isEmpty()) {
        return;
    }

    qWarning().noquote() << message;
    emit alarmFailed(message);
}

void AlarmManager::waitForSnapshotWorkers()
{
    m_snapshotPool.waitForDone(5000);
    m_snapshotTaskCount.store(0);
}
