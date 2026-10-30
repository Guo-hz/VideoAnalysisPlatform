#include "AlarmEventRepository.h"

#include <QDateTime>
#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

AlarmEventRepository::AlarmEventRepository(const QSqlDatabase &database)
    : m_database(database)
{
}

QVector<AlarmEvent> AlarmEventRepository::loadAll()
{
    QVector<AlarmEvent> events;
    if (!ensureDatabaseOpen()) {
        return events;
    }

    QSqlQuery query(m_database);
    if (!query.exec(QStringLiteral(R"(
        SELECT id, source_id, video_source_id, legacy_source_key, source_name, event_type, level, message,
               snapshot_path, video_clip_path, confirmed, timestamp
        FROM alarm_events
        ORDER BY timestamp DESC, id DESC
    )"))) {
        setLastError(QStringLiteral("读取告警事件失败"), query.lastError().text());
        return events;
    }

    while (query.next()) {
        AlarmEvent event;
        event.id = query.value(0).toInt();
        event.sourceId = query.value(1).toInt();
        event.videoSourceId = query.value(2).toInt();
        event.legacySourceKey = query.value(3).toString();
        event.sourceName = query.value(4).toString();
        event.eventType = query.value(5).toString();
        event.level = query.value(6).toString();
        event.message = query.value(7).toString();
        event.snapshotPath = query.value(8).toString();
        event.videoClipPath = query.value(9).toString();
        event.confirmed = query.value(10).toInt() != 0;
        event.timestamp = query.value(11).toString();
        events.append(event);
    }

    m_lastError.clear();
    return events;
}

bool AlarmEventRepository::add(AlarmEvent *event)
{
    if (!event) {
        setLastError(QStringLiteral("新增告警事件失败"), QStringLiteral("event 为空。"));
        return false;
    }
    if (!ensureDatabaseOpen()) {
        return false;
    }

    if (event->timestamp.trimmed().isEmpty()) {
        event->timestamp = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    }

    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(R"(
        INSERT INTO alarm_events (
            source_id, video_source_id, legacy_source_key, source_name, event_type, level, message,
            snapshot_path, video_clip_path, confirmed, timestamp
        )
        VALUES (
            :source_id, :video_source_id, :legacy_source_key, :source_name, :event_type, :level, :message,
            :snapshot_path, :video_clip_path, :confirmed, :timestamp
        )
    )"));
    query.bindValue(QStringLiteral(":source_id"), event->sourceId);
    query.bindValue(QStringLiteral(":video_source_id"), event->videoSourceId > 0 ? QVariant(event->videoSourceId) : QVariant());
    query.bindValue(QStringLiteral(":legacy_source_key"), event->legacySourceKey);
    query.bindValue(QStringLiteral(":source_name"), event->sourceName);
    query.bindValue(QStringLiteral(":event_type"), event->eventType);
    query.bindValue(QStringLiteral(":level"), event->level);
    query.bindValue(QStringLiteral(":message"), event->message);
    query.bindValue(QStringLiteral(":snapshot_path"), event->snapshotPath);
    query.bindValue(QStringLiteral(":video_clip_path"), event->videoClipPath);
    query.bindValue(QStringLiteral(":confirmed"), event->confirmed ? 1 : 0);
    query.bindValue(QStringLiteral(":timestamp"), event->timestamp);

    if (!query.exec()) {
        setLastError(QStringLiteral("新增告警事件失败"), query.lastError().text());
        return false;
    }

    event->id = query.lastInsertId().toInt();
    m_lastError.clear();
    return true;
}

bool AlarmEventRepository::confirm(int id)
{
    if (!ensureDatabaseOpen()) {
        return false;
    }

    QSqlQuery query(m_database);
    query.prepare(QStringLiteral("UPDATE alarm_events SET confirmed = 1 WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), id);

    if (!query.exec()) {
        setLastError(QStringLiteral("确认告警事件失败"), query.lastError().text());
        return false;
    }

    m_lastError.clear();
    return true;
}

bool AlarmEventRepository::remove(int id)
{
    if (!ensureDatabaseOpen()) {
        return false;
    }

    QSqlQuery query(m_database);
    query.prepare(QStringLiteral("DELETE FROM alarm_events WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), id);

    if (!query.exec()) {
        setLastError(QStringLiteral("删除告警事件失败"), query.lastError().text());
        return false;
    }

    m_lastError.clear();
    return true;
}

QString AlarmEventRepository::lastError() const
{
    return m_lastError;
}

bool AlarmEventRepository::ensureDatabaseOpen()
{
    if (m_database.isValid() && m_database.isOpen()) {
        return true;
    }

    setLastError(QStringLiteral("数据库操作失败"), QStringLiteral("数据库未打开。"));
    return false;
}

void AlarmEventRepository::setLastError(const QString &context, const QString &detail)
{
    m_lastError = QStringLiteral("%1：%2").arg(context, detail);
    qWarning().noquote() << m_lastError;
}
