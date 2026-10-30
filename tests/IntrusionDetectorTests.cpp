#include <QtTest>

#include "core/analysis/IntrusionDetector.h"

class IntrusionDetectorTests : public QObject
{
    Q_OBJECT

private slots:
    void detectsBoxCenterInsideRoi();
    void ignoresBoxCenterOutsideRoi();
    void ignoresDisabledRoiRule();
};

void IntrusionDetectorTests::detectsBoxCenterInsideRoi()
{
    DetectionResult result;
    result.sourceId = QStringLiteral("stream1");
    result.timestamp = 1234;
    result.boxes.append(QRect(40, 40, 20, 20));

    RoiRule rule;
    rule.id = 7;
    rule.sourceId = 1;
    rule.ruleName = QStringLiteral("Gate");
    rule.rect = QRectF(0.3, 0.3, 0.4, 0.4);
    rule.enabled = true;

    IntrusionDetector detector;
    const QVector<IntrusionMatch> matches = detector.detect(result, QSize(100, 100), {rule});
    QCOMPARE(matches.size(), 1);
    QCOMPARE(matches.first().rule.id, 7);
    QCOMPARE(matches.first().box, QRect(40, 40, 20, 20));
    QCOMPARE(matches.first().timestamp, 1234);
}

void IntrusionDetectorTests::ignoresBoxCenterOutsideRoi()
{
    DetectionResult result;
    result.sourceId = QStringLiteral("stream1");
    result.timestamp = 1234;
    result.boxes.append(QRect(5, 5, 10, 10));

    RoiRule rule;
    rule.id = 7;
    rule.sourceId = 1;
    rule.ruleName = QStringLiteral("Gate");
    rule.rect = QRectF(0.3, 0.3, 0.4, 0.4);
    rule.enabled = true;

    IntrusionDetector detector;
    const QVector<IntrusionMatch> matches = detector.detect(result, QSize(100, 100), {rule});
    QVERIFY(matches.isEmpty());
}

void IntrusionDetectorTests::ignoresDisabledRoiRule()
{
    DetectionResult result;
    result.sourceId = QStringLiteral("stream1");
    result.timestamp = 1234;
    result.boxes.append(QRect(40, 40, 20, 20));

    RoiRule rule;
    rule.id = 7;
    rule.sourceId = 1;
    rule.ruleName = QStringLiteral("Gate");
    rule.rect = QRectF(0.3, 0.3, 0.4, 0.4);
    rule.enabled = false;

    IntrusionDetector detector;
    const QVector<IntrusionMatch> matches = detector.detect(result, QSize(100, 100), {rule});
    QVERIFY(matches.isEmpty());
}

QTEST_MAIN(IntrusionDetectorTests)

#include "IntrusionDetectorTests.moc"
