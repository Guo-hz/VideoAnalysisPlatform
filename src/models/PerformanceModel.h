#pragma once

#include <QAbstractListModel>
#include <QTimer>
#include <QVector>

#include "core/video/StreamPerformance.h"

class StreamSessionManager;

class PerformanceModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role {
        SourceNameRole = Qt::UserRole + 1,
        FpsRole,
        DecodeTimeMsRole,
        QueueSizeRole,
        DroppedFramesRole,
        StatusRole
    };

    explicit PerformanceModel(StreamSessionManager *sessionManager, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

public slots:
    void refresh();

private:
    StreamSessionManager *m_sessionManager = nullptr;
    QVector<StreamPerformance> m_items;
    QTimer m_refreshTimer;
};
