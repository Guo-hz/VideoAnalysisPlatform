#include <QtTest>

#include "core/video/FFmpegVideoDecoder.h"
#include "core/video/VideoSource.h"

class FFmpegVideoDecoderTests : public QObject
{
    Q_OBJECT

private slots:
    void rtspTypeStringRoundTrips();
    void emptyInputFailsGracefully();
};

void FFmpegVideoDecoderTests::rtspTypeStringRoundTrips()
{
    QCOMPARE(videoSourceTypeToString(VideoSourceType::Rtsp), QStringLiteral("RTSP"));
    QCOMPARE(videoSourceTypeFromString(QStringLiteral("rtsp")), VideoSourceType::Rtsp);
}

void FFmpegVideoDecoderTests::emptyInputFailsGracefully()
{
    VideoSource source;
    source.type = VideoSourceType::Rtsp;
    source.url = QString();

    FFmpegVideoDecoder decoder;
    QVERIFY(!decoder.open(source));
    QVERIFY(!decoder.lastError().isEmpty());
}

QTEST_MAIN(FFmpegVideoDecoderTests)

#include "FFmpegVideoDecoderTests.moc"
