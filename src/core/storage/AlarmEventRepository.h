#pragma once

#include <QSqlDatabase>
#include <QString>
#include <QVector>

#include "core/alarm/AlarmEvent.h"

class AlarmEventRepository
{
public:
    explicit AlarmEventRepository(const QSqlDatabase &database);

    QVector<AlarmEvent> loadAll();
    bool add(AlarmEvent *event);
    bool confirm(int id);
    bool remove(int id);

    QString lastError() const;

private:
    bool ensureDatabaseOpen();
    void setLastError(const QString &context, const QString &detail);

    QSqlDatabase m_database;
    QString m_lastError;
};
