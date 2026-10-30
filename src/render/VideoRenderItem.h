#pragma once

#include <QImage>
#include <QMutex>
#include <QObject>
#include <QPointer>
#include <QQuickPaintedItem>
#include <QVariantList>
#include <QVector>
#include <QtQml/qqmlregistration.h>

class StreamSession;

class VideoRenderItem : public QQuickPaintedItem
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QObject *streamSession READ streamSession WRITE setStreamSession NOTIFY streamSessionChanged)
    Q_PROPERTY(bool hasFrame READ hasFrame NOTIFY hasFrameChanged)
    Q_PROPERTY(qreal paintedWidth READ paintedWidth NOTIFY paintedGeometryChanged)
    Q_PROPERTY(qreal paintedHeight READ paintedHeight NOTIFY paintedGeometryChanged)

public:
    explicit VideoRenderItem(QQuickItem *parent = nullptr);

    QObject *streamSession() const;
    void setStreamSession(QObject *streamSession);

    bool hasFrame() const;
    qreal paintedWidth() const;
    qreal paintedHeight() const;

    void paint(QPainter *painter) override;

signals:
    void streamSessionChanged();
    void hasFrameChanged();
    void paintedGeometryChanged();

protected:
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;

private:
    void disconnectSession();
    void connectSession(StreamSession *session);
    void clearCachedState();
    void refreshFrame();
    void refreshOverlay();
    QRectF videoRectForFrame(const QSize &frameSize) const;
    void updatePaintedGeometry();

    QPointer<QObject> m_streamSession;
    QVector<QMetaObject::Connection> m_connections;
    mutable QMutex m_mutex;
    QImage m_frame;
    QVariantList m_detectionBoxes;
    QVariantList m_objectDetectionBoxes;
    QVariantList m_roiRules;
    QString m_status;
    double m_fps = 0.0;
    bool m_recording = false;
    bool m_motionDetectionEnabled = false;
    bool m_aiDetectionEnabled = false;
    bool m_hasFrame = false;
    qreal m_paintedWidth = 0.0;
    qreal m_paintedHeight = 0.0;
};
