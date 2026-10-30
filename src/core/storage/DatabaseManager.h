#pragma once

#include <QSqlDatabase>
#include <QString>

class DatabaseManager
{
public:
    static constexpr int CurrentSchemaVersion = 3;

    DatabaseManager();
    ~DatabaseManager();

    bool open(const QString &databasePath = QStringLiteral("data/app.db"));
    bool initialize();

    QSqlDatabase database() const;
    QString databasePath() const;
    QString lastError() const;
    bool isOpen() const;

private:
    bool ensureParentDirectory(const QString &databasePath);
    bool configureConnection();
    bool migrate(int fromVersion);
    bool migrateLegacyRoiRules();
    bool tableExists(const QString &tableName) const;
    bool columnExists(const QString &tableName, const QString &columnName) const;
    bool addColumnIfMissing(const QString &tableName,
                            const QString &columnName,
                            const QString &definition);
    bool execute(const QString &sql, const QString &context);
    bool backupBeforeMigration(int fromVersion);
    int schemaVersion() const;
    void setSchemaVersion(int version);
    void setLastError(const QString &message);

    QString m_connectionName;
    QString m_databasePath;
    QString m_lastError;
    QSqlDatabase m_database;
};
