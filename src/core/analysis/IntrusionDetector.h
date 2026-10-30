#pragma once

#include <QRect>
#include <QSize>
#include <QVector>

#include "core/analysis/DetectionResult.h"
#include "core/analysis/RoiRule.h"

struct IntrusionMatch {
    RoiRule rule;
    QRect box;
    qint64 timestamp = 0;
};

class IntrusionDetector
{
public:
    QVector<IntrusionMatch> detect(const DetectionResult &result,
                                   const QSize &frameSize,
                                   const QVector<RoiRule> &roiRules) const;
};
