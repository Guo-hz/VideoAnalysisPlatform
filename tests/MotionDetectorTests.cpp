#include <QImage>
#include <QPainter>
#include <QtTest>

#include "core/analysis/MotionDetector.h"

class MotionDetectorTests : public QObject
{
    Q_OBJECT

private slots:
    void detectsChangedRegion();
    void resetClearsPreviousFrame();
};

void MotionDetectorTests::detectsChangedRegion()
{
    MotionDetector detector;

    QImage firstFrame(120, 90, QImage::Format_RGB32);
    firstFrame.fill(Qt::black);
    const DetectionResult firstResult = detector.analyze(firstFrame, QStringLiteral("stream1"), 50);
    QVERIFY(firstResult.boxes.isEmpty());

    QImage secondFrame(120, 90, QImage::Format_RGB32);
    secondFrame.fill(Qt::black);
    QPainter painter(&secondFrame);
    painter.fillRect(QRect(30, 20, 36, 28), Qt::white);
    painter.end();

    const DetectionResult secondResult = detector.analyze(secondFrame, QStringLiteral("stream1"), 50);

#ifdef VIDEOANALYSIS_HAS_OPENCV
    QVERIFY(!secondResult.boxes.isEmpty());
    QCOMPARE(secondResult.sourceId, QStringLiteral("stream1"));
    QVERIFY(secondResult.timestamp > 0);
#else
    QVERIFY(secondResult.boxes.isEmpty());
#endif
}

void MotionDetectorTests::resetClearsPreviousFrame()
{
    MotionDetector detector;

    QImage frame(64, 64, QImage::Format_RGB32);
    frame.fill(Qt::black);
    detector.analyze(frame, QStringLiteral("stream1"), 20);
    detector.reset();

    QImage changedFrame(64, 64, QImage::Format_RGB32);
    changedFrame.fill(Qt::white);
    const DetectionResult result = detector.analyze(changedFrame, QStringLiteral("stream1"), 20);
    QVERIFY(result.boxes.isEmpty());
}

QTEST_MAIN(MotionDetectorTests)

#include "MotionDetectorTests.moc"
