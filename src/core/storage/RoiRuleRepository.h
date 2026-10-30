#pragma once

#include <QSqlDatabase>
#include <QString>
#include <QVector>

#include "core/analysis/RoiRule.h"

class QSqlQuery;

class RoiRuleRepository
{
public:
    explicit RoiRuleRepository(const QSqlDatabase &database);

    QVector<RoiRule> loadAll();
    QVector<RoiRule> loadBySource(int sourceId);
    bool add(RoiRule *rule);
    bool update(const RoiRule &rule);
    bool remove(int id);
    bool removeBySource(int sourceId);

    QString lastError() const;

private:
    QVector<RoiRule> loadByQuery(QSqlQuery &query, const QString &context);
    bool ensureDatabaseOpen();
    void setLastError(const QString &context, const QString &detail);

    QSqlDatabase m_database;
    QString m_lastError;
};
