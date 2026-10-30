#include "RecordingManager.h"

#include <cmath>

#include <QDate>
#include <QDir>
#include <QFileInfo>
#include <QMutexLocker>
#include <QTime>
#include <QUuid>

#ifdef VIDEOANALYSIS_HAS_OPENCV
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>

class RecordingManager::WriterHolder
{
public:
    cv::VideoWriter writer;
};

namespace {
cv::Mat imageToBgrMat(const QImage &image, const QSize &frameSize)
{
    QImage normalizedImage = image;
    if (normalizedImage.size() != frameSize) {
        normalizedImage = normalizedImage.scaled(frameSize,
                                                 Qt::IgnoreAspectRatio,
                                                 Qt::SmoothTransformation);
    }

    if (normalizedImage.format() != QImage::Format_RGB888) {
        normalizedImage = normalizedImage.convertToFormat(QImage::Format_RGB888);
    }

    cv::Mat rgbMat(normalizedImage.height(),
                   normalizedImage.width(),
                   CV_8UC3,
                   const_cast<uchar *>(normalizedImage.constBits()),
                   static_cast<size_t>(normalizedImage.bytesPerLine()));
    cv::Mat bgrMat;
    cv::cvtColor(rgbMat, bgrMat, cv::COLOR_RGB2BGR);
    return bgrMat.clone();
}
}
#endif

RecordingManager::RecordingManager(QString baseDirectory)
    : m_baseDirectory(QDir::cleanPath(baseDirectory))
{
#ifdef VIDEOANALYSIS_HAS_OPENCV
    m_writerHolder = new WriterHolder;
#endif
}

RecordingManager::~RecordingManager()
{
    stopRecording();
#ifdef VIDEOANALYSIS_HAS_OPENCV
    delete m_writerHolder;
    m_writerHolder = nullptr;
#endif
}

RecordingResult RecordingManager::startRecording(const QString &sourceId, const QSize &frameSize, double fps)
{
    QMutexLocker locker(&m_mutex);

    RecordingResult result;
    if (!frameSize.isValid() || frameSize.isEmpty()) {
        result.errorMessage = QStringLiteral("Recording failed: invalid frame size.");
        m_lastError = result.errorMessage;
        return result;
    }

    if (m_fps > 0.0) {
        result.errorMessage = QStringLiteral("Recording is already running.");
        result.filePath = m_currentFilePath;
        m_lastError = result.errorMessage;
        return result;
    }

#ifndef VIDEOANALYSIS_HAS_OPENCV
    Q_UNUSED(sourceId)
    Q_UNUSED(fps)
    result.errorMessage = QStringLiteral("Recording failed: OpenCV is not enabled.");
    m_lastError = result.errorMessage;
    return result;
#else
    const QString directoryPath = dayDirectoryPath();
    QDir directory(directoryPath);
    if (!directory.exists() && !QDir().mkpath(directoryPath)) {
        result.errorMessage = QStringLiteral("Recording failed: could not create directory %1.").arg(directoryPath);
        m_lastError = result.errorMessage;
        return result;
    }

    const double normalizedFps = (fps > 0.0 && std::isfinite(fps)) ? fps : 25.0;
    const QString filePath = QFileInfo(directory.absoluteFilePath(makeFileName(sourceId))).absoluteFilePath();
    const int fourcc = cv::VideoWriter::fourcc('M', 'J', 'P', 'G');
    const cv::Size size(frameSize.width(), frameSize.height());

    if (!m_writerHolder->writer.open(filePath.toLocal8Bit().constData(), fourcc, normalizedFps, size, true)) {
        result.errorMessage = QStringLiteral("Recording failed: could not open video writer.");
        m_lastError = result.errorMessage;
        return result;
    }

    m_currentFilePath = filePath;
    m_frameSize = frameSize;
    m_fps = normalizedFps;
    m_lastError.clear();

    result.success = true;
    result.filePath = m_currentFilePath;
    return result;
#endif
}

RecordingResult RecordingManager::writeFrame(const QImage &image)
{
    QMutexLocker locker(&m_mutex);

    RecordingResult result;
    result.filePath = m_currentFilePath;

    if (m_fps <= 0.0) {
        // A concurrent stop is a normal outcome for a frame already in flight.
        result.success = true;
        m_lastError.clear();
        return result;
    }

    if (image.isNull()) {
        result.errorMessage = QStringLiteral("Recording failed: frame is empty.");
        m_lastError = result.errorMessage;
        return result;
    }

#ifndef VIDEOANALYSIS_HAS_OPENCV
    result.errorMessage = QStringLiteral("Recording failed: OpenCV is not enabled.");
    m_lastError = result.errorMessage;
    return result;
#else
    cv::Mat frame = imageToBgrMat(image, m_frameSize);
    if (frame.empty()) {
        result.errorMessage = QStringLiteral("Recording failed: converted frame is empty.");
        m_lastError = result.errorMessage;
        return result;
    }

    m_writerHolder->writer.write(frame);
    m_lastError.clear();
    result.success = true;
    result.filePath = m_currentFilePath;
    return result;
#endif
}

RecordingResult RecordingManager::stopRecording()
{
    QMutexLocker locker(&m_mutex);

    RecordingResult result;
    result.filePath = m_currentFilePath;

    if (m_fps <= 0.0) {
        result.success = true;
        return result;
    }

#ifdef VIDEOANALYSIS_HAS_OPENCV
    if (m_writerHolder && m_writerHolder->writer.isOpened()) {
        m_writerHolder->writer.release();
    }
#endif

    m_frameSize = QSize();
    m_fps = 0.0;
    m_lastError.clear();
    result.success = true;
    return result;
}

bool RecordingManager::isRecording() const
{
    QMutexLocker locker(&m_mutex);
    return m_fps > 0.0;
}

QString RecordingManager::currentFilePath() const
{
    QMutexLocker locker(&m_mutex);
    return m_currentFilePath;
}

QString RecordingManager::lastError() const
{
    QMutexLocker locker(&m_mutex);
    return m_lastError;
}

QString RecordingManager::dayDirectoryPath() const
{
    const QString datePart = QDate::currentDate().toString(QStringLiteral("yyyy-MM-dd"));
    return QDir(m_baseDirectory).filePath(datePart);
}

QString RecordingManager::makeFileName(const QString &sourceId) const
{
    const QString safeSourceId = sanitizedToken(sourceId, QStringLiteral("source"));
    const QString timePart = QTime::currentTime().toString(QStringLiteral("HHmmss_zzz"));
    const QString uniquePart = QUuid::createUuid().toString(QUuid::WithoutBraces);
    return QStringLiteral("%1_%2_%3_manual.avi").arg(safeSourceId, timePart, uniquePart);
}

QString RecordingManager::sanitizedToken(const QString &value, const QString &fallback) const
{
    QString token = value.trimmed();
    if (token.isEmpty()) {
        token = fallback;
    }

    for (QChar &character : token) {
        if (!character.isLetterOrNumber() && character != QLatin1Char('_') && character != QLatin1Char('-')) {
            character = QLatin1Char('_');
        }
    }

    return token;
}
