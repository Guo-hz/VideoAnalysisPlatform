#include <QImage>
#include <QtTest>

#include "core/analysis/OnnxObjectDetector.h"

class OnnxObjectDetectorTests : public QObject
{
    Q_OBJECT

private slots:
    void emptyModelPathFailsGracefully();
    void detectWithoutLoadedModelReturnsEmptyResult();
    void confidenceThresholdIsClamped();
};

void OnnxObjectDetectorTests::emptyModelPathFailsGracefully()
{
    OnnxObjectDetector detector;
    QVERIFY(!detector.loadModel(QString()));
    QVERIFY(!detector.lastError().isEmpty());
    QVERIFY(!detector.isReady());
}

void OnnxObjectDetectorTests::detectWithoutLoadedModelReturnsEmptyResult()
{
    OnnxObjectDetector detector;
    QImage frame(64, 48, QImage::Format_RGB32);
    frame.fill(Qt::black);

    const DetectionResult result = detector.detect(frame, QStringLiteral("stream1"));
    QCOMPARE(result.sourceId, QStringLiteral("stream1"));
    QVERIFY(result.timestamp > 0);
    QVERIFY(result.boxes.isEmpty());
    QVERIFY(result.detections.isEmpty());
}

void OnnxObjectDetectorTests::confidenceThresholdIsClamped()
{
    OnnxObjectDetector detector;
    detector.setConfidenceThreshold(-1.0f);
    QVERIFY(detector.confidenceThreshold() > 0.0f);

    detector.setConfidenceThreshold(2.0f);
    QVERIFY(detector.confidenceThreshold() < 1.0f);

    detector.setConfidenceThreshold(0.62f);
    QCOMPARE(qRound(detector.confidenceThreshold() * 100.0f), 62);
}

QTEST_MAIN(OnnxObjectDetectorTests)

#include "OnnxObjectDetectorTests.moc"
