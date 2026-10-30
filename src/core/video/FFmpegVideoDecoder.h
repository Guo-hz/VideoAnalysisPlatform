#pragma once

#include <atomic>
#include <memory>

#include "core/video/IVideoDecoder.h"

class FFmpegVideoDecoder : public IVideoDecoder
{
public:
    FFmpegVideoDecoder();
    ~FFmpegVideoDecoder() override;

    bool open(const VideoSource &source) override;
    bool readFrame(QImage &frame) override;
    void close() override;
    double frameRate() const override;
    bool isEndOfStream() const override;
    QString lastError() const override;

    void setInterruptFlag(const std::atomic_bool *interruptFlag);
    int width() const;
    int height() const;

private:
    struct Private;
    std::unique_ptr<Private> d;
};
