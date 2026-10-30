#include "RecordingModel.h"

#include <QDateTime>
#include <QDirIterator>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <QUuid>

#include <utility>

RecordingModel::RecordingModel(const QSqlDatabase &database,
                               QString recordingDirectory,
                               QObject *parent)
    : QAbstractListModel(parent)
    , m_database(database)
    , m_recordingDirectory(std::move(recordingDirectory))
{
    reload();
}

int RecordingModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_items.size();
}

QVariant RecordingModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size()) {
        return {};
    }
    const RecordingItem &item = m_items.at(index.row());
    switch (role) {
    case IdRole: return item.id;
    case SourceIdRole: return item.sourceId;
    case SourceNameRole: return item.sourceName;
    case PathRole: return item.path;
    case StartedAtRole: return item.startedAt;
    case EndedAtRole: return item.endedAt;
    case DurationMsRole: return item.durationMs;
    case WidthRole: return item.width;
    case HeightRole: return item.height;
    case FpsRole: return item.fps;
    case StatusRole: return item.status;
    default: return {};
    }
}

QHash<int, QByteArray> RecordingModel::roleNames() const
{
    return {{IdRole, "recordingId"}, {SourceIdRole, "sourceId"},
            {SourceNameRole, "sourceName"}, {PathRole, "path"},
            {StartedAtRole, "startedAt"}, {EndedAtRole, "endedAt"},
            {DurationMsRole, "durationMs"}, {WidthRole, "width"},
            {HeightRole, "height"}, {FpsRole, "fps"}, {StatusRole, "status"}};
}

int RecordingModel::count() const
{
    return m_items.size();
}

QString RecordingModel::lastError() const
{
    return m_lastError;
}

void RecordingModel::reload()
{
    beginResetModel();
    m_items.clear();
    loadFromDatabase();
    endResetModel();
    emit countChanged();
}

bool RecordingModel::reindex()
{
    if (!ensureDatabaseOpen()) {
        return false;
    }

    QDirIterator iterator(m_recordingDirectory,
                          QStringList{QStringLiteral("*.avi")},
                          QDir::Files,
                          QDirIterator::Subdirectories);
    bool success = true;
    while (iterator.hasNext()) {
        if (!insertIndexRow(QFileInfo(iterator.next()).absoluteFilePath())) {
            success = false;
        }
    }
    reload();
    return success;
}

QVariantMap RecordingModel::itemAt(int row) const
{
    QVariantMap map;
    if (row < 0 || row >= m_items.size()) {
        return map;
    }
    const RecordingItem &item = m_items.at(row);
    map.insert(QStringLiteral("recordingId"), item.id);
    map.insert(QStringLiteral("sourceId"), item.sourceId);
    map.insert(QStringLiteral("sourceName"), item.sourceName);
    map.insert(QStringLiteral("path"), item.path);
    map.insert(QStringLiteral("startedAt"), item.startedAt);
    map.insert(QStringLiteral("endedAt"), item.endedAt);
    map.insert(QStringLiteral("durationMs"), item.durationMs);
    map.insert(QStringLiteral("status"), item.status);
    return map;
}

bool RecordingModel::ensureDatabaseOpen()
{
    if (m_database.isValid() && m_database.isOpen()) {
        return true;
    }
    setLastError(QStringLiteral("录像索引数据库未打开。"));
    return false;
}

bool RecordingModel::loadFromDatabase()
{
    if (!ensureDatabaseOpen()) {
        return false;
    }
    QSqlQuery query(m_database);
    if (!query.exec(QStringLiteral(
            "SELECT id, source_id, source_name, path, started_at, ended_at, duration_ms, "
            "width, height, fps, status FROM recordings ORDER BY started_at DESC"))) {
        setLastError(QStringLiteral("读取录像索引失败：%1").arg(query.lastError().text()));
        return false;
    }
    while (query.next()) {
        RecordingItem item;
        item.id = query.value(0).toString();
        item.sourceId = query.value(1).toInt();
        item.sourceName = query.value(2).toString();
        item.path = query.value(3).toString();
        item.startedAt = query.value(4).toString();
        item.endedAt = query.value(5).toString();
        item.durationMs = query.value(6).toLongLong();
        item.width = query.value(7).toInt();
        item.height = query.value(8).toInt();
        item.fps = query.value(9).toDouble();
        item.status = query.value(10).toString();
        if (QFileInfo::exists(item.path)) {
            m_items.append(item);
        }
    }
    clearLastError();
    return true;
}

bool RecordingModel::insertIndexRow(const QString &path)
{
    const QFileInfo file(path);
    if (!file.isFile()) {
        return false;
    }
    const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const QString startedAt = file.birthTime().isValid()
                                  ? file.birthTime().toUTC().toString(Qt::ISODate)
                                  : file.lastModified().toUTC().toString(Qt::ISODate);
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "INSERT OR IGNORE INTO recordings "
        "(id, source_id, source_name, path, started_at, status) "
        "VALUES (:id, 0, :source_name, :path, :started_at, 'Completed')"));
    query.bindValue(QStringLiteral(":id"), id);
    query.bindValue(QStringLiteral(":source_name"), QStringLiteral("Indexed recording"));
    query.bindValue(QStringLiteral(":path"), file.absoluteFilePath());
    query.bindValue(QStringLiteral(":started_at"), startedAt);
    if (!query.exec()) {
        setLastError(QStringLiteral("写入录像索引失败：%1").arg(query.lastError().text()));
        return false;
    }
    return true;
}

void RecordingModel::setLastError(const QString &message)
{
    if (m_lastError == message) {
        return;
    }
    m_lastError = message;
    emit lastErrorChanged();
}

void RecordingModel::clearLastError()
{
    if (!m_lastError.isEmpty()) {
        m_lastError.clear();
        emit lastErrorChanged();
    }
}
