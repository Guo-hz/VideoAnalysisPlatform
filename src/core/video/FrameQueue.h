#pragma once

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <mutex>

#include "core/video/Frame.h"

class FrameQueue
{
public:
    explicit FrameQueue(std::size_t maxSize = 3);

    void push(Frame frame);
    bool pop(Frame &frame);
    bool tryPop(Frame &frame);
    void clear();
    void close();
    void reopen();

    std::size_t size() const;
    std::size_t maxSize() const;
    std::size_t droppedFrames() const;
    bool isClosed() const;

private:
    mutable std::mutex m_mutex;
    std::condition_variable m_notEmpty;
    std::deque<Frame> m_frames;
    std::size_t m_maxSize = 3;
    std::size_t m_droppedFrames = 0;
    bool m_closed = false;
};
