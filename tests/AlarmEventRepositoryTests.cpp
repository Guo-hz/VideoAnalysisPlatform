#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QUuid>
#include <QtTest>

#include "core/storage/AlarmEventRepository.h"

class AlarmEventRepositoryTests : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();
    void addLoadConfirmAndRemoveAlarmEvent();

private:
    QString m_connectionName;
};

void AlarmEventRepositoryTests::init()
{
    m_connectionName = QStringLiteral("AlarmEventRepositoryTests_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
}

void AlarmEventRepositoryTests::cleanup()
{
    if (QSqlDatabase::contains(m_connectionName)) {
        QSqlDatabase database = QSqlDatabase::database(m_connectionName);
        database.close();
        database = QSqlDatabase();
        QSqlDatabase::removeDatabase(m_connectionName);
    }
}

void AlarmEventRepositoryTests::addLoadConfirmAndRemoveAlarmEvent()
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

    AlarmEvent event;
    event.sourceId = 7;
    event.sourceName = QStringLiteral("测试源");
    event.eventType = QStringLiteral("ManualTest");
    event.level = QStringLiteral("Warning");
    event.message = QStringLiteral("测试告警");
    event.snapshotPath = QStringLiteral("data/snapshots/test.jpg");
    event.videoClipPath = QStringLiteral("data/records/test.mp4");
    event.confirmed = false;
    event.timestamp = QStringLiteral("2026-07-02T12:00:00Z");

    QVERIFY(repository.add(&event));
    QVERIFY(event.id > 0);

    QVector<AlarmEvent> events = repository.loadAll();
    QCOMPARE(events.size(), 1);
    QCOMPARE(events.first().sourceId, 7);
    QCOMPARE(events.first().sourceName, QStringLiteral("测试源"));
    QCOMPARE(events.first().eventType, QStringLiteral("ManualTest"));
    QVERIFY(!events.first().confirmed);

    QVERIFY(repository.confirm(event.id));
    events = repository.loadAll();
    QCOMPARE(events.size(), 1);
    QVERIFY(events.first().confirmed);

    QVERIFY(repository.remove(event.id));
    events = repository.loadAll();
    QVERIFY(events.isEmpty());
}

QTEST_MAIN(AlarmEventRepositoryTests)

#include "AlarmEventRepositoryTests.moc"
