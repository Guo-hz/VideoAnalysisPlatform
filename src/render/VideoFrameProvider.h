#pragma once

#include <QImage>
#include <QHash>
#include <QMutex>
#include <QQuickImageProvider>
#include <QString>

class VideoFrameProvider : public QQuickImageProvider
{
public:
    VideoFrameProvider();

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;
    void setFrame(const QString &streamId, const QImage &frame);
    QImage currentFrame(const QString &streamId) const;
    void clearFrame(const QString &streamId);

private:
    QString frameIdFromRequest(const QString &id) const;
    QImage scaledFrame(const QImage &frame, const QSize &requestedSize) const;

    mutable QMutex m_mutex;
    QHash<QString, QImage> m_frames;
};
