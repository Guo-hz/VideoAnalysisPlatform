#include "FFmpegVideoDecoder.h"

#include <algorithm>
#include <cmath>

#include <QUrl>

#ifdef VIDEOANALYSIS_HAS_FFMPEG
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/error.h>
#include <libswscale/swscale.h>
}
#endif

struct FFmpegVideoDecoder::Private {
#ifdef VIDEOANALYSIS_HAS_FFMPEG
    AVFormatContext *formatContext = nullptr;
    AVCodecContext *codecContext = nullptr;
    AVFrame *decodedFrame = nullptr;
    AVPacket *packet = nullptr;
    SwsContext *swsContext = nullptr;
    AVPixelFormat pixelFormat = AV_PIX_FMT_NONE;
    int videoStreamIndex = -1;
    bool networkInitialized = false;
#endif
    bool flushSent = false;
    const std::atomic_bool *interruptFlag = nullptr;
    double fps = 0.0;
    int width = 0;
    int height = 0;
    bool endOfStream = false;
    QString lastError;
};

namespace {
#ifdef VIDEOANALYSIS_HAS_FFMPEG
QString ffmpegErrorString(int errorCode)
{
    char buffer[AV_ERROR_MAX_STRING_SIZE] = {};
    av_strerror(errorCode, buffer, sizeof(buffer));
    return QString::fromLocal8Bit(buffer);
}

double rationalToDouble(AVRational rational)
{
    if (rational.num <= 0 || rational.den <= 0) {
        return 0.0;
    }

    const double value = av_q2d(rational);
    return std::isfinite(value) ? value : 0.0;
}

int ffmpegInterruptCallback(void *opaque)
{
    const auto *interruptFlag = static_cast<const std::atomic_bool *>(opaque);
    return interruptFlag && interruptFlag->load() ? 1 : 0;
}
#endif

QString inputFromSource(const VideoSource &source)
{
    QString input = source.url.trimmed();
    if (source.type == VideoSourceType::LocalFile) {
        const QUrl url(input);
        if (url.isLocalFile()) {
            input = url.toLocalFile();
        }
    }
    return input;
}
}

FFmpegVideoDecoder::FFmpegVideoDecoder()
    : d(std::make_unique<Private>())
{
}

FFmpegVideoDecoder::~FFmpegVideoDecoder()
{
    close();
}

bool FFmpegVideoDecoder::open(const VideoSource &source)
{
    close();
    d->lastError.clear();
    d->endOfStream = false;
    d->flushSent = false;
    d->fps = 0.0;
    d->width = 0;
    d->height = 0;

    if (source.type != VideoSourceType::LocalFile && source.type != VideoSourceType::Rtsp
        && source.type != VideoSourceType::Http) {
        d->lastError = QStringLiteral("FFmpeg 解码器当前只支持本地视频、RTSP 和 HTTP。");
        return false;
    }

    const QString input = inputFromSource(source);
    if (input.isEmpty()) {
        d->lastError = QStringLiteral("视频输入地址为空。");
        return false;
    }

#ifndef VIDEOANALYSIS_HAS_FFMPEG
    d->lastError = QStringLiteral("FFmpeg 未启用：请安装 FFmpeg 开发包并在 CMake 中设置 FFMPEG_ROOT。");
    return false;
#else
    avformat_network_init();
    d->networkInitialized = true;

    QByteArray inputBytes = input.toUtf8();
    AVDictionary *options = nullptr;
    if (source.type == VideoSourceType::Rtsp) {
        av_dict_set(&options, "rtsp_transport", "tcp", 0);
    }
    const QString timeout = QString::number(
        static_cast<qint64>(std::max(1000, source.readTimeoutMs)) * 1000);
    av_dict_set(&options, "stimeout", timeout.toUtf8().constData(), 0);
    av_dict_set(&options, "timeout", timeout.toUtf8().constData(), 0);
    av_dict_set(&options, "rw_timeout", timeout.toUtf8().constData(), 0);

    d->formatContext = avformat_alloc_context();
    if (!d->formatContext) {
        av_dict_free(&options);
        d->lastError = QStringLiteral("创建 FFmpeg 输入上下文失败。");
        close();
        return false;
    }
    d->formatContext->interrupt_callback.callback = ffmpegInterruptCallback;
    d->formatContext->interrupt_callback.opaque = const_cast<std::atomic_bool *>(d->interruptFlag);

    int result = avformat_open_input(&d->formatContext, inputBytes.constData(), nullptr, &options);
    av_dict_free(&options);
    if (result < 0) {
        d->lastError = QStringLiteral("打开视频流失败：%1，%2").arg(input, ffmpegErrorString(result));
        close();
        return false;
    }

    result = avformat_find_stream_info(d->formatContext, nullptr);
    if (result < 0) {
        d->lastError = QStringLiteral("读取视频流信息失败：%1").arg(ffmpegErrorString(result));
        close();
        return false;
    }

#if LIBAVCODEC_VERSION_MAJOR < 59
    AVCodec *decoder = nullptr;
#else
    const AVCodec *decoder = nullptr;
#endif
    result = av_find_best_stream(d->formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, &decoder, 0);
    if (result < 0 || !decoder) {
        d->lastError = QStringLiteral("未找到可解码的视频流：%1").arg(ffmpegErrorString(result));
        close();
        return false;
    }

    d->videoStreamIndex = result;
    AVStream *videoStream = d->formatContext->streams[d->videoStreamIndex];
    d->codecContext = avcodec_alloc_context3(decoder);
    if (!d->codecContext) {
        d->lastError = QStringLiteral("创建 FFmpeg 解码上下文失败。");
        close();
        return false;
    }

    result = avcodec_parameters_to_context(d->codecContext, videoStream->codecpar);
    if (result < 0) {
        d->lastError = QStringLiteral("复制视频流参数失败：%1").arg(ffmpegErrorString(result));
        close();
        return false;
    }

    result = avcodec_open2(d->codecContext, decoder, nullptr);
    if (result < 0) {
        d->lastError = QStringLiteral("打开 FFmpeg 视频解码器失败：%1").arg(ffmpegErrorString(result));
        close();
        return false;
    }

    d->decodedFrame = av_frame_alloc();
    d->packet = av_packet_alloc();
    if (!d->decodedFrame || !d->packet) {
        d->lastError = QStringLiteral("分配 FFmpeg 帧或包失败。");
        close();
        return false;
    }

    d->width = d->codecContext->width;
    d->height = d->codecContext->height;
    d->pixelFormat = d->codecContext->pix_fmt;
    d->fps = rationalToDouble(videoStream->avg_frame_rate);
    if (d->fps <= 0.0) {
        d->fps = rationalToDouble(videoStream->r_frame_rate);
    }
    if (d->fps <= 0.0) {
        d->fps = 30.0;
    }

    return true;
#endif
}

bool FFmpegVideoDecoder::readFrame(QImage &frame)
{
#ifndef VIDEOANALYSIS_HAS_FFMPEG
    Q_UNUSED(frame)
    d->lastError = QStringLiteral("FFmpeg 未启用，无法读取视频帧。");
    return false;
#else
    if (!d->formatContext || !d->codecContext || !d->decodedFrame || !d->packet) {
        d->lastError = QStringLiteral("视频尚未打开。");
        return false;
    }

    const auto convertFrame = [this, &frame]() -> bool {
            QImage convertedFrame(d->decodedFrame->width,
                                   d->decodedFrame->height,
                                   QImage::Format_RGBA8888);
            if (convertedFrame.isNull()) {
                d->lastError = QStringLiteral("创建视频帧图像失败。");
                return false;
            }

            d->swsContext = sws_getCachedContext(d->swsContext,
                                                 d->decodedFrame->width,
                                                 d->decodedFrame->height,
                                                 static_cast<AVPixelFormat>(d->decodedFrame->format),
                                                 d->decodedFrame->width,
                                                 d->decodedFrame->height,
                                                 AV_PIX_FMT_RGBA,
                                                 SWS_BILINEAR,
                                                 nullptr,
                                                 nullptr,
                                                 nullptr);
            if (!d->swsContext) {
                d->lastError = QStringLiteral("创建 FFmpeg 像素格式转换上下文失败。");
                return false;
            }

            uint8_t *destinationData[4] = {convertedFrame.bits(), nullptr, nullptr, nullptr};
            const int destinationLineSize[4] = {convertedFrame.bytesPerLine(), 0, 0, 0};
            const int convertedHeight = sws_scale(d->swsContext,
                                                  d->decodedFrame->data,
                                                  d->decodedFrame->linesize,
                                                  0,
                                                  d->decodedFrame->height,
                                                  destinationData,
                                                  destinationLineSize);
            if (convertedHeight <= 0) {
                d->lastError = QStringLiteral("转换 FFmpeg 视频帧失败。");
                return false;
            }
            frame = convertedFrame;
            d->lastError.clear();
            return true;
        };

    while (true) {
        const int receiveResult = avcodec_receive_frame(d->codecContext, d->decodedFrame);
        if (receiveResult == 0) {
            const bool converted = convertFrame();
            av_frame_unref(d->decodedFrame);
            return converted;
        }
        if (receiveResult == AVERROR_EOF) {
            d->endOfStream = true;
            d->lastError.clear();
            return false;
        }
        if (receiveResult != AVERROR(EAGAIN)) {
            d->lastError = QStringLiteral("解码视频帧失败：%1").arg(ffmpegErrorString(receiveResult));
            return false;
        }

        if (d->flushSent) {
            d->endOfStream = true;
            d->lastError.clear();
            return false;
        }

        av_packet_unref(d->packet);
        const int readResult = av_read_frame(d->formatContext, d->packet);
        if (readResult < 0) {
            if (readResult == AVERROR_EOF) {
                const int flushResult = avcodec_send_packet(d->codecContext, nullptr);
                d->flushSent = true;
                if (flushResult < 0 && flushResult != AVERROR_EOF && flushResult != AVERROR(EAGAIN)) {
                    d->lastError = QStringLiteral("刷新 FFmpeg 解码器失败：%1")
                                       .arg(ffmpegErrorString(flushResult));
                    return false;
                }
                continue;
            }
            d->endOfStream = false;
            d->lastError = QStringLiteral("读取视频流失败：%1").arg(ffmpegErrorString(readResult));
            return false;
        }

        if (d->packet->stream_index != d->videoStreamIndex) {
            av_packet_unref(d->packet);
            continue;
        }

        int sendResult = avcodec_send_packet(d->codecContext, d->packet);
        if (sendResult == AVERROR(EAGAIN)) {
            // Drain frames before retrying the packet; avcodec_send_packet did not consume it.
            while (true) {
                const int drainedResult = avcodec_receive_frame(d->codecContext, d->decodedFrame);
                if (drainedResult == AVERROR(EAGAIN)) {
                    break;
                }
                if (drainedResult == AVERROR_EOF) {
                    d->endOfStream = true;
                    d->lastError.clear();
                    av_packet_unref(d->packet);
                    return false;
                }
                if (drainedResult < 0) {
                    d->lastError = QStringLiteral("解码视频帧失败：%1")
                                       .arg(ffmpegErrorString(drainedResult));
                    av_packet_unref(d->packet);
                    return false;
                }
                const bool converted = convertFrame();
                av_frame_unref(d->decodedFrame);
                if (converted) {
                    av_packet_unref(d->packet);
                    return true;
                }
                av_packet_unref(d->packet);
                return false;
            }
            sendResult = avcodec_send_packet(d->codecContext, d->packet);
        }
        av_packet_unref(d->packet);
        if (sendResult < 0) {
            d->lastError = QStringLiteral("发送视频包到解码器失败：%1").arg(ffmpegErrorString(sendResult));
            return false;
        }
        while (true) {
            const int receiveResult = avcodec_receive_frame(d->codecContext, d->decodedFrame);
            if (receiveResult == AVERROR(EAGAIN)) {
                break;
            }
            if (receiveResult == AVERROR_EOF) {
                d->endOfStream = true;
                d->lastError.clear();
                return false;
            }
            if (receiveResult < 0) {
                d->lastError = QStringLiteral("解码视频帧失败：%1").arg(ffmpegErrorString(receiveResult));
                return false;
            }

            const bool converted = convertFrame();
            av_frame_unref(d->decodedFrame);
            return converted;
        }
    }
#endif
}

void FFmpegVideoDecoder::close()
{
#ifdef VIDEOANALYSIS_HAS_FFMPEG
    if (d->swsContext) {
        sws_freeContext(d->swsContext);
        d->swsContext = nullptr;
    }
    if (d->packet) {
        av_packet_free(&d->packet);
    }
    if (d->decodedFrame) {
        av_frame_free(&d->decodedFrame);
    }
    if (d->codecContext) {
        avcodec_free_context(&d->codecContext);
    }
    if (d->formatContext) {
        avformat_close_input(&d->formatContext);
    }
    d->videoStreamIndex = -1;
    d->pixelFormat = AV_PIX_FMT_NONE;
    if (d->networkInitialized) {
        avformat_network_deinit();
        d->networkInitialized = false;
    }
#endif
    d->fps = 0.0;
    d->width = 0;
    d->height = 0;
    d->endOfStream = false;
    d->flushSent = false;
}

double FFmpegVideoDecoder::frameRate() const
{
    return d->fps;
}

bool FFmpegVideoDecoder::isEndOfStream() const
{
    return d->endOfStream;
}

QString FFmpegVideoDecoder::lastError() const
{
    return d->lastError;
}

void FFmpegVideoDecoder::setInterruptFlag(const std::atomic_bool *interruptFlag)
{
    d->interruptFlag = interruptFlag;
}

int FFmpegVideoDecoder::width() const
{
    return d->width;
}

int FFmpegVideoDecoder::height() const
{
    return d->height;
}
