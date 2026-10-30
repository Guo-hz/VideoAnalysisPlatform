#pragma once

#include <memory>

#include "core/analysis/IVideoAnalyzer.h"

class MotionDetector : public IVideoAnalyzer
{
public:
    MotionDetector();
    ~MotionDetector() override;

    DetectionResult analyze(const QImage &frame,
                            const QString &sourceId,
                            int minTargetArea) override;
    void reset() override;

private:
    struct Private;
    std::unique_ptr<Private> d;
};
