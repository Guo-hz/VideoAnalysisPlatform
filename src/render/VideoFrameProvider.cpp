#include "VideoFrameProvider.h"

#include <QMutexLocker>

VideoFrameProvider::VideoFrameProvider()
    : QQuickImageProvider(QQuickImageProvider::Image)
{
}

QImage VideoFrameProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    const QString frameId = frameIdFromRequest(id);
    QMutexLocker locker(&m_mutex);
    const QImage frame = m_frames.value(frameId);
    locker.unlock();

    if (size) {
        *size = frame.size();
    }

    return scaledFrame(frame, requestedSize);
}

void VideoFrameProvider::setFrame(const QString &streamId, const QImage &frame)
{
    if (streamId.isEmpty()) {
        return;
    }

    QMutexLocker locker(&m_mutex);
    m_frames.insert(streamId, frame.copy());
}

QImage VideoFrameProvider::currentFrame(const QString &streamId) const
{
    QMutexLocker locker(&m_mutex);
    return m_frames.value(streamId).copy();
}

void VideoFrameProvider::clearFrame(const QString &streamId)
{
    QMutexLocker locker(&m_mutex);
    m_frames.remove(streamId);
}

QString VideoFrameProvider::frameIdFromRequest(const QString &id) const
{
    const int queryIndex = id.indexOf(QLatin1Char('?'));
    return queryIndex >= 0 ? id.left(queryIndex) : id;
}

QImage VideoFrameProvider::scaledFrame(const QImage &frame, const QSize &requestedSize) const
{
    if (frame.isNull()) {
        return {};
    }

    if (!requestedSize.isValid() || requestedSize.isEmpty()) {
        return frame;
    }

    return frame.scaled(requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}
