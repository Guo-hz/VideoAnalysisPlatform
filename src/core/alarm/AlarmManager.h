#pragma once

#include <atomic>

#include <QHash>
#include <QImage>
#include <QObject>
#include <QThreadPool>
#include <QString>
#include <functional>

#include "core/alarm/AlarmEvent.h"
#include "core/video/SnapshotManager.h"

class AlarmEventRepository;

class AlarmManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int cooldownSeconds READ cooldownSeconds WRITE setCooldownSeconds NOTIFY cooldownSecondsChanged)

public:
    explicit AlarmManager(AlarmEventRepository *repository,
                          QString snapshotBaseDirectory = QStringLiteral("data/snapshots"),
                          QObject *parent = nullptr);
    ~AlarmManager() override;

    int cooldownSeconds() const;
    void setCooldownSeconds(int seconds);

public slots:
    void reportMotionDetected(const QString &sourceKey,
                              int sourceId,
                              const QString &sourceName,
                              const QImage &frame,
                              int boxCount,
                              qint64 timestamp);
    void reportIntrusionDetected(const QString &sourceKey,
                                 int sourceId,
                                 const QString &sourceName,
                                 int ruleId,
                                 const QString &ruleName,
                                 int cooldownSeconds,
                                 const QImage &frame,
                                 int boxCount,
                                 qint64 timestamp);
    void reportObjectDetected(const QString &sourceKey,
                              int sourceId,
                              const QString &sourceName,
                              const QString &eventType,
                              const QString &className,
                              double confidence,
                              const QImage &frame,
                              int boxCount,
                              qint64 timestamp);

signals:
    void alarmCreated(const AlarmEvent &event);
    void alarmFailed(const QString &message);
    void cooldownSecondsChanged();

private:
    bool isCoolingDown(const QString &cooldownKey, qint64 timestamp, int cooldownSeconds) const;
    void createMotionAlarm(const QString &sourceKey,
                           int sourceId,
                           const QString &sourceName,
                           int boxCount,
                           qint64 timestamp,
                           const QString &cooldownKey,
                           const SnapshotResult &snapshotResult);
    void createIntrusionAlarm(const QString &sourceKey,
                              int sourceId,
                              const QString &sourceName,
                              const QString &ruleName,
                              int boxCount,
                              qint64 timestamp,
                              const QString &cooldownKey,
                              const SnapshotResult &snapshotResult);
    void createObjectAlarm(const QString &sourceKey,
                           int sourceId,
                           const QString &sourceName,
                           const QString &eventType,
                           const QString &className,
                           double confidence,
                           int boxCount,
                           qint64 timestamp,
                           const QString &cooldownKey,
                           const SnapshotResult &snapshotResult);
    void emitFailure(const QString &message);
    void waitForSnapshotWorkers();
    bool tryReserveSnapshotTask();
    void enqueueSnapshot(const QImage &frame,
                         const QString &sourceId,
                         const QString &eventType,
                         const QString &format,
                         std::function<void(const SnapshotResult &)> callback);
    void reserveCooldown(const QString &cooldownKey, qint64 timestamp);
    void releaseCooldown(const QString &cooldownKey);
    void commitCooldown(const QString &cooldownKey, qint64 timestamp);

    AlarmEventRepository *m_repository = nullptr;
    SnapshotManager m_snapshotManager;
    QHash<QString, qint64> m_lastAlarmTimeBySource;
    QHash<QString, qint64> m_pendingAlarmTimes;
    QThreadPool m_snapshotPool;
    std::atomic_int m_snapshotTaskCount = 0;
    std::atomic_bool m_shuttingDown = false;
    int m_cooldownSeconds = 10;
};
