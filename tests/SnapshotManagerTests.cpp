#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QTemporaryDir>
#include <QtTest>

#include "core/video/SnapshotManager.h"

class SnapshotManagerTests : public QObject
{
    Q_OBJECT

private slots:
    void saveSnapshotCreatesDateDirectoryAndFile();
    void saveSnapshotRejectsEmptyImage();
};

void SnapshotManagerTests::saveSnapshotCreatesDateDirectoryAndFile()
{
    QTemporaryDir temporaryDir;
    QVERIFY(temporaryDir.isValid());

    QImage image(16, 12, QImage::Format_RGB32);
    image.fill(Qt::red);

    SnapshotManager manager(temporaryDir.path());
    const SnapshotResult result = manager.saveSnapshot(image,
                                                       QStringLiteral("source 1"),
                                                       QStringLiteral("manual"),
                                                       QStringLiteral("png"));

    QVERIFY2(result.success, qPrintable(result.errorMessage));
    QVERIFY(QFileInfo::exists(result.filePath));
    QVERIFY(result.filePath.contains(QDate::currentDate().toString(QStringLiteral("yyyy-MM-dd"))));
    QVERIFY(result.filePath.contains(QStringLiteral("source_1")));
    QVERIFY(result.filePath.contains(QStringLiteral("manual")));
    QVERIFY(result.filePath.endsWith(QStringLiteral(".png")));
}

void SnapshotManagerTests::saveSnapshotRejectsEmptyImage()
{
    QTemporaryDir temporaryDir;
    QVERIFY(temporaryDir.isValid());

    SnapshotManager manager(temporaryDir.path());
    const SnapshotResult result = manager.saveSnapshot(QImage(),
                                                       QStringLiteral("source1"),
                                                       QStringLiteral("manual"),
                                                       QStringLiteral("jpg"));

    QVERIFY(!result.success);
    QVERIFY(!result.errorMessage.isEmpty());
}

QTEST_MAIN(SnapshotManagerTests)

#include "SnapshotManagerTests.moc"
