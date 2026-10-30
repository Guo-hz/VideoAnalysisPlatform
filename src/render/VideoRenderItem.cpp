#include "VideoRenderItem.h"

#include <QColor>
#include <QFont>
#include <QFontMetricsF>
#include <QMutexLocker>
#include <QPainter>
#include <QPen>
#include <QVariantMap>

#include <algorithm>
#include <utility>

#include "core/video/StreamSession.h"

namespace {

QRectF normalizedRectToPaintRect(const QVariantMap &map, const QRectF &videoRect)
{
    const QRectF normalized(map.value(QStringLiteral("x")).toDouble(),
                            map.value(QStringLiteral("y")).toDouble(),
                            map.value(QStringLiteral("width")).toDouble(),
                            map.value(QStringLiteral("height")).toDouble());
    const QRectF bounded = normalized.intersected(QRectF(0.0, 0.0, 1.0, 1.0));
    if (bounded.isEmpty() || videoRect.isEmpty()) {
        return {};
    }

    return QRectF(videoRect.x() + bounded.x() * videoRect.width(),
                  videoRect.y() + bounded.y() * videoRect.height(),
                  bounded.width() * videoRect.width(),
                  bounded.height() * videoRect.height());
}

void drawNormalizedRects(QPainter *painter,
                         const QVariantList &rects,
                         const QRectF &videoRect,
                         const QColor &fillColor,
                         const QColor &borderColor,
                         const QString &labelRole)
{
    if (!painter || rects.isEmpty() || videoRect.isEmpty()) {
        return;
    }

    QPen pen(borderColor);
    pen.setWidthF(2.0);
    painter->setPen(pen);
    painter->setBrush(fillColor);

    for (const QVariant &item : rects) {
        const QVariantMap map = item.toMap();
        if (map.contains(QStringLiteral("enabled")) && !map.value(QStringLiteral("enabled")).toBool()) {
            continue;
        }

        QRectF rect = normalizedRectToPaintRect(map, videoRect);
        if (rect.width() < 2.0 || rect.height() < 2.0) {
            continue;
        }

        painter->drawRect(rect);

        if (labelRole.isEmpty()) {
            continue;
        }

        const QString label = map.value(labelRole).toString();
        if (label.isEmpty()) {
            continue;
        }

        QFont labelFont = painter->font();
        labelFont.setPixelSize(11);
        labelFont.setBold(true);
        painter->setFont(labelFont);
        painter->setPen(QColor(QStringLiteral("#e0f2fe")));
        painter->drawText(rect.adjusted(4.0, 3.0, -4.0, -3.0), Qt::AlignLeft | Qt::AlignTop, label);
        painter->setPen(pen);
    }
}

void drawObjectRects(QPainter *painter, const QVariantList &rects, const QRectF &videoRect)
{
    if (!painter || rects.isEmpty() || videoRect.isEmpty()) {
        return;
    }

    for (const QVariant &item : rects) {
        const QVariantMap map = item.toMap();
        const QRectF rect = normalizedRectToPaintRect(map, videoRect);
        if (rect.width() < 2.0 || rect.height() < 2.0) {
            continue;
        }

        const QString eventType = map.value(QStringLiteral("eventType")).toString();
        const QColor borderColor = eventType == QStringLiteral("PersonDetected")
                                       ? QColor(QStringLiteral("#ef4444"))
                                       : eventType == QStringLiteral("VehicleDetected")
                                           ? QColor(QStringLiteral("#a855f7"))
                                           : QColor(QStringLiteral("#22c55e"));
        QColor fillColor = borderColor;
        fillColor.setAlpha(28);

        QPen pen(borderColor);
        pen.setWidthF(2.0);
        painter->setPen(pen);
        painter->setBrush(fillColor);
        painter->drawRect(rect);

        const QString className = map.value(QStringLiteral("className")).toString();
        const double confidence = map.value(QStringLiteral("confidence")).toDouble();
        const QString label = className.trimmed().isEmpty()
                                  ? QString::number(confidence, 'f', 2)
                                  : QStringLiteral("%1 %2").arg(className, QString::number(confidence, 'f', 2));
        if (label.trimmed().isEmpty()) {
            continue;
        }

        QFont labelFont = painter->font();
        labelFont.setPixelSize(11);
        labelFont.setBold(true);
        painter->setFont(labelFont);
        const QFontMetricsF metrics(labelFont);
        const QRectF labelRect(rect.left(),
                               std::max(videoRect.top(), rect.top() - metrics.height() - 6.0),
                               metrics.horizontalAdvance(label) + 8.0,
                               metrics.height() + 5.0);
        painter->setPen(Qt::NoPen);
        QColor labelBackground = borderColor;
        labelBackground.setAlpha(220);
        painter->setBrush(labelBackground);
        painter->drawRoundedRect(labelRect, 3.0, 3.0);
        painter->setPen(QColor(QStringLiteral("#ffffff")));
        painter->drawText(labelRect.adjusted(4.0, 2.0, -4.0, -2.0),
                          Qt::AlignLeft | Qt::AlignVCenter,
                          label);
    }
}

void drawStatusOverlay(QPainter *painter,
                       const QRectF &bounds,
                       const QString &status,
                       double fps,
                       bool recording)
{
    if (!painter || bounds.isEmpty()) {
        return;
    }

    QString text = QStringLiteral("%1 | FPS %2").arg(status.isEmpty() ? QStringLiteral("Stopped") : status).arg(fps, 0, 'f', 1);
    if (recording) {
        text.prepend(QStringLiteral("REC | "));
    }

    QFont font = painter->font();
    font.setPixelSize(12);
    font.setBold(true);
    painter->setFont(font);

    const QFontMetricsF metrics(font);
    const qreal paddingX = 8.0;
    const qreal paddingY = 5.0;
    const QRectF textRect(bounds.left() + 8.0,
                          bounds.top() + 8.0,
                          metrics.horizontalAdvance(text) + paddingX * 2.0,
                          metrics.height() + paddingY * 2.0);

    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(15, 23, 42, 190));
    painter->drawRoundedRect(textRect, 5.0, 5.0);
    painter->setPen(recording ? QColor(QStringLiteral("#fca5a5")) : QColor(QStringLiteral("#e5edf5")));
    painter->drawText(textRect.adjusted(paddingX, paddingY, -paddingX, -paddingY),
                      Qt::AlignLeft | Qt::AlignVCenter,
                      text);
}

} // namespace

VideoRenderItem::VideoRenderItem(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
    setAntialiasing(true);
    setOpaquePainting(true);
}

QObject *VideoRenderItem::streamSession() const
{
    return m_streamSession.data();
}

void VideoRenderItem::setStreamSession(QObject *streamSession)
{
    if (m_streamSession.data() == streamSession) {
        return;
    }

    disconnectSession();
    m_streamSession = streamSession;

    if (auto *session = qobject_cast<StreamSession *>(streamSession)) {
        connectSession(session);
        refreshFrame();
        refreshOverlay();
    } else {
        clearCachedState();
    }

    emit streamSessionChanged();
    updatePaintedGeometry();
    update();
}

bool VideoRenderItem::hasFrame() const
{
    QMutexLocker locker(&m_mutex);
    return m_hasFrame;
}

qreal VideoRenderItem::paintedWidth() const
{
    return m_paintedWidth;
}

qreal VideoRenderItem::paintedHeight() const
{
    return m_paintedHeight;
}

void VideoRenderItem::paint(QPainter *painter)
{
    if (!painter) {
        return;
    }

    QImage frame;
    QVariantList detectionBoxes;
    QVariantList objectDetectionBoxes;
    QVariantList roiRules;
    QString status;
    double fps = 0.0;
    bool recording = false;
    bool motionDetectionEnabled = false;
    bool aiDetectionEnabled = false;

    {
        QMutexLocker locker(&m_mutex);
        frame = m_frame;
        detectionBoxes = m_detectionBoxes;
        objectDetectionBoxes = m_objectDetectionBoxes;
        roiRules = m_roiRules;
        status = m_status;
        fps = m_fps;
        recording = m_recording;
        motionDetectionEnabled = m_motionDetectionEnabled;
        aiDetectionEnabled = m_aiDetectionEnabled;
    }

    const QRectF bounds = boundingRect();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter->fillRect(bounds, QColor(QStringLiteral("#0b1220")));

    const QRectF videoRect = videoRectForFrame(frame.size());
    if (!frame.isNull() && !videoRect.isEmpty()) {
        painter->drawImage(videoRect, frame);
        drawNormalizedRects(painter,
                            roiRules,
                            videoRect,
                            QColor(56, 189, 248, 38),
                            QColor(QStringLiteral("#38bdf8")),
                            QStringLiteral("ruleName"));

        if (motionDetectionEnabled) {
            drawNormalizedRects(painter,
                                detectionBoxes,
                                videoRect,
                                QColor(250, 204, 21, 20),
                                QColor(QStringLiteral("#facc15")),
                                QString());
        }

        if (aiDetectionEnabled) {
            drawObjectRects(painter, objectDetectionBoxes, videoRect);
        }
    }

    drawStatusOverlay(painter, bounds, status, fps, recording);
}

void VideoRenderItem::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickPaintedItem::geometryChange(newGeometry, oldGeometry);
    updatePaintedGeometry();
}

void VideoRenderItem::disconnectSession()
{
    for (const QMetaObject::Connection &connection : std::as_const(m_connections)) {
        QObject::disconnect(connection);
    }
    m_connections.clear();
}

void VideoRenderItem::connectSession(StreamSession *session)
{
    if (!session) {
        return;
    }

    m_connections.append(connect(session, &StreamSession::frameSourceChanged, this, &VideoRenderItem::refreshFrame));
    m_connections.append(connect(session, &StreamSession::detectionBoxesChanged, this, &VideoRenderItem::refreshOverlay));
    m_connections.append(connect(session, &StreamSession::objectDetectionBoxesChanged, this, &VideoRenderItem::refreshOverlay));
    m_connections.append(connect(session, &StreamSession::roiRulesChanged, this, &VideoRenderItem::refreshOverlay));
    m_connections.append(connect(session, &StreamSession::statusChanged, this, &VideoRenderItem::refreshOverlay));
    m_connections.append(connect(session, &StreamSession::fpsChanged, this, &VideoRenderItem::refreshOverlay));
    m_connections.append(connect(session, &StreamSession::recordingChanged, this, &VideoRenderItem::refreshOverlay));
    m_connections.append(connect(session,
                                 &StreamSession::motionDetectionEnabledChanged,
                                 this,
                                 &VideoRenderItem::refreshOverlay));
    m_connections.append(connect(session,
                                 &StreamSession::aiDetectionEnabledChanged,
                                 this,
                                 &VideoRenderItem::refreshOverlay));
    m_connections.append(connect(session, &QObject::destroyed, this, [this]() {
        m_streamSession.clear();
        m_connections.clear();
        clearCachedState();
        emit streamSessionChanged();
        updatePaintedGeometry();
        update();
    }));
}

void VideoRenderItem::clearCachedState()
{
    bool hasFrameStateChanged = false;
    {
        QMutexLocker locker(&m_mutex);
        hasFrameStateChanged = m_hasFrame;
        m_frame = QImage();
        m_detectionBoxes.clear();
        m_objectDetectionBoxes.clear();
        m_roiRules.clear();
        m_status.clear();
        m_fps = 0.0;
        m_recording = false;
        m_motionDetectionEnabled = false;
        m_aiDetectionEnabled = false;
        m_hasFrame = false;
    }

    if (hasFrameStateChanged) {
        emit hasFrameChanged();
    }
}

void VideoRenderItem::refreshFrame()
{
    auto *session = qobject_cast<StreamSession *>(m_streamSession.data());
    const QImage frame = session ? session->currentFrame() : QImage();
    const bool nextHasFrame = !frame.isNull();
    bool hasFrameStateChanged = false;

    {
        QMutexLocker locker(&m_mutex);
        hasFrameStateChanged = m_hasFrame != nextHasFrame;
        m_frame = frame;
        m_hasFrame = nextHasFrame;
    }

    if (hasFrameStateChanged) {
        emit hasFrameChanged();
    }

    updatePaintedGeometry();
    update();
}

void VideoRenderItem::refreshOverlay()
{
    auto *session = qobject_cast<StreamSession *>(m_streamSession.data());
    {
        QMutexLocker locker(&m_mutex);
        if (session) {
            m_detectionBoxes = session->detectionBoxes();
            m_objectDetectionBoxes = session->objectDetectionBoxes();
            m_roiRules = session->roiRules();
            m_status = session->status();
            m_fps = session->fps();
            m_recording = session->recording();
            m_motionDetectionEnabled = session->motionDetectionEnabled();
            m_aiDetectionEnabled = session->aiDetectionEnabled();
        } else {
            m_detectionBoxes.clear();
            m_objectDetectionBoxes.clear();
            m_roiRules.clear();
            m_status.clear();
            m_fps = 0.0;
            m_recording = false;
            m_motionDetectionEnabled = false;
            m_aiDetectionEnabled = false;
        }
    }

    update();
}

QRectF VideoRenderItem::videoRectForFrame(const QSize &frameSize) const
{
    if (frameSize.isEmpty() || width() <= 0.0 || height() <= 0.0) {
        return {};
    }

    const qreal scale = qMin(width() / static_cast<qreal>(frameSize.width()),
                             height() / static_cast<qreal>(frameSize.height()));
    const QSizeF paintedSize(frameSize.width() * scale, frameSize.height() * scale);
    return QRectF((width() - paintedSize.width()) / 2.0,
                  (height() - paintedSize.height()) / 2.0,
                  paintedSize.width(),
                  paintedSize.height());
}

void VideoRenderItem::updatePaintedGeometry()
{
    QSize frameSize;
    {
        QMutexLocker locker(&m_mutex);
        frameSize = m_frame.size();
    }

    const QRectF rect = videoRectForFrame(frameSize);
    const qreal nextWidth = rect.width();
    const qreal nextHeight = rect.height();
    if (qFuzzyCompare(m_paintedWidth + 1.0, nextWidth + 1.0)
        && qFuzzyCompare(m_paintedHeight + 1.0, nextHeight + 1.0)) {
        return;
    }

    m_paintedWidth = nextWidth;
    m_paintedHeight = nextHeight;
    emit paintedGeometryChanged();
}
