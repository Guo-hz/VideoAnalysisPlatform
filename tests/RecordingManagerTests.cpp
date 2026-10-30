#include <QDate>
#include <QColor>
#include <QFileInfo>
#include <QImage>
#include <QTemporaryDir>
#include <QtTest>

#include "core/video/RecordingManager.h"

#ifdef VIDEOANALYSIS_HAS_OPENCV
#include <opencv2/videoio.hpp>
#endif

class RecordingManagerTests : public QObject
{
    Q_OBJECT

private slots:
    void manualRecordingCreatesPlayableFileWhenOpenCvIsAvailable();
    void startRecordingRejectsInvalidFrameSize();
};

void RecordingManagerTests::manualRecordingCreatesPlayableFileWhenOpenCvIsAvailable()
{
    QTemporaryDir temporaryDir;
    QVERIFY(temporaryDir.isValid());

    RecordingManager manager(temporaryDir.path());
    const RecordingResult startResult = manager.startRecording(QStringLiteral("source 1"),
                                                               QSize(160, 120),
                                                               10.0);

#ifndef VIDEOANALYSIS_HAS_OPENCV
    QVERIFY(!startResult.success);
    QVERIFY(!startResult.errorMessage.isEmpty());
#else
    QVERIFY2(startResult.success, qPrintable(startResult.errorMessage));
    QVERIFY(manager.isRecording());

    for (int i = 0; i < 5; ++i) {
        QImage image(160, 120, QImage::Format_RGB888);
        image.fill(QColor::fromRgb(20 + i * 20, 80, 160));
        const RecordingResult writeResult = manager.writeFrame(image);
        QVERIFY2(writeResult.success, qPrintable(writeResult.errorMessage));
    }

    const RecordingResult stopResult = manager.stopRecording();
    QVERIFY2(stopResult.success, qPrintable(stopResult.errorMessage));
    QVERIFY(!manager.isRecording());
    QVERIFY(QFileInfo::exists(stopResult.filePath));
    QVERIFY(stopResult.filePath.contains(QDate::currentDate().toString(QStringLiteral("yyyy-MM-dd"))));
    QVERIFY(stopResult.filePath.contains(QStringLiteral("source_1")));
    QVERIFY(stopResult.filePath.contains(QStringLiteral("manual")));
    QVERIFY(stopResult.filePath.endsWith(QStringLiteral(".avi")));
    QVERIFY(QFileInfo(stopResult.filePath).size() > 0);

    cv::VideoCapture capture(stopResult.filePath.toLocal8Bit().constData());
    QVERIFY(capture.isOpened());
    cv::Mat frame;
    const bool frameRead = capture.read(frame);
    QVERIFY2(frameRead,
             qPrintable(QStringLiteral("Could not read first recorded frame. fileSize=%1 frameCount=%2")
                         .arg(QFileInfo(stopResult.filePath).size())
                         .arg(capture.get(cv::CAP_PROP_FRAME_COUNT))));
    QVERIFY(!frame.empty());
#endif
}

void RecordingManagerTests::startRecordingRejectsInvalidFrameSize()
{
    QTemporaryDir temporaryDir;
    QVERIFY(temporaryDir.isValid());

    RecordingManager manager(temporaryDir.path());
    const RecordingResult result = manager.startRecording(QStringLiteral("source1"), QSize(), 25.0);

    QVERIFY(!result.success);
    QVERIFY(!result.errorMessage.isEmpty());
}

QTEST_MAIN(RecordingManagerTests)

#include "RecordingManagerTests.moc"
