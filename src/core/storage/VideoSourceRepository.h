#pragma once

#include <QSqlDatabase>
#include <QString>
#include <QVector>

#include "core/video/VideoSource.h"

class QSqlQuery;

class VideoSourceRepository
{
public:
    explicit VideoSourceRepository(const QSqlDatabase &database);

    QVector<VideoSource> loadAll();
    bool findById(int id, VideoSource *source);
    bool add(VideoSource *source);
    bool update(const VideoSource &source);
    bool remove(int id);
    bool updateName(int id, const QString &name);
    bool secureLegacyNetworkEndpoints();

    QString lastError() const;

private:
    bool ensureDatabaseOpen();
    bool readSource(QSqlQuery &query, VideoSource *source) const;
    void setLastError(const QString &context, const QString &detail);

    QSqlDatabase m_database;
    QString m_lastError;
};
