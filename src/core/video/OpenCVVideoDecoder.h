#pragma once

#include <memory>

#include "core/video/IVideoDecoder.h"

class OpenCVVideoDecoder : public IVideoDecoder
{
public:
    OpenCVVideoDecoder();
    ~OpenCVVideoDecoder() override;

    bool open(const VideoSource &source) override;
    bool readFrame(QImage &frame) override;
    void close() override;
    double frameRate() const override;
    bool isEndOfStream() const override;
    QString lastError() const override;

private:
    struct Private;
    std::unique_ptr<Private> d;
};
