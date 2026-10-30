#include "AlarmEventModel.h"

#include <algorithm>
#include <utility>

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>

AlarmEventModel::AlarmEventModel(AlarmEventRepository *repository, QObject *parent)
    : QAbstractListModel(parent)
    , m_repository(repository)
{
    loadFromRepository();
}

int AlarmEventModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return m_events.size();
}

QVariant AlarmEventModel::data(const QModelIndex &index, int role) const
{
    if (!isValidRow(index.row())) {
        return {};
    }

    const AlarmEvent &event = m_events.at(index.row());
    switch (role) {
    case IdRole:
        return event.id;
    case SourceIdRole:
        return event.sourceId;
    case VideoSourceIdRole:
        return event.videoSourceId;
    case LegacySourceKeyRole:
        return event.legacySourceKey;
    case SourceNameRole:
        return event.sourceName;
    case EventTypeRole:
        return event.eventType;
    case LevelRole:
        return event.level;
    case MessageRole:
        return event.message;
    case SnapshotPathRole:
        return event.snapshotPath;
    case VideoClipPathRole:
        return event.videoClipPath;
    case ConfirmedRole:
        return event.confirmed;
    case TimestampRole:
        return event.timestamp;
    default:
        return {};
    }
}

QHash<int, QByteArray> AlarmEventModel::roleNames() const
{
    return {
        {IdRole, "alarmId"},
        {SourceIdRole, "sourceId"},
        {VideoSourceIdRole, "videoSourceId"},
        {LegacySourceKeyRole, "legacySourceKey"},
        {SourceNameRole, "sourceName"},
        {EventTypeRole, "eventType"},
        {LevelRole, "level"},
        {MessageRole, "message"},
        {SnapshotPathRole, "snapshotPath"},
        {VideoClipPathRole, "videoClipPath"},
        {ConfirmedRole, "confirmed"},
        {TimestampRole, "timestamp"},
    };
}

int AlarmEventModel::count() const
{
    return m_events.size();
}

QString AlarmEventModel::lastError() const
{
    return m_lastError;
}

QString AlarmEventModel::eventTypeFilter() const
{
    return m_eventTypeFilter;
}

QString AlarmEventModel::sourceFilter() const
{
    return m_sourceFilter;
}

int AlarmEventModel::confirmedFilter() const
{
    return m_confirmedFilter;
}

QString AlarmEventModel::mediaDirectory() const
{
    return m_mediaDirectory;
}

void AlarmEventModel::setMediaDirectory(const QString &value)
{
    const QString normalized = QDir::cleanPath(value.trimmed());
    if (m_mediaDirectory == normalized) {
        return;
    }
    m_mediaDirectory = normalized;
    emit mediaDirectoryChanged();
}

void AlarmEventModel::setEventTypeFilter(const QString &value)
{
    const QString normalized = value.trimmed();
    if (m_eventTypeFilter == normalized) {
        return;
    }
    m_eventTypeFilter = normalized;
    applyFilters();
    emit filtersChanged();
}

void AlarmEventModel::setSourceFilter(const QString &value)
{
    const QString normalized = value.trimmed();
    if (m_sourceFilter == normalized) {
        return;
    }
    m_sourceFilter = normalized;
    applyFilters();
    emit filtersChanged();
}

void AlarmEventModel::setConfirmedFilter(int value)
{
    const int normalized = value < 0 ? -1 : (value > 0 ? 1 : 0);
    if (m_confirmedFilter == normalized) {
        return;
    }
    m_confirmedFilter = normalized;
    applyFilters();
    emit filtersChanged();
}

int AlarmEventModel::addTestAlarm()
{
    const int alarmNumber = m_nextTestAlarmIndex++;

    AlarmEvent event;
    event.sourceId = alarmNumber;
    event.sourceName = QStringLiteral("测试视频源 %1").arg(alarmNumber);
    event.eventType = QStringLiteral("ManualTest");
    event.level = alarmNumber % 3 == 0 ? QStringLiteral("Critical")
                                       : alarmNumber % 2 == 0 ? QStringLiteral("Warning")
                                                              : QStringLiteral("Info");
    event.message = QStringLiteral("阶段 10 手动插入的测试告警 %1").arg(alarmNumber);
    event.snapshotPath = QString();
    event.videoClipPath = QString();
    event.confirmed = false;
    event.timestamp = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

    if (!m_repository || !m_repository->add(&event)) {
        setLastError(m_repository ? m_repository->lastError() : QStringLiteral("告警事件仓库未初始化。"));
        return -1;
    }

    m_allEvents.prepend(event);
    applyFilters();
    clearLastError();
    return 0;
}

bool AlarmEventModel::confirmAlarm(int row)
{
    if (!isValidRow(row)) {
        setLastError(QStringLiteral("确认告警失败：索引无效。"));
        return false;
    }

    AlarmEvent &event = m_events[row];
    if (event.confirmed) {
        clearLastError();
        return true;
    }

    if (!m_repository || !m_repository->confirm(event.id)) {
        setLastError(m_repository ? m_repository->lastError() : QStringLiteral("告警事件仓库未初始化。"));
        return false;
    }

    event.confirmed = true;
    for (AlarmEvent &allEvent : m_allEvents) {
        if (allEvent.id == event.id) {
            allEvent.confirmed = true;
            break;
        }
    }
    applyFilters();
    clearLastError();
    return true;
}

bool AlarmEventModel::removeAlarm(int row)
{
    if (!isValidRow(row)) {
        setLastError(QStringLiteral("删除告警失败：索引无效。"));
        return false;
    }

    const int id = m_events.at(row).id;
    if (!m_repository || !m_repository->remove(id)) {
        setLastError(m_repository ? m_repository->lastError() : QStringLiteral("告警事件仓库未初始化。"));
        return false;
    }

    m_allEvents.erase(std::remove_if(m_allEvents.begin(), m_allEvents.end(), [id](const AlarmEvent &event) {
        return event.id == id;
    }), m_allEvents.end());
    applyFilters();
    clearLastError();
    return true;
}

bool AlarmEventModel::removeAlarmWithMedia(int row, bool deleteMedia)
{
    if (!isValidRow(row)) {
        setLastError(QStringLiteral("删除告警失败：索引无效。"));
        return false;
    }
    const AlarmEvent event = m_events.at(row);
    if (deleteMedia && !event.snapshotPath.isEmpty() && !m_mediaDirectory.isEmpty()) {
        const QString root = QFileInfo(m_mediaDirectory).canonicalFilePath();
        const QString file = QFileInfo(event.snapshotPath).canonicalFilePath();
        const QString prefix = root.endsWith(QDir::separator()) ? root : root + QDir::separator();
        bool referenced = false;
        for (const AlarmEvent &other : std::as_const(m_allEvents)) {
            if (other.id != event.id && other.snapshotPath == event.snapshotPath) {
                referenced = true;
                break;
            }
        }
        if (!root.isEmpty() && !file.isEmpty() && file.startsWith(prefix, Qt::CaseInsensitive)
            && !referenced) {
            QFile::remove(file);
        }
    }
    return removeAlarm(row);
}

void AlarmEventModel::reload()
{
    loadFromRepository();
}

void AlarmEventModel::addAlarm(const AlarmEvent &event)
{
    m_allEvents.prepend(event);
    applyFilters();
    clearLastError();
}

bool AlarmEventModel::isValidRow(int row) const
{
    return row >= 0 && row < m_events.size();
}

void AlarmEventModel::loadFromRepository()
{
    if (!m_repository) {
        setLastError(QStringLiteral("告警事件仓库未初始化。"));
        return;
    }

    m_allEvents = m_repository->loadAll();
    applyFilters();

    int maxTestIndex = 0;
    for (const AlarmEvent &event : std::as_const(m_allEvents)) {
        if (event.eventType == QStringLiteral("ManualTest")) {
            maxTestIndex = std::max(maxTestIndex, event.sourceId);
        }
    }
    m_nextTestAlarmIndex = maxTestIndex + 1;

    if (!m_repository->lastError().isEmpty()) {
        setLastError(m_repository->lastError());
    } else {
        clearLastError();
    }
}

bool AlarmEventModel::matchesFilters(const AlarmEvent &event) const
{
    if (!m_eventTypeFilter.isEmpty() && event.eventType != m_eventTypeFilter) {
        return false;
    }
    if (!m_sourceFilter.isEmpty()
        && !event.sourceName.contains(m_sourceFilter, Qt::CaseInsensitive)
        && !event.legacySourceKey.contains(m_sourceFilter, Qt::CaseInsensitive)
        && QString::number(event.videoSourceId > 0 ? event.videoSourceId : event.sourceId) != m_sourceFilter) {
        return false;
    }
    if (m_confirmedFilter >= 0 && (event.confirmed ? 1 : 0) != m_confirmedFilter) {
        return false;
    }
    return true;
}

void AlarmEventModel::applyFilters()
{
    beginResetModel();
    m_events.clear();
    for (const AlarmEvent &event : std::as_const(m_allEvents)) {
        if (matchesFilters(event)) {
            m_events.append(event);
        }
    }
    endResetModel();
    emit countChanged();
}

void AlarmEventModel::clearLastError()
{
    if (m_lastError.isEmpty()) {
        return;
    }

    m_lastError.clear();
    emit lastErrorChanged();
}

void AlarmEventModel::setLastError(const QString &message)
{
    if (m_lastError == message) {
        return;
    }

    m_lastError = message;
    emit lastErrorChanged();
}
