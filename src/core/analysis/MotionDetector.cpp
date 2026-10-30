#include "MotionDetector.h"

#include <algorithm>

#include <QDateTime>

#ifdef VIDEOANALYSIS_HAS_OPENCV
#include <opencv2/imgproc.hpp>
#include <opencv2/video/background_segm.hpp>
#endif

struct MotionDetector::Private {
#ifdef VIDEOANALYSIS_HAS_OPENCV
    cv::Ptr<cv::BackgroundSubtractorMOG2> backgroundModel;
    int warmupRemaining = 30;
    int consecutiveFrames = 0;
#endif
};

MotionDetector::MotionDetector()
    : d(std::make_unique<Private>())
{
}

MotionDetector::~MotionDetector() = default;

DetectionResult MotionDetector::analyze(const QImage &frame, const QString &sourceId, int minTargetArea)
{
    DetectionResult result;
    result.sourceId = sourceId;
    result.timestamp = QDateTime::currentMSecsSinceEpoch();

    if (frame.isNull()) {
        reset();
        return result;
    }

#ifndef VIDEOANALYSIS_HAS_OPENCV
    Q_UNUSED(minTargetArea)
    return result;
#else
    if (!d->backgroundModel) {
        d->backgroundModel = cv::createBackgroundSubtractorMOG2(500, 16.0, true);
        d->warmupRemaining = 30;
        d->consecutiveFrames = 0;
    }
    const QImage rgbFrame = frame.convertToFormat(QImage::Format_RGB888);
    cv::Mat rgbMat(rgbFrame.height(),
                   rgbFrame.width(),
                   CV_8UC3,
                   const_cast<uchar *>(rgbFrame.constBits()),
                   static_cast<size_t>(rgbFrame.bytesPerLine()));

    cv::Mat currentGray;
    cv::cvtColor(rgbMat, currentGray, cv::COLOR_RGB2GRAY);

    cv::Mat foreground;
    d->backgroundModel->apply(currentGray, foreground, -1.0);
    // MOG2 uses 127 for shadows. Keep only confident foreground pixels.
    cv::threshold(foreground, foreground, 200, 255, cv::THRESH_BINARY);

    const cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::morphologyEx(foreground, foreground, cv::MORPH_OPEN, kernel);
    cv::dilate(foreground, foreground, kernel, cv::Point(-1, -1), 2);

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(foreground, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    const double minArea = static_cast<double>(std::max(1, minTargetArea));
    for (const std::vector<cv::Point> &contour : contours) {
        if (cv::contourArea(contour) < minArea) {
            continue;
        }

        const cv::Rect boundingBox = cv::boundingRect(contour);
        result.boxes.append(QRect(boundingBox.x, boundingBox.y, boundingBox.width, boundingBox.height));
    }
    if (d->warmupRemaining > 0) {
        --d->warmupRemaining;
        d->consecutiveFrames = 0;
        result.boxes.clear();
        return result;
    }
    if (result.boxes.isEmpty()) {
        d->consecutiveFrames = 0;
        return result;
    }
    ++d->consecutiveFrames;
    if (d->consecutiveFrames < 3) {
        result.boxes.clear();
    }
    return result;
#endif
}

void MotionDetector::reset()
{
#ifdef VIDEOANALYSIS_HAS_OPENCV
    d->backgroundModel.release();
    d->warmupRemaining = 30;
    d->consecutiveFrames = 0;
#endif
}
