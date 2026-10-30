#pragma once

#include <QImage>
#include <QString>

#include "core/analysis/DetectionResult.h"

class IObjectDetector
{
public:
    virtual ~IObjectDetector() = default;

    virtual bool loadModel(const QString &modelPath) = 0;
    virtual bool isReady() const = 0;
    virtual QString lastError() const = 0;
    virtual void setConfidenceThreshold(float threshold) = 0;
    virtual float confidenceThreshold() const = 0;
    virtual DetectionResult detect(const QImage &frame, const QString &sourceId) = 0;
    virtual void reset() = 0;
};
