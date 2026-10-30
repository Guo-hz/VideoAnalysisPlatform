#include "VideoSourceModel.h"

#include <algorithm>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTimer>
#include <QUrl>
#include <utility>

#include "core/security/CredentialStore.h"

namespace {
bool parseType(const QString &value, VideoSourceType *type)
{
    if (!type) {
        return false;
    }
    const QString normalized = value.trimmed().toLower();
    if (normalized == QStringLiteral("localfile") || normalized == QStringLiteral("local")
        || normalized == QStringLiteral("file")) {
        *type = VideoSourceType::LocalFile;
        return true;
    }
    if (normalized == QStringLiteral("camera")) {
        *type = VideoSourceType::Camera;
        return true;
    }
    if (normalized == QStringLiteral("rtsp")) {
        *type = VideoSourceType::Rtsp;
        return true;
    }
    if (normalized == QStringLiteral("http") || normalized == QStringLiteral("https")) {
        *type = VideoSourceType::Http;
        return true;
    }
    return false;
}
}

VideoSourceModel::VideoSourceModel(VideoSourceRepository *repository, QObject *parent)
    : QAbstractListModel(parent)
    , m_repository(repository)
{
    loadFromRepository();
}

int VideoSourceModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_sources.size();
}

QVariant VideoSourceModel::data(const QModelIndex &index, int role) const
{
    if (!isValidRow(index.row())) {
        return {};
    }
    const VideoSource &source = m_sources.at(index.row());
    switch (role) {
    case IdRole:
        return source.id;
    case NameRole:
        return source.name;
    case TypeRole:
        return videoSourceTypeToString(source.type);
    case UrlRole:
        return source.url;
    case EnabledRole:
    case SourceEnabledRole:
        return source.enabled;
    case StatusRole:
        return videoSourceStatusToString(source.status);
    case LastErrorRole:
        return source.lastError;
    case CredentialRefRole:
        return source.credentialRef;
    case ReconnectMaxAttemptsRole:
        return source.reconnectMaxAttempts;
    case ReconnectIntervalMsRole:
        return source.reconnectIntervalMs;
    default:
        return {};
    }
}

bool VideoSourceModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!isValidRow(index.row())) {
        return false;
    }
    if (role == NameRole) {
        return renameSource(index.row(), value.toString());
    }
    if (role == EnabledRole || role == SourceEnabledRole) {
        return setSourceEnabled(index.row(), value.toBool());
    }
    return false;
}

Qt::ItemFlags VideoSourceModel::flags(const QModelIndex &index) const
{
    return index.isValid() ? Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable
                           : Qt::NoItemFlags;
}

QHash<int, QByteArray> VideoSourceModel::roleNames() const
{
    return {
        {IdRole, "sourceId"},
        {NameRole, "name"},
        {TypeRole, "type"},
        {UrlRole, "url"},
        {EnabledRole, "enabled"},
        {StatusRole, "status"},
        {LastErrorRole, "lastError"},
        {SourceEnabledRole, "sourceEnabled"},
        {CredentialRefRole, "credentialRef"},
        {ReconnectMaxAttemptsRole, "reconnectMaxAttempts"},
        {ReconnectIntervalMsRole, "reconnectIntervalMs"},
    };
}

int VideoSourceModel::count() const
{
    return m_sources.size();
}

QString VideoSourceModel::lastError() const
{
    return m_lastError;
}

QVariantMap VideoSourceModel::sourceAt(int row) const
{
    QVariantMap result;
    if (!isValidRow(row)) {
        return result;
    }
    const VideoSource &source = m_sources.at(row);
    result.insert(QStringLiteral("sourceId"), source.id);
    result.insert(QStringLiteral("name"), source.name);
    result.insert(QStringLiteral("type"), videoSourceTypeToString(source.type));
    result.insert(QStringLiteral("url"), source.url);
    result.insert(QStringLiteral("enabled"), source.enabled);
    result.insert(QStringLiteral("status"), videoSourceStatusToString(source.status));
    result.insert(QStringLiteral("credentialRef"), source.credentialRef);
    result.insert(QStringLiteral("reconnectMaxAttempts"), source.reconnectMaxAttempts);
    result.insert(QStringLiteral("reconnectIntervalMs"), source.reconnectIntervalMs);
    return result;
}

int VideoSourceModel::addSource(const QString &name,
                                const QString &type,
                                const QString &endpoint,
                                const QString &username,
                                const QString &password)
{
    VideoSource source;
    if (!buildSource(&source, name, type, endpoint, username, password)) {
        return -1;
    }
    if (!m_repository || !m_repository->add(&source)) {
        if (!source.credentialRef.isEmpty()) {
            CredentialStore::remove(source.credentialRef, nullptr);
        }
        setLastError(m_repository ? m_repository->lastError() : QStringLiteral("视频源仓库未初始化。"));
        return -1;
    }
    appendSource(source);
    emit sourceChanged(source.id);
    clearLastError();
    return m_sources.size() - 1;
}

bool VideoSourceModel::updateSource(int row,
                                    const QString &name,
                                    const QString &type,
                                    const QString &endpoint,
                                    const QString &username,
                                    const QString &password)
{
    if (!isValidRow(row)) {
        setLastError(QStringLiteral("更新视频源失败：索引无效。"));
        return false;
    }
    const VideoSource old = m_sources.at(row);
    VideoSource updated = old;
    if (!buildSource(&updated, name, type, endpoint, username, password, &old)) {
        return false;
    }
    if (!m_repository || !m_repository->update(updated)) {
        if (updated.credentialRef != old.credentialRef && !updated.credentialRef.isEmpty()) {
            CredentialStore::remove(updated.credentialRef, nullptr);
        }
        setLastError(m_repository ? m_repository->lastError() : QStringLiteral("视频源仓库未初始化。"));
        return false;
    }
    if (updated.credentialRef != old.credentialRef && !old.credentialRef.isEmpty()) {
        CredentialStore::remove(old.credentialRef, nullptr);
    }
    m_sources[row] = updated;
    const QModelIndex changed = index(row, 0);
    emit dataChanged(changed, changed);
    emit sourceChanged(updated.id);
    clearLastError();
    return true;
}

bool VideoSourceModel::setSourceEnabled(int row, bool enabled)
{
    if (!isValidRow(row)) {
        setLastError(QStringLiteral("更新视频源状态失败：索引无效。"));
        return false;
    }
    VideoSource updated = m_sources.at(row);
    updated.enabled = enabled;
    if (!m_repository || !m_repository->update(updated)) {
        setLastError(m_repository ? m_repository->lastError() : QStringLiteral("视频源仓库未初始化。"));
        return false;
    }
    m_sources[row] = updated;
    const QModelIndex changed = index(row, 0);
    emit dataChanged(changed, changed, {EnabledRole, SourceEnabledRole});
    emit sourceChanged(updated.id);
    clearLastError();
    return true;
}

bool VideoSourceModel::removeSource(int row)
{
    if (!isValidRow(row)) {
        setLastError(QStringLiteral("删除视频源失败：索引无效。"));
        return false;
    }
    const int id = m_sources.at(row).id;
    if (!m_repository || !m_repository->remove(id)) {
        setLastError(m_repository ? m_repository->lastError() : QStringLiteral("视频源仓库未初始化。"));
        return false;
    }
    beginRemoveRows(QModelIndex(), row, row);
    m_sources.removeAt(row);
    endRemoveRows();
    emit countChanged();
    emit sourceChanged(id);
    clearLastError();
    return true;
}

bool VideoSourceModel::renameSource(int row, const QString &name)
{
    if (!isValidRow(row)) {
        setLastError(QStringLiteral("修改视频源名称失败：索引无效。"));
        return false;
    }
    const QString trimmed = name.trimmed();
    if (trimmed.isEmpty()) {
        setLastError(QStringLiteral("修改视频源名称失败：名称不能为空。"));
        return false;
    }
    VideoSource updated = m_sources.at(row);
    updated.name = trimmed;
    if (!m_repository || !m_repository->update(updated)) {
        setLastError(m_repository ? m_repository->lastError() : QStringLiteral("视频源仓库未初始化。"));
        return false;
    }
    m_sources[row] = updated;
    const QModelIndex changed = index(row, 0);
    emit dataChanged(changed, changed, {NameRole});
    emit sourceChanged(updated.id);
    clearLastError();
    return true;
}

bool VideoSourceModel::testSource(int row)
{
    if (!isValidRow(row)) {
        setLastError(QStringLiteral("连接测试失败：源索引无效。"));
        return false;
    }

    const VideoSource source = m_sources.at(row);
    const int sourceId = source.id;
    QString error;
    bool success = true;
    if (source.type == VideoSourceType::LocalFile) {
        success = QFileInfo::exists(source.url);
        if (!success) {
            error = QStringLiteral("本地文件不存在：%1").arg(source.url);
        }
    } else if (source.type == VideoSourceType::Camera) {
        bool ok = false;
        const QString value = source.url.trimmed().startsWith(QStringLiteral("camera://"), Qt::CaseInsensitive)
                                  ? source.url.trimmed().mid(QStringLiteral("camera://").size())
                                  : source.url.trimmed();
        const int index = value.toInt(&ok);
        success = ok && index >= 0;
        if (!success) {
            error = QStringLiteral("Camera 需要非负设备索引。" );
        }
    } else {
        const QUrl url(source.url);
        const QString scheme = url.scheme().toLower();
        success = url.isValid() && !url.host().isEmpty()
                  && ((source.type == VideoSourceType::Rtsp && scheme == QStringLiteral("rtsp"))
                      || (source.type == VideoSourceType::Http
                          && (scheme == QStringLiteral("http") || scheme == QStringLiteral("https"))));
        if (!success) {
            error = QStringLiteral("网络源地址或协议无效。" );
        }
    }

    QTimer::singleShot(0, this, [this, sourceId, success, error]() {
        const QString message = success ? QStringLiteral("源配置有效，已通过本地连接检查。") : error;
        if (success) {
            clearLastError();
        } else {
            setLastError(message);
        }
        emit sourceTestFinished(sourceId, success, message);
    });
    return true;
}

int VideoSourceModel::addLocalPlaceholderSource()
{
    const int number = m_nextPlaceholderIndex++;
    return addSource(QStringLiteral("本地视频源 %1").arg(number),
                     QStringLiteral("LocalFile"),
                     QStringLiteral("placeholder/local-video-%1.mp4").arg(number));
}

int VideoSourceModel::addRtspPlaceholderSource()
{
    return addSource(QStringLiteral("RTSP 视频源 %1").arg(m_sources.size() + 1),
                     QStringLiteral("Rtsp"),
                     QStringLiteral("rtsp://example.com/stream"));
}

bool VideoSourceModel::isValidRow(int row) const
{
    return row >= 0 && row < m_sources.size();
}

bool VideoSourceModel::buildSource(VideoSource *source,
                                   const QString &name,
                                   const QString &type,
                                   const QString &endpoint,
                                   const QString &username,
                                   const QString &password,
                                   const VideoSource *existing)
{
    if (!source) {
        setLastError(QStringLiteral("视频源对象为空。"));
        return false;
    }
    const QString trimmedName = name.trimmed();
    if (trimmedName.isEmpty()) {
        setLastError(QStringLiteral("视频源名称不能为空。"));
        return false;
    }
    VideoSourceType parsedType;
    if (!parseType(type, &parsedType)) {
        setLastError(QStringLiteral("不支持的视频源类型：%1").arg(type));
        return false;
    }
    *source = existing ? *existing : VideoSource();
    source->name = trimmedName;
    source->type = parsedType;
    source->enabled = existing ? existing->enabled : true;

    const QString trimmedEndpoint = endpoint.trimmed();
    if (existing && trimmedEndpoint.isEmpty() && CredentialStore::isNetworkType(type)) {
        return true;
    }
    if (trimmedEndpoint.isEmpty()) {
        setLastError(QStringLiteral("视频源地址不能为空。"));
        return false;
    }
    if (parsedType == VideoSourceType::LocalFile && !QFileInfo(trimmedEndpoint).exists()) {
        setLastError(QStringLiteral("本地视频文件不存在：%1").arg(trimmedEndpoint));
        return false;
    }
    if (parsedType == VideoSourceType::Rtsp || parsedType == VideoSourceType::Http) {
        QUrl url(trimmedEndpoint);
        const QString scheme = url.scheme().toLower();
        const bool validScheme = (parsedType == VideoSourceType::Rtsp && scheme == QStringLiteral("rtsp"))
                                 || (parsedType == VideoSourceType::Http
                                     && (scheme == QStringLiteral("http") || scheme == QStringLiteral("https")));
        if (!url.isValid() || url.host().isEmpty() || !validScheme) {
            setLastError(QStringLiteral("网络视频源地址无效。"));
            return false;
        }
    }
    if (!persistCredential(source, trimmedEndpoint, username, password, existing)) {
        return false;
    }
    if (parsedType != VideoSourceType::Rtsp && parsedType != VideoSourceType::Http) {
        source->url = trimmedEndpoint;
        source->credentialRef.clear();
    }
    return true;
}

bool VideoSourceModel::persistCredential(VideoSource *source,
                                         const QString &endpoint,
                                         const QString &username,
                                         const QString &password,
                                         const VideoSource *existing)
{
    if (!source || !CredentialStore::isNetworkType(videoSourceTypeToString(source->type))) {
        return true;
    }
    QUrl url(endpoint);
    if (!username.trimmed().isEmpty()) {
        url.setUserName(username.trimmed());
    }
    if (!password.isEmpty()) {
        url.setPassword(password);
    }
    const QString reference = existing && !existing->credentialRef.isEmpty()
                                  ? existing->credentialRef
                                  : CredentialStore::newReference();
    CredentialStore::Secret secret;
    secret.endpoint = url.toString(QUrl::FullyEncoded);
    secret.username = url.userName();
    secret.password = url.password();
    QString error;
    if (!CredentialStore::write(reference, secret, &error)) {
        setLastError(error);
        return false;
    }
    source->credentialRef = reference;
    source->url = CredentialStore::sanitizeEndpoint(secret.endpoint);
    source->displayUrl = source->url;
    return true;
}

void VideoSourceModel::appendSource(const VideoSource &source)
{
    const int row = m_sources.size();
    beginInsertRows(QModelIndex(), row, row);
    m_sources.append(source);
    endInsertRows();
    emit countChanged();
}

void VideoSourceModel::loadFromRepository()
{
    if (!m_repository) {
        setLastError(QStringLiteral("视频源仓库未初始化。"));
        return;
    }
    beginResetModel();
    m_sources = m_repository->loadAll();
    endResetModel();
    emit countChanged();
    int maxIndex = 0;
    const QRegularExpression pattern(QStringLiteral("^本地视频源\\s+(\\d+)$"));
    for (const VideoSource &source : std::as_const(m_sources)) {
        const auto match = pattern.match(source.name);
        if (match.hasMatch()) {
            maxIndex = std::max(maxIndex, match.captured(1).toInt());
        }
    }
    m_nextPlaceholderIndex = maxIndex + 1;
    if (!m_repository->lastError().isEmpty()) {
        setLastError(m_repository->lastError());
    } else {
        clearLastError();
    }
}

void VideoSourceModel::clearLastError()
{
    if (!m_lastError.isEmpty()) {
        m_lastError.clear();
        emit lastErrorChanged();
    }
}

void VideoSourceModel::setLastError(const QString &message)
{
    if (m_lastError == message) {
        return;
    }
    m_lastError = message;
    emit lastErrorChanged();
}
