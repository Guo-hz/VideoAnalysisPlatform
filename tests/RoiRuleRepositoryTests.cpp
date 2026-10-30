#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QUuid>
#include <QtTest>

#include "core/storage/RoiRuleRepository.h"

class RoiRuleRepositoryTests : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();
    void addLoadUpdateAndRemoveRoiRule();

private:
    QString m_connectionName;
};

void RoiRuleRepositoryTests::init()
{
    m_connectionName = QStringLiteral("RoiRuleRepositoryTests_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
}

void RoiRuleRepositoryTests::cleanup()
{
    if (QSqlDatabase::contains(m_connectionName)) {
        QSqlDatabase database = QSqlDatabase::database(m_connectionName);
        database.close();
        database = QSqlDatabase();
        QSqlDatabase::removeDatabase(m_connectionName);
    }
}

void RoiRuleRepositoryTests::addLoadUpdateAndRemoveRoiRule()
{
    QTemporaryDir temporaryDir;
    QVERIFY(temporaryDir.isValid());

    QSqlDatabase database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    database.setDatabaseName(temporaryDir.filePath(QStringLiteral("roi.db")));
    QVERIFY(database.open());

    QSqlQuery query(database);
    QVERIFY(query.exec(QStringLiteral(R"(
        CREATE TABLE IF NOT EXISTS roi_rules (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            source_id INTEGER NOT NULL,
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
    )")));

    RoiRuleRepository repository(database);

    RoiRule rule;
    rule.sourceId = 2;
    rule.ruleName = QStringLiteral("Gate");
    rule.rect = QRectF(0.25, 0.2, 0.4, 0.3);
    rule.enabled = true;
    rule.cooldownSeconds = 8;

    QVERIFY(repository.add(&rule));
    QVERIFY(rule.id > 0);
    QCOMPARE(rule.cooldownSeconds, 8);

    QVector<RoiRule> sourceRules = repository.loadBySource(2);
    QCOMPARE(sourceRules.size(), 1);
    QCOMPARE(sourceRules.first().id, rule.id);
    QCOMPARE(sourceRules.first().ruleName, QStringLiteral("Gate"));
    QCOMPARE(sourceRules.first().rect, QRectF(0.25, 0.2, 0.4, 0.3));

    rule.ruleName = QStringLiteral("Gate Updated");
    rule.enabled = false;
    rule.cooldownSeconds = 20;
    QVERIFY(repository.update(rule));

    sourceRules = repository.loadBySource(2);
    QCOMPARE(sourceRules.size(), 1);
    QCOMPARE(sourceRules.first().ruleName, QStringLiteral("Gate Updated"));
    QVERIFY(!sourceRules.first().enabled);
    QCOMPARE(sourceRules.first().cooldownSeconds, 20);

    QVERIFY(repository.remove(rule.id));
    sourceRules = repository.loadBySource(2);
    QVERIFY(sourceRules.isEmpty());
}

QTEST_MAIN(RoiRuleRepositoryTests)

#include "RoiRuleRepositoryTests.moc"
