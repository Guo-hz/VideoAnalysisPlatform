#include "VideoSourceRepository.h"

#include <algorithm>
#include <QDateTime>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <QDebug>
#include <QUrl>

#include "core/security/CredentialStore.h"

VideoSourceRepository::VideoSourceRepository(const QSqlDatabase &database)
    : m_database(database)
{
}

QVector<VideoSource> VideoSourceRepository::loadAll()
{
    QVector<VideoSource> sources;
    if (!ensureDatabaseOpen()) {
        return sources;
    }

    QSqlQuery query(m_database);
    if (!query.exec(QStringLiteral(
            "SELECT id, name, type, url, credential_ref, enabled, reconnect_max_attempts, "
            "reconnect_interval_ms, connect_timeout_ms, read_timeout_ms "
            "FROM video_sources ORDER BY id ASC"))) {
        setLastError(QStringLiteral("读取视频源失败"), query.lastError().text());
        return sources;
    }

    while (query.next()) {
        VideoSource source;
        if (readSource(query, &source)) {
            sources.append(source);
        }
    }

    m_lastError.clear();
    return sources;
}

bool VideoSourceRepository::findById(int id, VideoSource *source)
{
    if (!source || id <= 0 || !ensureDatabaseOpen()) {
        return false;
    }
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(R"(
        SELECT id, name, type, url, credential_ref, enabled, reconnect_max_attempts,
               reconnect_interval_ms, connect_timeout_ms, read_timeout_ms
        FROM video_sources WHERE id = :id
    )"));
    query.bindValue(QStringLiteral(":id"), id);
    if (!query.exec() || !query.next()) {
        setLastError(QStringLiteral("读取视频源失败"),
                     query.lastError().text().isEmpty() ? QStringLiteral("视频源不存在。")
                                                        : query.lastError().text());
        return false;
    }
    m_lastError.clear();
    return readSource(query, source);
}

bool VideoSourceRepository::add(VideoSource *source)
{
    if (!source) {
        setLastError(QStringLiteral("新增视频源失败"), QStringLiteral("source 为空。"));
        return false;
    }
    if (!ensureDatabaseOpen()) {
        return false;
    }

    const QString now = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(R"(
        INSERT INTO video_sources (
            name, type, url, credential_ref, enabled, reconnect_max_attempts,
            reconnect_interval_ms, connect_timeout_ms, read_timeout_ms, created_at, updated_at
        )
        VALUES (
            :name, :type, :url, :credential_ref, :enabled, :reconnect_max_attempts,
            :reconnect_interval_ms, :connect_timeout_ms, :read_timeout_ms, :created_at, :updated_at
        )
    )"));
    query.bindValue(QStringLiteral(":name"), source->name);
    query.bindValue(QStringLiteral(":type"), videoSourceTypeToString(source->type));
    query.bindValue(QStringLiteral(":url"), source->url);
    query.bindValue(QStringLiteral(":credential_ref"), source->credentialRef);
    query.bindValue(QStringLiteral(":enabled"), source->enabled ? 1 : 0);
    query.bindValue(QStringLiteral(":reconnect_max_attempts"), std::clamp(source->reconnectMaxAttempts, 0, 20));
    query.bindValue(QStringLiteral(":reconnect_interval_ms"), std::clamp(source->reconnectIntervalMs, 250, 60000));
    query.bindValue(QStringLiteral(":connect_timeout_ms"), std::clamp(source->connectTimeoutMs, 1000, 60000));
    query.bindValue(QStringLiteral(":read_timeout_ms"), std::clamp(source->readTimeoutMs, 1000, 120000));
    query.bindValue(QStringLiteral(":created_at"), now);
    query.bindValue(QStringLiteral(":updated_at"), now);

    if (!query.exec()) {
        setLastError(QStringLiteral("新增视频源失败"), query.lastError().text());
        return false;
    }

    source->id = query.lastInsertId().toInt();
    m_lastError.clear();
    return true;
}

bool VideoSourceRepository::update(const VideoSource &source)
{
    if (source.id <= 0 || !ensureDatabaseOpen()) {
        return false;
    }
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(R"(
        UPDATE video_sources
        SET name = :name, type = :type, url = :url, credential_ref = :credential_ref,
            enabled = :enabled, reconnect_max_attempts = :reconnect_max_attempts,
            reconnect_interval_ms = :reconnect_interval_ms,
            connect_timeout_ms = :connect_timeout_ms, read_timeout_ms = :read_timeout_ms,
            updated_at = :updated_at
        WHERE id = :id
    )"));
    query.bindValue(QStringLiteral(":name"), source.name);
    query.bindValue(QStringLiteral(":type"), videoSourceTypeToString(source.type));
    query.bindValue(QStringLiteral(":url"), source.url);
    query.bindValue(QStringLiteral(":credential_ref"), source.credentialRef);
    query.bindValue(QStringLiteral(":enabled"), source.enabled ? 1 : 0);
    query.bindValue(QStringLiteral(":reconnect_max_attempts"), std::clamp(source.reconnectMaxAttempts, 0, 20));
    query.bindValue(QStringLiteral(":reconnect_interval_ms"), std::clamp(source.reconnectIntervalMs, 250, 60000));
    query.bindValue(QStringLiteral(":connect_timeout_ms"), std::clamp(source.connectTimeoutMs, 1000, 60000));
    query.bindValue(QStringLiteral(":read_timeout_ms"), std::clamp(source.readTimeoutMs, 1000, 120000));
    query.bindValue(QStringLiteral(":updated_at"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    query.bindValue(QStringLiteral(":id"), source.id);
    if (!query.exec()) {
        setLastError(QStringLiteral("更新视频源失败"), query.lastError().text());
        return false;
    }
    m_lastError.clear();
    return true;
}

bool VideoSourceRepository::remove(int id)
{
    if (!ensureDatabaseOpen()) {
        return false;
    }

    VideoSource existing;
    if (!findById(id, &existing)) {
        return false;
    }
    CredentialStore::Secret oldSecret;
    QString credentialError;
    const bool hadCredential = !existing.credentialRef.isEmpty()
                               && CredentialStore::read(existing.credentialRef,
                                                        &oldSecret,
                                                        &credentialError);
    if (!existing.credentialRef.isEmpty()
        && !CredentialStore::remove(existing.credentialRef, &credentialError)) {
        setLastError(QStringLiteral("删除视频源凭据失败"), credentialError);
        return false;
    }

    QSqlQuery query(m_database);
    query.prepare(QStringLiteral("DELETE FROM video_sources WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), id);

    if (!query.exec()) {
        if (hadCredential) {
            CredentialStore::write(existing.credentialRef, oldSecret, nullptr);
        }
        setLastError(QStringLiteral("删除视频源失败"), query.lastError().text());
        return false;
    }

    m_lastError.clear();
    return true;
}

bool VideoSourceRepository::secureLegacyNetworkEndpoints()
{
    QVector<VideoSource> sources = loadAll();
    if (!m_lastError.isEmpty()) {
        return false;
    }
    for (VideoSource &source : sources) {
        const QString type = videoSourceTypeToString(source.type);
        if (!CredentialStore::isNetworkType(type) || !source.credentialRef.isEmpty()) {
            continue;
        }
        CredentialStore::Secret secret;
        secret.endpoint = source.url;
        const QUrl parsed(source.url);
        secret.username = parsed.userName();
        secret.password = parsed.password();
        const QString reference = CredentialStore::newReference();
        QString error;
        if (!CredentialStore::write(reference, secret, &error)) {
            setLastError(QStringLiteral("迁移网络视频源凭据失败"), error);
            return false;
        }
        source.credentialRef = reference;
        source.url = CredentialStore::sanitizeEndpoint(secret.endpoint);
        if (!update(source)) {
            CredentialStore::remove(reference, nullptr);
            return false;
        }
    }
    m_lastError.clear();
    return true;
}

bool VideoSourceRepository::updateName(int id, const QString &name)
{
    if (!ensureDatabaseOpen()) {
        return false;
    }

    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(R"(
        UPDATE video_sources
        SET name = :name, updated_at = :updated_at
        WHERE id = :id
    )"));
    query.bindValue(QStringLiteral(":name"), name);
    query.bindValue(QStringLiteral(":updated_at"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    query.bindValue(QStringLiteral(":id"), id);

    if (!query.exec()) {
        setLastError(QStringLiteral("修改视频源名称失败"), query.lastError().text());
        return false;
    }

    m_lastError.clear();
    return true;
}

QString VideoSourceRepository::lastError() const
{
    return m_lastError;
}

bool VideoSourceRepository::readSource(QSqlQuery &query, VideoSource *source) const
{
    if (!source) {
        return false;
    }
    source->id = query.value(0).toInt();
    source->name = query.value(1).toString();
    source->type = videoSourceTypeFromString(query.value(2).toString());
    source->url = query.value(3).toString();
    source->displayUrl = source->url;
    source->credentialRef = query.value(4).toString();
    source->enabled = query.value(5).toInt() != 0;
    source->reconnectMaxAttempts = query.value(6).toInt();
    source->reconnectIntervalMs = query.value(7).toInt();
    source->connectTimeoutMs = query.value(8).toInt();
    source->readTimeoutMs = query.value(9).toInt();
    source->status = source->enabled ? VideoSourceStatus::Stopped : VideoSourceStatus::Disabled;
    return true;
}

bool VideoSourceRepository::ensureDatabaseOpen()
{
    if (m_database.isValid() && m_database.isOpen()) {
        return true;
    }

    setLastError(QStringLiteral("数据库操作失败"), QStringLiteral("数据库未打开。"));
    return false;
}

void VideoSourceRepository::setLastError(const QString &context, const QString &detail)
{
    m_lastError = QStringLiteral("%1：%2").arg(context, detail);
    qWarning().noquote() << m_lastError;
}
