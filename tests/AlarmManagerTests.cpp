#include <QFileInfo>
#include <QDateTime>
#include <QSignalSpy>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QUuid>
#include <QtTest>

#include "core/alarm/AlarmManager.h"
#include "core/storage/AlarmEventRepository.h"

class AlarmManagerTests : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();
    void createsMotionAlarmWithSnapshotAndCooldown();
    void createsIntrusionAlarmWithRuleCooldown();
    void createsObjectAlarmWithCooldown();

private:
    QString m_connectionName;
};

void AlarmManagerTests::init()
{
    m_connectionName = QStringLiteral("AlarmManagerTests_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    qRegisterMetaType<AlarmEvent>("AlarmEvent");
}

void AlarmManagerTests::cleanup()
{
    if (QSqlDatabase::contains(m_connectionName)) {
        QSqlDatabase database = QSqlDatabase::database(m_connectionName);
        database.close();
        database = QSqlDatabase();
        QSqlDatabase::removeDatabase(m_connectionName);
    }
}

void AlarmManagerTests::createsMotionAlarmWithSnapshotAndCooldown()
{
    QTemporaryDir temporaryDir;
    QVERIFY(temporaryDir.isValid());

    QSqlDatabase database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    database.setDatabaseName(temporaryDir.filePath(QStringLiteral("alarms.db")));
    QVERIFY(database.open());

    QSqlQuery query(database);
    QVERIFY(query.exec(QStringLiteral(R"(
        CREATE TABLE IF NOT EXISTS alarm_events (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            source_id INTEGER NOT NULL,
            video_source_id INTEGER NULL,
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
    )")));

    AlarmEventRepository repository(database);
    AlarmManager manager(&repository, temporaryDir.filePath(QStringLiteral("snapshots")));
    manager.setCooldownSeconds(10);

    QSignalSpy createdSpy(&manager, &AlarmManager::alarmCreated);
    QImage frame(32, 32, QImage::Format_RGB32);
    frame.fill(Qt::red);

    const qint64 timestamp = QDateTime::currentMSecsSinceEpoch();
    manager.reportMotionDetected(QStringLiteral("stream1"),
                                 1,
                                 QStringLiteral("测试通道"),
                                 frame,
                                 2,
                                 timestamp);

    QTRY_COMPARE(createdSpy.count(), 1);

    QVector<AlarmEvent> events = repository.loadAll();
    QCOMPARE(events.size(), 1);
    QCOMPARE(events.first().sourceId, 1);
    QCOMPARE(events.first().sourceName, QStringLiteral("测试通道"));
    QCOMPARE(events.first().eventType, QStringLiteral("MotionDetected"));
    QCOMPARE(events.first().level, QStringLiteral("Warning"));
    QVERIFY(events.first().snapshotPath.endsWith(QStringLiteral(".jpg")));
    QVERIFY(QFileInfo::exists(events.first().snapshotPath));

    manager.reportMotionDetected(QStringLiteral("stream1"),
                                 1,
                                 QStringLiteral("测试通道"),
                                 frame,
                                 1,
                                 timestamp + 1000);
    QTest::qWait(200);

    events = repository.loadAll();
    QCOMPARE(events.size(), 1);
    QCOMPARE(createdSpy.count(), 1);
}

void AlarmManagerTests::createsIntrusionAlarmWithRuleCooldown()
{
    QTemporaryDir temporaryDir;
    QVERIFY(temporaryDir.isValid());

    QSqlDatabase database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    database.setDatabaseName(temporaryDir.filePath(QStringLiteral("intrusion_alarms.db")));
    QVERIFY(database.open());

    QSqlQuery query(database);
    QVERIFY(query.exec(QStringLiteral(R"(
        CREATE TABLE IF NOT EXISTS alarm_events (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            source_id INTEGER NOT NULL,
            video_source_id INTEGER NULL,
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
    )")));

    AlarmEventRepository repository(database);
    AlarmManager manager(&repository, temporaryDir.filePath(QStringLiteral("snapshots")));

    QSignalSpy createdSpy(&manager, &AlarmManager::alarmCreated);
    QImage frame(32, 32, QImage::Format_RGB32);
    frame.fill(Qt::blue);

    const qint64 timestamp = QDateTime::currentMSecsSinceEpoch();
    manager.reportIntrusionDetected(QStringLiteral("stream1"),
                                    1,
                                    QStringLiteral("测试通道"),
                                    9,
                                    QStringLiteral("Gate"),
                                    10,
                                    frame,
                                    1,
                                    timestamp);

    QTRY_COMPARE(createdSpy.count(), 1);

    QVector<AlarmEvent> events = repository.loadAll();
    QCOMPARE(events.size(), 1);
    QCOMPARE(events.first().eventType, QStringLiteral("IntrusionDetected"));
    QVERIFY(events.first().message.contains(QStringLiteral("Gate")));
    QVERIFY(events.first().snapshotPath.endsWith(QStringLiteral(".jpg")));
    QVERIFY(QFileInfo::exists(events.first().snapshotPath));

    manager.reportIntrusionDetected(QStringLiteral("stream1"),
                                    1,
                                    QStringLiteral("测试通道"),
                                    9,
                                    QStringLiteral("Gate"),
                                    10,
                                    frame,
                                    1,
                                    timestamp + 1000);
    QTest::qWait(200);

    events = repository.loadAll();
    QCOMPARE(events.size(), 1);
    QCOMPARE(createdSpy.count(), 1);
}

void AlarmManagerTests::createsObjectAlarmWithCooldown()
{
    QTemporaryDir temporaryDir;
    QVERIFY(temporaryDir.isValid());

    QSqlDatabase database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    database.setDatabaseName(temporaryDir.filePath(QStringLiteral("object_alarms.db")));
    QVERIFY(database.open());

    QSqlQuery query(database);
    QVERIFY(query.exec(QStringLiteral(R"(
        CREATE TABLE IF NOT EXISTS alarm_events (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            source_id INTEGER NOT NULL,
            video_source_id INTEGER NULL,
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
    )")));

    AlarmEventRepository repository(database);
    AlarmManager manager(&repository, temporaryDir.filePath(QStringLiteral("snapshots")));

    QSignalSpy createdSpy(&manager, &AlarmManager::alarmCreated);
    QImage frame(32, 32, QImage::Format_RGB32);
    frame.fill(Qt::green);

    const qint64 timestamp = QDateTime::currentMSecsSinceEpoch();
    manager.reportObjectDetected(QStringLiteral("stream1"),
                                 1,
                                 QStringLiteral("测试通道"),
                                 QStringLiteral("PersonDetected"),
                                 QStringLiteral("person"),
                                 0.88,
                                 frame,
                                 1,
                                 timestamp);

    QTRY_COMPARE(createdSpy.count(), 1);

    QVector<AlarmEvent> events = repository.loadAll();
    QCOMPARE(events.size(), 1);
    QCOMPARE(events.first().eventType, QStringLiteral("PersonDetected"));
    QVERIFY(events.first().message.contains(QStringLiteral("person")));
    QVERIFY(events.first().snapshotPath.endsWith(QStringLiteral(".jpg")));
    QVERIFY(QFileInfo::exists(events.first().snapshotPath));

    manager.reportObjectDetected(QStringLiteral("stream1"),
                                 1,
                                 QStringLiteral("测试通道"),
                                 QStringLiteral("PersonDetected"),
                                 QStringLiteral("person"),
                                 0.90,
                                 frame,
                                 1,
                                 timestamp + 1000);
    QTest::qWait(200);

    events = repository.loadAll();
    QCOMPARE(events.size(), 1);
    QCOMPARE(createdSpy.count(), 1);
}

QTEST_MAIN(AlarmManagerTests)

#include "AlarmManagerTests.moc"
