#include "DatabaseManager.h"

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

DatabaseManager::DatabaseManager()
    : m_connectionName(QStringLiteral("VideoAnalysisPlatformConnection-%1")
                           .arg(QUuid::createUuid().toString(QUuid::WithoutBraces)))
{
}

DatabaseManager::~DatabaseManager()
{
    const QString connectionName = m_connectionName;
    if (m_database.isValid()) {
        m_database.close();
        m_database = QSqlDatabase();
    }
    if (QSqlDatabase::contains(connectionName)) {
        QSqlDatabase::removeDatabase(connectionName);
    }
}

bool DatabaseManager::open(const QString &databasePath)
{
    m_lastError.clear();
    if (databasePath == QStringLiteral(":memory:")) {
        m_databasePath = databasePath;
    } else {
        m_databasePath = QFileInfo(databasePath).absoluteFilePath();
    }

    if (!ensureParentDirectory(m_databasePath)) {
        return false;
    }

    const QFileInfo existingFile(m_databasePath);
    const bool existingDatabase = existingFile.isFile() && existingFile.size() > 0;

    m_database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    m_database.setDatabaseName(m_databasePath);
    if (!m_database.open()) {
        setLastError(QStringLiteral("打开 SQLite 数据库失败：%1").arg(m_database.lastError().text()));
        return false;
    }
    if (!configureConnection()) {
        return false;
    }

    const int version = schemaVersion();
    if (version < 0) {
        return false;
    }
    if (existingDatabase && version < CurrentSchemaVersion && !backupBeforeMigration(version)) {
        return false;
    }
    return initialize();
}

bool DatabaseManager::initialize()
{
    if (!m_database.isOpen()) {
        setLastError(QStringLiteral("初始化数据库失败：数据库未打开。"));
        return false;
    }
    const int version = schemaVersion();
    if (version < 0) {
        return false;
    }
    if (!m_database.transaction()) {
        setLastError(QStringLiteral("启动数据库事务失败：%1").arg(m_database.lastError().text()));
        return false;
    }
    if (!migrate(version)) {
        m_database.rollback();
        return false;
    }
    if (!m_database.commit()) {
        setLastError(QStringLiteral("提交数据库迁移失败：%1").arg(m_database.lastError().text()));
        return false;
    }
    return true;
}

QSqlDatabase DatabaseManager::database() const
{
    return m_database;
}

QString DatabaseManager::databasePath() const
{
    return m_databasePath;
}

QString DatabaseManager::lastError() const
{
    return m_lastError;
}

bool DatabaseManager::isOpen() const
{
    return m_database.isValid() && m_database.isOpen();
}

bool DatabaseManager::ensureParentDirectory(const QString &databasePath)
{
    if (databasePath == QStringLiteral(":memory:")) {
        return true;
    }
    const QFileInfo databaseFile(databasePath);
    const QString parentPath = databaseFile.absoluteDir().path();
    if (QDir(parentPath).exists()) {
        return true;
    }
    if (!QDir().mkpath(parentPath)) {
        setLastError(QStringLiteral("创建数据库目录失败：%1").arg(parentPath));
        return false;
    }
    return true;
}

bool DatabaseManager::configureConnection()
{
    QSqlQuery query(m_database);
    const QStringList pragmas = {
        QStringLiteral("PRAGMA foreign_keys = ON"),
        QStringLiteral("PRAGMA busy_timeout = 5000"),
        QStringLiteral("PRAGMA synchronous = NORMAL"),
    };
    for (const QString &pragma : pragmas) {
        if (!query.exec(pragma)) {
            setLastError(QStringLiteral("配置 SQLite 失败：%1").arg(query.lastError().text()));
            return false;
        }
    }
    if (!query.exec(QStringLiteral("PRAGMA journal_mode = WAL"))) {
        setLastError(QStringLiteral("启用 SQLite WAL 失败：%1").arg(query.lastError().text()));
        return false;
    }
    return true;
}

bool DatabaseManager::migrate(int fromVersion)
{
    const bool hasLegacySources = tableExists(QStringLiteral("video_sources"));
    if (!hasLegacySources
        && !execute(QStringLiteral(R"(
            CREATE TABLE video_sources (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                name TEXT NOT NULL,
                type TEXT NOT NULL,
                url TEXT NOT NULL,
                credential_ref TEXT NOT NULL DEFAULT '',
                enabled INTEGER NOT NULL DEFAULT 1,
                reconnect_max_attempts INTEGER NOT NULL DEFAULT 3,
                reconnect_interval_ms INTEGER NOT NULL DEFAULT 3000,
                connect_timeout_ms INTEGER NOT NULL DEFAULT 5000,
                read_timeout_ms INTEGER NOT NULL DEFAULT 10000,
                created_at TEXT NOT NULL,
                updated_at TEXT NOT NULL
            )
        )"),
                       QStringLiteral("创建 video_sources 表失败"))) {
        return false;
    }

    if (!addColumnIfMissing(QStringLiteral("video_sources"),
                            QStringLiteral("credential_ref"),
                            QStringLiteral("TEXT NOT NULL DEFAULT ''"))
        || !addColumnIfMissing(QStringLiteral("video_sources"),
                               QStringLiteral("reconnect_max_attempts"),
                               QStringLiteral("INTEGER NOT NULL DEFAULT 3"))
        || !addColumnIfMissing(QStringLiteral("video_sources"),
                               QStringLiteral("reconnect_interval_ms"),
                               QStringLiteral("INTEGER NOT NULL DEFAULT 3000"))
        || !addColumnIfMissing(QStringLiteral("video_sources"),
                               QStringLiteral("connect_timeout_ms"),
                               QStringLiteral("INTEGER NOT NULL DEFAULT 5000"))
        || !addColumnIfMissing(QStringLiteral("video_sources"),
                               QStringLiteral("read_timeout_ms"),
                               QStringLiteral("INTEGER NOT NULL DEFAULT 10000"))) {
        return false;
    }

    if (!tableExists(QStringLiteral("alarm_events"))) {
        if (!execute(QStringLiteral(R"(
            CREATE TABLE alarm_events (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                source_id INTEGER NOT NULL,
                video_source_id INTEGER NULL REFERENCES video_sources(id) ON DELETE SET NULL,
                legacy_source_key TEXT NOT NULL DEFAULT '',
                source_name TEXT NOT NULL,
                event_type TEXT NOT NULL,
                level TEXT NOT NULL,
                message TEXT,
                snapshot_path TEXT,
                video_clip_path TEXT,
                confirmed INTEGER NOT NULL DEFAULT 0,
                timestamp TEXT NOT NULL
            )
        )"),
                       QStringLiteral("创建 alarm_events 表失败"))) {
            return false;
        }
    } else if (!addColumnIfMissing(QStringLiteral("alarm_events"),
                                   QStringLiteral("video_source_id"),
                                   QStringLiteral("INTEGER NULL"))
               || !addColumnIfMissing(QStringLiteral("alarm_events"),
                                      QStringLiteral("legacy_source_key"),
                                      QStringLiteral("TEXT NOT NULL DEFAULT ''"))) {
        return false;
    }

    if (!tableExists(QStringLiteral("legacy_roi_rules"))
        && !execute(QStringLiteral(R"(
            CREATE TABLE legacy_roi_rules (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                original_rule_id INTEGER NOT NULL,
                legacy_channel INTEGER NOT NULL,
                rule_name TEXT NOT NULL,
                enabled INTEGER NOT NULL DEFAULT 1,
                points_json TEXT NOT NULL,
                cooldown_seconds INTEGER NOT NULL DEFAULT 10,
                assigned_source_id INTEGER NULL REFERENCES video_sources(id) ON DELETE SET NULL,
                assigned_at TEXT NULL,
                created_at TEXT NOT NULL,
                updated_at TEXT NOT NULL,
                UNIQUE(original_rule_id, legacy_channel)
            )
        )"),
                       QStringLiteral("创建 legacy_roi_rules 表失败"))) {
        return false;
    }

    if (fromVersion < CurrentSchemaVersion
        && tableExists(QStringLiteral("roi_rules"))
        && columnExists(QStringLiteral("roi_rules"), QStringLiteral("source_id"))
        && !columnExists(QStringLiteral("roi_rules"), QStringLiteral("assigned_source_id"))) {
        if (!migrateLegacyRoiRules()) {
            return false;
        }
    }

    if (!tableExists(QStringLiteral("roi_rules"))
        && !execute(QStringLiteral(R"(
            CREATE TABLE roi_rules (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                source_id INTEGER NOT NULL REFERENCES video_sources(id) ON DELETE CASCADE,
                rule_name TEXT NOT NULL,
                enabled INTEGER NOT NULL DEFAULT 1,
                roi_type TEXT NOT NULL DEFAULT 'rect',
                points_json TEXT NOT NULL,
                sensitivity REAL NOT NULL DEFAULT 0.5,
                min_target_area INTEGER NOT NULL DEFAULT 0,
                cooldown_seconds INTEGER NOT NULL DEFAULT 10,
                created_at TEXT NOT NULL,
                updated_at TEXT NOT NULL
            )
        )"),
                       QStringLiteral("创建 roi_rules 表失败"))) {
        return false;
    }

    if (!execute(QStringLiteral(R"(
        CREATE TABLE IF NOT EXISTS app_settings (
            key TEXT PRIMARY KEY,
            value TEXT NOT NULL,
            updated_at TEXT NOT NULL
        )
    )"),
                  QStringLiteral("创建 app_settings 表失败"))) {
        return false;
    }
    if (!execute(QStringLiteral(R"(
        CREATE TABLE IF NOT EXISTS channel_bindings (
            channel_index INTEGER PRIMARY KEY CHECK(channel_index BETWEEN 0 AND 3),
            source_id INTEGER UNIQUE NOT NULL REFERENCES video_sources(id) ON DELETE CASCADE,
            updated_at TEXT NOT NULL
        )
    )"),
                  QStringLiteral("创建 channel_bindings 表失败"))) {
        return false;
    }
    if (!execute(QStringLiteral(R"(
        CREATE TABLE IF NOT EXISTS source_analysis_settings (
            source_id INTEGER PRIMARY KEY REFERENCES video_sources(id) ON DELETE CASCADE,
            motion_enabled INTEGER NOT NULL DEFAULT 0,
            mog2_history INTEGER NOT NULL DEFAULT 500,
            mog2_var_threshold REAL NOT NULL DEFAULT 16,
            motion_warmup_frames INTEGER NOT NULL DEFAULT 30,
            motion_confirm_frames INTEGER NOT NULL DEFAULT 3,
            min_target_area INTEGER NOT NULL DEFAULT 800,
            ai_enabled INTEGER NOT NULL DEFAULT 0,
            ai_model_id TEXT NOT NULL DEFAULT 'object_detection_yolox_2022nov',
            ai_confidence REAL NOT NULL DEFAULT 0.5,
            ai_nms_threshold REAL NOT NULL DEFAULT 0.45,
            ai_interval_ms INTEGER NOT NULL DEFAULT 500,
            ai_alert_categories TEXT NOT NULL DEFAULT '["person","bicycle","car","motorcycle","bus","truck"]',
            updated_at TEXT NOT NULL
        )
    )"),
                  QStringLiteral("创建 source_analysis_settings 表失败"))) {
        return false;
    }
    if (!execute(QStringLiteral(R"(
        CREATE TABLE IF NOT EXISTS recordings (
            id TEXT PRIMARY KEY,
            source_id INTEGER NULL REFERENCES video_sources(id) ON DELETE SET NULL,
            source_name TEXT NOT NULL,
            path TEXT NOT NULL UNIQUE,
            started_at TEXT NOT NULL,
            ended_at TEXT,
            duration_ms INTEGER NOT NULL DEFAULT 0,
            width INTEGER NOT NULL DEFAULT 0,
            height INTEGER NOT NULL DEFAULT 0,
            fps REAL NOT NULL DEFAULT 0,
            status TEXT NOT NULL DEFAULT 'Completed'
        )
    )"),
                  QStringLiteral("创建 recordings 表失败"))) {
        return false;
    }

    if (!execute(QStringLiteral("UPDATE alarm_events SET legacy_source_key = 'stream' || source_id WHERE legacy_source_key = ''"),
                 QStringLiteral("补齐告警来源标识失败"))
        || !execute(QStringLiteral("CREATE INDEX IF NOT EXISTS idx_alarm_events_timestamp ON alarm_events(timestamp DESC, id DESC)"),
                    QStringLiteral("创建告警时间索引失败"))
        || !execute(QStringLiteral("CREATE INDEX IF NOT EXISTS idx_alarm_events_source ON alarm_events(video_source_id, event_type, timestamp DESC)"),
                    QStringLiteral("创建告警来源索引失败"))
        || !execute(QStringLiteral("CREATE INDEX IF NOT EXISTS idx_recordings_started ON recordings(started_at DESC)"),
                    QStringLiteral("创建录像索引失败"))
        || !execute(QStringLiteral("CREATE INDEX IF NOT EXISTS idx_roi_rules_source ON roi_rules(source_id, enabled)"),
                    QStringLiteral("创建 ROI 索引失败"))) {
        return false;
    }

    setSchemaVersion(CurrentSchemaVersion);
    return m_lastError.isEmpty();
}

bool DatabaseManager::migrateLegacyRoiRules()
{
    const QString copySql = QStringLiteral(R"(
        INSERT OR IGNORE INTO legacy_roi_rules (
            original_rule_id, legacy_channel, rule_name, enabled,
            points_json, cooldown_seconds, created_at, updated_at
        )
        SELECT id, source_id, rule_name, enabled, points_json,
               cooldown_seconds, created_at, updated_at
        FROM roi_rules
    )");
    if (!execute(copySql, QStringLiteral("迁移旧通道 ROI 失败"))) {
        return false;
    }
    if (!execute(QStringLiteral("DROP TABLE roi_rules"), QStringLiteral("重建旧 ROI 表失败"))) {
        return false;
    }
    return true;
}

bool DatabaseManager::tableExists(const QString &tableName) const
{
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral("SELECT 1 FROM sqlite_master WHERE type = 'table' AND name = :name"));
    query.bindValue(QStringLiteral(":name"), tableName);
    return query.exec() && query.next();
}

bool DatabaseManager::columnExists(const QString &tableName, const QString &columnName) const
{
    QSqlQuery query(m_database);
    if (!query.exec(QStringLiteral("PRAGMA table_info(%1)").arg(tableName))) {
        return false;
    }
    while (query.next()) {
        if (query.value(1).toString().compare(columnName, Qt::CaseInsensitive) == 0) {
            return true;
        }
    }
    return false;
}

bool DatabaseManager::addColumnIfMissing(const QString &tableName,
                                         const QString &columnName,
                                         const QString &definition)
{
    if (!tableExists(tableName) || columnExists(tableName, columnName)) {
        return true;
    }
    return execute(QStringLiteral("ALTER TABLE %1 ADD COLUMN %2 %3")
                       .arg(tableName, columnName, definition),
                   QStringLiteral("扩展 %1 表失败").arg(tableName));
}

bool DatabaseManager::execute(const QString &sql, const QString &context)
{
    QSqlQuery query(m_database);
    if (!query.exec(sql)) {
        setLastError(QStringLiteral("%1：%2").arg(context, query.lastError().text()));
        return false;
    }
    return true;
}

bool DatabaseManager::backupBeforeMigration(int fromVersion)
{
    if (m_databasePath == QStringLiteral(":memory:")) {
        return true;
    }
    QSqlQuery checkpoint(m_database);
    checkpoint.exec(QStringLiteral("PRAGMA wal_checkpoint(TRUNCATE)"));
    m_database.close();

    const QString backupPath = QStringLiteral("%1.bak-v%2-%3")
                                   .arg(m_databasePath)
                                   .arg(fromVersion)
                                   .arg(QDateTime::currentDateTimeUtc().toString(
                                       QStringLiteral("yyyyMMdd-HHmmsszzz")));
    if (!QFile::copy(m_databasePath, backupPath)) {
        m_database.open();
        setLastError(QStringLiteral("迁移前备份数据库失败：%1").arg(backupPath));
        return false;
    }
    if (!m_database.open() || !configureConnection()) {
        setLastError(QStringLiteral("重新打开迁移数据库失败：%1").arg(m_database.lastError().text()));
        return false;
    }
    return true;
}

int DatabaseManager::schemaVersion() const
{
    QSqlQuery query(m_database);
    if (!query.exec(QStringLiteral("PRAGMA user_version")) || !query.next()) {
        const_cast<DatabaseManager *>(this)->setLastError(
            QStringLiteral("读取数据库 schema 版本失败：%1").arg(query.lastError().text()));
        return -1;
    }
    return query.value(0).toInt();
}

void DatabaseManager::setSchemaVersion(int version)
{
    execute(QStringLiteral("PRAGMA user_version = %1").arg(version), QStringLiteral("写入 schema 版本失败"));
}

void DatabaseManager::setLastError(const QString &message)
{
    m_lastError = message;
    qWarning().noquote() << message;
}
