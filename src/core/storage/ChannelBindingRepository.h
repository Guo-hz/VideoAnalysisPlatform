#pragma once

#include <QHash>
#include <QSqlDatabase>
#include <QString>

class ChannelBindingRepository
{
public:
    explicit ChannelBindingRepository(const QSqlDatabase &database);

    QHash<int, int> loadAll();
    bool bind(int channelIndex, int sourceId);
    bool unbindChannel(int channelIndex);
    bool unbindSource(int sourceId);
    QString lastError() const;

private:
    bool ensureDatabaseOpen();
    void setLastError(const QString &context, const QString &detail);

    QSqlDatabase m_database;
    QString m_lastError;
};
