#pragma once

#include <QMetaType>
#include <QRectF>
#include <QString>

struct RoiRule {
    int id = -1;
    int sourceId = 0;
    QString ruleName;
    QRectF rect;
    bool enabled = true;
    int cooldownSeconds = 10;
    QString createdAt;
    QString updatedAt;
};

Q_DECLARE_METATYPE(RoiRule)
