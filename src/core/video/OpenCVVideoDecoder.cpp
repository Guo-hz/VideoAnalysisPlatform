#include "OpenCVVideoDecoder.h"

#include <cmath>

#include <QUrl>

#ifdef VIDEOANALYSIS_HAS_OPENCV
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#endif

struct OpenCVVideoDecoder::Private {
#ifdef VIDEOANALYSIS_HAS_OPENCV
    cv::VideoCapture capture;
#endif
    int cameraIndex = -1;
    double fps = 0.0;
    bool endOfStream = false;
    QString lastError;
};

OpenCVVideoDecoder::OpenCVVideoDecoder()
    : d(std::make_unique<Private>())
{
}

OpenCVVideoDecoder::~OpenCVVideoDecoder()
{
    close();
}

bool OpenCVVideoDecoder::open(const VideoSource &source)
{
    close();
    d->lastError.clear();
    d->endOfStream = false;
    d->fps = 0.0;

#ifndef VIDEOANALYSIS_HAS_OPENCV
    Q_UNUSED(source)
    d->lastError = QStringLiteral("OpenCV 未启用：请安装 OpenCV 并在 CMake 中设置 OpenCV_DIR。");
    return false;
#else
    if (source.type != VideoSourceType::LocalFile && source.type != VideoSourceType::Camera) {
        d->lastError = QStringLiteral("OpenCV 解码器只支持本地文件和 Camera。");
        return false;
    }

    QString filePath = source.url;
    const QUrl url(source.url);
    if (url.isLocalFile()) {
        filePath = url.toLocalFile();
    }

    if (filePath.trimmed().isEmpty()) {
        d->lastError = QStringLiteral("视频文件路径为空。");
        return false;
    }

    if (source.type == VideoSourceType::Camera) {
        QString cameraValue = filePath.trimmed();
        if (cameraValue.startsWith(QStringLiteral("camera://"), Qt::CaseInsensitive)) {
            cameraValue = cameraValue.mid(QStringLiteral("camera://").size());
        }
        bool ok = false;
        const int index = cameraValue.toInt(&ok);
        if (!ok || index < 0) {
            d->lastError = QStringLiteral("Camera 源需要非负设备索引，例如 0。");
            return false;
        }
        d->cameraIndex = index;
        if (!d->capture.open(index, cv::CAP_ANY)) {
            d->lastError = QStringLiteral("打开 Camera 失败：%1").arg(index);
            return false;
        }
    } else if (!d->capture.open(filePath.toStdString())) {
        d->lastError = QStringLiteral("打开本地视频失败：%1").arg(filePath);
        return false;
    }

    d->fps = d->capture.get(cv::CAP_PROP_FPS);
    if (d->fps <= 0.0 || !std::isfinite(d->fps)) {
        d->fps = 30.0;
    }

    return true;
#endif
}

bool OpenCVVideoDecoder::readFrame(QImage &frame)
{
#ifndef VIDEOANALYSIS_HAS_OPENCV
    Q_UNUSED(frame)
    d->lastError = QStringLiteral("OpenCV 未启用，无法读取视频帧。");
    return false;
#else
    if (!d->capture.isOpened()) {
        d->lastError = QStringLiteral("视频尚未打开。");
        return false;
    }

    cv::Mat mat;
    if (!d->capture.read(mat)) {
        d->endOfStream = true;
        d->lastError.clear();
        return false;
    }

    if (mat.empty()) {
        d->lastError = QStringLiteral("读取到空视频帧。");
        return false;
    }

    cv::Mat converted;
    if (mat.channels() == 1) {
        cv::cvtColor(mat, converted, cv::COLOR_GRAY2RGB);
        frame = QImage(converted.data,
                       converted.cols,
                       converted.rows,
                       static_cast<int>(converted.step),
                       QImage::Format_RGB888).copy();
        return true;
    }

    if (mat.channels() == 3) {
        cv::cvtColor(mat, converted, cv::COLOR_BGR2RGB);
        frame = QImage(converted.data,
                       converted.cols,
                       converted.rows,
                       static_cast<int>(converted.step),
                       QImage::Format_RGB888).copy();
        return true;
    }

    if (mat.channels() == 4) {
        cv::cvtColor(mat, converted, cv::COLOR_BGRA2RGBA);
        frame = QImage(converted.data,
                       converted.cols,
                       converted.rows,
                       static_cast<int>(converted.step),
                       QImage::Format_RGBA8888).copy();
        return true;
    }

    d->lastError = QStringLiteral("不支持的视频帧格式：%1 通道。").arg(mat.channels());
    return false;
#endif
}

void OpenCVVideoDecoder::close()
{
#ifdef VIDEOANALYSIS_HAS_OPENCV
    if (d->capture.isOpened()) {
        d->capture.release();
    }
#endif
    d->cameraIndex = -1;
    d->endOfStream = false;
}

double OpenCVVideoDecoder::frameRate() const
{
    return d->fps;
}

bool OpenCVVideoDecoder::isEndOfStream() const
{
    return d->endOfStream;
}

QString OpenCVVideoDecoder::lastError() const
{
    return d->lastError;
}
