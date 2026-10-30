#pragma once

#include <QImage>
#include <QString>

#include "core/analysis/DetectionResult.h"

class IVideoAnalyzer
{
public:
    virtual ~IVideoAnalyzer() = default;

    virtual DetectionResult analyze(const QImage &frame,
                                    const QString &sourceId,
                                    int minTargetArea) = 0;
    virtual void reset() = 0;
};
