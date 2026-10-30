#pragma once

#include <QObject>
#include <QString>

class AppPaths : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString rootDirectory READ rootDirectory CONSTANT)
    Q_PROPERTY(QString databasePath READ databasePath CONSTANT)
    Q_PROPERTY(QString snapshotDirectory READ snapshotDirectory CONSTANT)
    Q_PROPERTY(QString recordingDirectory READ recordingDirectory CONSTANT)
    Q_PROPERTY(QString modelDirectory READ modelDirectory CONSTANT)
    Q_PROPERTY(QString logDirectory READ logDirectory CONSTANT)
    Q_PROPERTY(QString migratedFrom READ migratedFrom CONSTANT)

public:
    explicit AppPaths(QObject *parent = nullptr);

    bool prepare();

    QString rootDirectory() const;
    QString databasePath() const;
    QString snapshotDirectory() const;
    QString recordingDirectory() const;
    QString modelDirectory() const;
    QString logDirectory() const;
    QString migratedFrom() const;
    QString lastError() const;

private:
    bool ensureRuntimeDirectories(const QString &rootPath);
    bool migrateNewestLegacyDirectory();

    QString m_rootDirectory;
    QString m_migratedFrom;
    QString m_lastError;
};
