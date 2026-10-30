#include "AppPaths.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSet>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QStringList>
#include <QUuid>
#include <QVector>

#include <algorithm>

namespace {
bool copyDirectory(const QString &sourcePath, const QString &targetPath, QString *error)
{
    const QDir source(sourcePath);
    if (!source.exists()) {
        return true;
    }
    if (!QDir().mkpath(targetPath)) {
        if (error) {
            *error = QStringLiteral("创建迁移目录失败：%1").arg(targetPath);
        }
        return false;
    }

    const QFileInfoList entries = source.entryInfoList(
        QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);
    for (const QFileInfo &entry : entries) {
        const QString target = QDir(targetPath).filePath(entry.fileName());
        if (entry.isDir()) {
            if (!copyDirectory(entry.absoluteFilePath(), target, error)) {
                return false;
            }
            continue;
        }
        if (!QFile::copy(entry.absoluteFilePath(), target)) {
            if (error) {
                *error = QStringLiteral("迁移文件失败：%1").arg(entry.absoluteFilePath());
            }
            return false;
        }
    }
    return true;
}

bool sqliteDatabaseLooksValid(const QString &databasePath)
{
    const QFileInfo file(databasePath);
    if (!file.isFile() || file.size() <= 0) {
        return false;
    }
    if (!QSqlDatabase::isDriverAvailable(QStringLiteral("QSQLITE"))) {
        return true;
    }

    const QString connectionName = QStringLiteral("LegacyProbe-%1")
                                       .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    bool valid = false;
    {
        QSqlDatabase database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
        database.setConnectOptions(QStringLiteral("QSQLITE_OPEN_READONLY"));
        database.setDatabaseName(file.absoluteFilePath());
        if (database.open()) {
            QSqlQuery query(database);
            const bool quickCheck = query.exec(QStringLiteral("PRAGMA quick_check")) && query.next()
                                    && query.value(0).toString().compare(QStringLiteral("ok"), Qt::CaseInsensitive) == 0;
            bool hasSources = false;
            if (quickCheck && query.exec(QStringLiteral("PRAGMA table_info(video_sources)"))) {
                QSet<QString> columns;
                while (query.next()) {
                    columns.insert(query.value(1).toString().toLower());
                }
                const QSet<QString> required = {QStringLiteral("id"),
                                                QStringLiteral("name"),
                                                QStringLiteral("type"),
                                                QStringLiteral("url"),
                                                QStringLiteral("enabled"),
                                                QStringLiteral("created_at"),
                                                QStringLiteral("updated_at")};
                hasSources = std::all_of(required.cbegin(), required.cend(),
                                         [&columns](const QString &column) { return columns.contains(column); });
            }
            valid = quickCheck && hasSources;
            database.close();
        }
    }
    QSqlDatabase::removeDatabase(connectionName);
    return valid;
}
}

AppPaths::AppPaths(QObject *parent)
    : QObject(parent)
    , m_rootDirectory(QDir::cleanPath(
          QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)))
{
}

bool AppPaths::prepare()
{
    m_lastError.clear();
    if (m_rootDirectory.isEmpty()) {
        m_lastError = QStringLiteral("系统未返回可写的应用数据目录。");
        return false;
    }

    if (!QFileInfo::exists(databasePath()) && !migrateNewestLegacyDirectory()) {
        return false;
    }
    return ensureRuntimeDirectories(m_rootDirectory);
}

QString AppPaths::rootDirectory() const
{
    return m_rootDirectory;
}

QString AppPaths::databasePath() const
{
    return QDir(m_rootDirectory).filePath(QStringLiteral("app.db"));
}

QString AppPaths::snapshotDirectory() const
{
    return QDir(m_rootDirectory).filePath(QStringLiteral("snapshots"));
}

QString AppPaths::recordingDirectory() const
{
    return QDir(m_rootDirectory).filePath(QStringLiteral("records"));
}

QString AppPaths::modelDirectory() const
{
    return QDir(m_rootDirectory).filePath(QStringLiteral("models"));
}

QString AppPaths::logDirectory() const
{
    return QDir(m_rootDirectory).filePath(QStringLiteral("logs"));
}

QString AppPaths::migratedFrom() const
{
    return m_migratedFrom;
}

QString AppPaths::lastError() const
{
    return m_lastError;
}

bool AppPaths::ensureRuntimeDirectories(const QString &rootPath)
{
    const QStringList directories = {
        rootPath,
        QDir(rootPath).filePath(QStringLiteral("snapshots")),
        QDir(rootPath).filePath(QStringLiteral("records")),
        QDir(rootPath).filePath(QStringLiteral("models")),
        QDir(rootPath).filePath(QStringLiteral("logs")),
    };
    for (const QString &directory : directories) {
        if (!QDir().mkpath(directory)) {
            m_lastError = QStringLiteral("创建运行目录失败：%1").arg(directory);
            return false;
        }
    }
    return true;
}

bool AppPaths::migrateNewestLegacyDirectory()
{
    const QString targetCanonical = QFileInfo(m_rootDirectory).absoluteFilePath();
    const QString currentDirectory = QDir::currentPath();
    const QString applicationDirectory = QCoreApplication::applicationDirPath();
    const QStringList configurations = {QStringLiteral("Debug"),
                                        QStringLiteral("Release"),
                                        QStringLiteral("RelWithDebInfo"),
                                        QStringLiteral("MinSizeRel")};
    QStringList rawCandidates;
    rawCandidates.append(QDir(currentDirectory).filePath(QStringLiteral("data")));
    rawCandidates.append(QDir(applicationDirectory).filePath(QStringLiteral("data")));
    for (const QString &configuration : configurations) {
        rawCandidates.append(QDir(currentDirectory).filePath(
            QStringLiteral("build/%1/data").arg(configuration)));
        rawCandidates.append(QDir(applicationDirectory).filePath(
            QStringLiteral("../%1/data").arg(configuration)));
    }
    rawCandidates.append(QDir(applicationDirectory).filePath(QStringLiteral("../../data")));

    QVector<QFileInfo> candidates;
    QSet<QString> seen;
    for (const QString &candidatePath : rawCandidates) {
        const QFileInfo directory(candidatePath);
        const QString absolute = directory.absoluteFilePath();
        if (seen.contains(absolute) || absolute == targetCanonical) {
            continue;
        }
        seen.insert(absolute);
        const QFileInfo database(QDir(absolute).filePath(QStringLiteral("app.db")));
        if (sqliteDatabaseLooksValid(database.absoluteFilePath())) {
            candidates.append(database);
        }
    }

    if (candidates.isEmpty()) {
        return true;
    }
    std::sort(candidates.begin(), candidates.end(), [](const QFileInfo &left, const QFileInfo &right) {
        if (left.lastModified() == right.lastModified()) {
            return left.absoluteFilePath() < right.absoluteFilePath();
        }
        return left.lastModified() > right.lastModified();
    });

    const QString sourceRoot = candidates.constFirst().absoluteDir().absolutePath();
    const QString temporaryRoot = QStringLiteral("%1.migrating-%2")
                                      .arg(m_rootDirectory,
                                           QUuid::createUuid().toString(QUuid::WithoutBraces));
    QString copyError;
    if (!copyDirectory(sourceRoot, temporaryRoot, &copyError)
        || !ensureRuntimeDirectories(temporaryRoot)) {
        m_lastError = copyError.isEmpty() ? m_lastError : copyError;
        return false;
    }

    if (QFileInfo::exists(m_rootDirectory)) {
        const QString preservedRoot = QStringLiteral("%1.preexisting-%2")
                                          .arg(m_rootDirectory,
                                               QDateTime::currentDateTimeUtc().toString(
                                                   QStringLiteral("yyyyMMdd-HHmmsszzz")));
        if (!QDir().rename(m_rootDirectory, preservedRoot)) {
            m_lastError = QStringLiteral("保留既有应用数据目录失败：%1").arg(m_rootDirectory);
            return false;
        }
    }
    if (!QDir().rename(temporaryRoot, m_rootDirectory)) {
        m_lastError = QStringLiteral("切换迁移数据目录失败：%1").arg(m_rootDirectory);
        return false;
    }

    m_migratedFrom = sourceRoot;
    return true;
}
