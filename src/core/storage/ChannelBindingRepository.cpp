#include "ChannelBindingRepository.h"

#include <QDateTime>
#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>

ChannelBindingRepository::ChannelBindingRepository(const QSqlDatabase &database)
    : m_database(database)
{
}

QHash<int, int> ChannelBindingRepository::loadAll()
{
    QHash<int, int> bindings;
    if (!ensureDatabaseOpen()) {
        return bindings;
    }
    QSqlQuery query(m_database);
    if (!query.exec(QStringLiteral(
            "SELECT channel_index, source_id FROM channel_bindings ORDER BY channel_index"))) {
        setLastError(QStringLiteral("读取通道绑定失败"), query.lastError().text());
        return bindings;
    }
    while (query.next()) {
        bindings.insert(query.value(0).toInt(), query.value(1).toInt());
    }
    m_lastError.clear();
    return bindings;
}

bool ChannelBindingRepository::bind(int channelIndex, int sourceId)
{
    if (channelIndex < 0 || channelIndex > 3 || sourceId <= 0) {
        setLastError(QStringLiteral("绑定视频源失败"), QStringLiteral("通道或视频源无效。"));
        return false;
    }
    if (!ensureDatabaseOpen() || !m_database.transaction()) {
        setLastError(QStringLiteral("绑定视频源失败"), m_database.lastError().text());
        return false;
    }
    QSqlQuery removeExisting(m_database);
    removeExisting.prepare(QStringLiteral(
        "DELETE FROM channel_bindings WHERE channel_index = :channel OR source_id = :source"));
    removeExisting.bindValue(QStringLiteral(":channel"), channelIndex);
    removeExisting.bindValue(QStringLiteral(":source"), sourceId);
    if (!removeExisting.exec()) {
        m_database.rollback();
        setLastError(QStringLiteral("清理旧通道绑定失败"), removeExisting.lastError().text());
        return false;
    }
    QSqlQuery insert(m_database);
    insert.prepare(QStringLiteral(R"(
        INSERT INTO channel_bindings(channel_index, source_id, updated_at)
        VALUES(:channel, :source, :updated_at)
    )"));
    insert.bindValue(QStringLiteral(":channel"), channelIndex);
    insert.bindValue(QStringLiteral(":source"), sourceId);
    insert.bindValue(QStringLiteral(":updated_at"),
                     QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    if (!insert.exec() || !m_database.commit()) {
        m_database.rollback();
        setLastError(QStringLiteral("保存通道绑定失败"), insert.lastError().text());
        return false;
    }
    m_lastError.clear();
    return true;
}

bool ChannelBindingRepository::unbindChannel(int channelIndex)
{
    if (!ensureDatabaseOpen()) {
        return false;
    }
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral("DELETE FROM channel_bindings WHERE channel_index = :channel"));
    query.bindValue(QStringLiteral(":channel"), channelIndex);
    if (!query.exec()) {
        setLastError(QStringLiteral("解除通道绑定失败"), query.lastError().text());
        return false;
    }
    m_lastError.clear();
    return true;
}

bool ChannelBindingRepository::unbindSource(int sourceId)
{
    if (!ensureDatabaseOpen()) {
        return false;
    }
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral("DELETE FROM channel_bindings WHERE source_id = :source"));
    query.bindValue(QStringLiteral(":source"), sourceId);
    if (!query.exec()) {
        setLastError(QStringLiteral("解除视频源绑定失败"), query.lastError().text());
        return false;
    }
    m_lastError.clear();
    return true;
}

QString ChannelBindingRepository::lastError() const
{
    return m_lastError;
}

bool ChannelBindingRepository::ensureDatabaseOpen()
{
    if (m_database.isValid() && m_database.isOpen()) {
        return true;
    }
    setLastError(QStringLiteral("数据库操作失败"), QStringLiteral("数据库未打开。"));
    return false;
}

void ChannelBindingRepository::setLastError(const QString &context, const QString &detail)
{
    m_lastError = QStringLiteral("%1：%2").arg(context, detail);
    qWarning().noquote() << m_lastError;
}
