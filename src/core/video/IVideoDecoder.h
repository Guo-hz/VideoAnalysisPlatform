#pragma once

#include <QImage>
#include <QString>

#include "core/video/VideoSource.h"

class IVideoDecoder
{
public:
    virtual ~IVideoDecoder() = default;

    virtual bool open(const VideoSource &source) = 0;
    virtual bool readFrame(QImage &frame) = 0;
    virtual void close() = 0;
    virtual double frameRate() const = 0;
    virtual bool isEndOfStream() const = 0;
    virtual QString lastError() const = 0;
};
