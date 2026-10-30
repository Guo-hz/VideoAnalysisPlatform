#include "FrameQueue.h"

#include <algorithm>
#include <utility>

FrameQueue::FrameQueue(std::size_t maxSize)
    : m_maxSize(std::max<std::size_t>(1, maxSize))
{
}

void FrameQueue::push(Frame frame)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_closed) {
        return;
    }

    while (m_frames.size() >= m_maxSize) {
        m_frames.pop_front();
        ++m_droppedFrames;
    }

    m_frames.push_back(std::move(frame));
    m_notEmpty.notify_one();
}

bool FrameQueue::pop(Frame &frame)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_notEmpty.wait(lock, [this]() {
        return m_closed || !m_frames.empty();
    });

    if (m_frames.empty()) {
        return false;
    }

    frame = std::move(m_frames.front());
    m_frames.pop_front();
    return true;
}

bool FrameQueue::tryPop(Frame &frame)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_frames.empty()) {
        return false;
    }

    frame = std::move(m_frames.front());
    m_frames.pop_front();
    return true;
}

void FrameQueue::clear()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_frames.clear();
    m_notEmpty.notify_all();
}

void FrameQueue::close()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_closed = true;
    m_notEmpty.notify_all();
}

void FrameQueue::reopen()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_closed = false;
}

std::size_t FrameQueue::size() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_frames.size();
}

std::size_t FrameQueue::maxSize() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_maxSize;
}

std::size_t FrameQueue::droppedFrames() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_droppedFrames;
}

bool FrameQueue::isClosed() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_closed;
}
