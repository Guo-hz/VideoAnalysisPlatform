#include "PerformanceModel.h"

#include "core/video/StreamSession.h"
#include "core/video/StreamSessionManager.h"

PerformanceModel::PerformanceModel(StreamSessionManager *sessionManager, QObject *parent)
    : QAbstractListModel(parent)
    , m_sessionManager(sessionManager)
{
    refresh();

    m_refreshTimer.setInterval(1000);
    m_refreshTimer.setTimerType(Qt::CoarseTimer);
    connect(&m_refreshTimer, &QTimer::timeout, this, &PerformanceModel::refresh);
    m_refreshTimer.start();
}

int PerformanceModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return m_items.size();
}

QVariant PerformanceModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size()) {
        return {};
    }

    const StreamPerformance &performance = m_items.at(index.row());
    switch (role) {
    case SourceNameRole:
        return performance.sourceName;
    case FpsRole:
        return performance.fps;
    case DecodeTimeMsRole:
        return performance.decodeTimeMs;
    case QueueSizeRole:
        return performance.queueSize;
    case DroppedFramesRole:
        return performance.droppedFrames;
    case StatusRole:
        return performance.status;
    default:
        return {};
    }
}

QHash<int, QByteArray> PerformanceModel::roleNames() const
{
    return {
        {SourceNameRole, "sourceName"},
        {FpsRole, "fps"},
        {DecodeTimeMsRole, "decodeTimeMs"},
        {QueueSizeRole, "queueSize"},
        {DroppedFramesRole, "droppedFrames"},
        {StatusRole, "status"},
    };
}

void PerformanceModel::refresh()
{
    if (!m_sessionManager) {
        if (!m_items.isEmpty()) {
            beginResetModel();
            m_items.clear();
            endResetModel();
        }
        return;
    }

    const QVector<StreamSession *> sessions = m_sessionManager->sessions();
    if (m_items.size() != sessions.size()) {
        beginResetModel();
        m_items.clear();
        m_items.reserve(sessions.size());
        for (StreamSession *session : sessions) {
            m_items.append(session ? session->performanceSnapshot() : StreamPerformance{});
        }
        endResetModel();
        return;
    }

    for (int index = 0; index < sessions.size(); ++index) {
        m_items[index] = sessions.at(index) ? sessions.at(index)->performanceSnapshot() : StreamPerformance{};
    }

    if (!m_items.isEmpty()) {
        emit dataChanged(this->index(0, 0),
                         this->index(m_items.size() - 1, 0),
                         {SourceNameRole, FpsRole, DecodeTimeMsRole, QueueSizeRole, DroppedFramesRole, StatusRole});
    }
}
