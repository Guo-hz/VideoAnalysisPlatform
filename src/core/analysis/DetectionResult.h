#pragma once

#include <QRect>
#include <QString>
#include <QVector>

struct DetectionBox {
    QRect box;
    QString className;
    QString eventType;
    float confidence = 0.0f;
    int classId = -1;
};

struct DetectionResult {
    QString sourceId;
    qint64 timestamp = 0;
    QVector<QRect> boxes;
    QVector<DetectionBox> detections;
};
