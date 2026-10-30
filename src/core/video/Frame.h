#pragma once

#include <QDateTime>
#include <QImage>

struct Frame {
    QImage image;
    qint64 timestamp = 0;
    int width = 0;
    int height = 0;
    qint64 frameIndex = 0;

    static Frame fromImage(const QImage &image, qint64 frameIndex)
    {
        return Frame{
            image,
            QDateTime::currentMSecsSinceEpoch(),
            image.width(),
            image.height(),
            frameIndex,
        };
    }
};
