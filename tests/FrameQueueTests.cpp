#include <atomic>
#include <thread>

#include <QImage>
#include <QtTest/QtTest>

#include "core/video/FrameQueue.h"

class FrameQueueTests : public QObject
{
    Q_OBJECT

private slots:
    void pushAndPop();
    void tryPopReturnsFalseWhenEmpty();
    void dropsOldestFrameWhenFull();
    void clearRemovesQueuedFrames();
    void popUnblocksWhenClosed();
    void supportsBasicProducerConsumer();
};

namespace {
Frame makeFrame(qint64 frameIndex)
{
    QImage image(2, 2, QImage::Format_RGB32);
    image.fill(Qt::black);
    return Frame::fromImage(image, frameIndex);
}
}

void FrameQueueTests::pushAndPop()
{
    FrameQueue queue(2);
    queue.push(makeFrame(1));

    Frame frame;
    QVERIFY(queue.pop(frame));
    QCOMPARE(frame.frameIndex, 1);
    QCOMPARE(frame.width, 2);
    QCOMPARE(frame.height, 2);
    QCOMPARE(queue.size(), std::size_t{0});
}

void FrameQueueTests::tryPopReturnsFalseWhenEmpty()
{
    FrameQueue queue(2);
    Frame frame;
    QVERIFY(!queue.tryPop(frame));
}

void FrameQueueTests::dropsOldestFrameWhenFull()
{
    FrameQueue queue(2);
    queue.push(makeFrame(1));
    queue.push(makeFrame(2));
    queue.push(makeFrame(3));

    QCOMPARE(queue.size(), std::size_t{2});
    QCOMPARE(queue.droppedFrames(), std::size_t{1});

    Frame frame;
    QVERIFY(queue.tryPop(frame));
    QCOMPARE(frame.frameIndex, 2);
    QVERIFY(queue.tryPop(frame));
    QCOMPARE(frame.frameIndex, 3);
}

void FrameQueueTests::clearRemovesQueuedFrames()
{
    FrameQueue queue(3);
    queue.push(makeFrame(1));
    queue.push(makeFrame(2));
    queue.clear();

    QCOMPARE(queue.size(), std::size_t{0});

    Frame frame;
    QVERIFY(!queue.tryPop(frame));
}

void FrameQueueTests::popUnblocksWhenClosed()
{
    FrameQueue queue(1);
    std::atomic_bool popReturned = false;
    std::atomic_bool popResult = true;

    std::thread consumer([&queue, &popReturned, &popResult]() {
        Frame frame;
        popResult.store(queue.pop(frame));
        popReturned.store(true);
    });

    QTRY_VERIFY(!popReturned.load());
    queue.close();
    consumer.join();
    QVERIFY(popReturned.load());
    QVERIFY(!popResult.load());
}

void FrameQueueTests::supportsBasicProducerConsumer()
{
    FrameQueue queue(8);
    constexpr int kFrameCount = 20;
    std::atomic_int consumed = 0;

    std::thread consumer([&queue, &consumed]() {
        Frame frame;
        while (queue.pop(frame)) {
            consumed.fetch_add(1);
        }
    });

    std::thread producer([&queue]() {
        for (int index = 0; index < kFrameCount; ++index) {
            queue.push(makeFrame(index));
        }
        queue.close();
    });

    producer.join();
    consumer.join();

    QVERIFY(consumed.load() > 0);
    QVERIFY(queue.size() <= queue.maxSize());
}

QTEST_MAIN(FrameQueueTests)

#include "FrameQueueTests.moc"
