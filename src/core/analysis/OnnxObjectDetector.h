#pragma once

#include <memory>

#include "core/analysis/IObjectDetector.h"

class OnnxObjectDetector : public IObjectDetector
{
public:
    OnnxObjectDetector();
    ~OnnxObjectDetector() override;

    bool loadModel(const QString &modelPath) override;
    bool isReady() const override;
    QString lastError() const override;
    void setConfidenceThreshold(float threshold) override;
    float confidenceThreshold() const override;
    DetectionResult detect(const QImage &frame, const QString &sourceId) override;
    void reset() override;

private:
    struct Private;
    std::unique_ptr<Private> d;
};
