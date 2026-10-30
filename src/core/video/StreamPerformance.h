#pragma once

#include <QString>

struct StreamPerformance {
    QString sourceName;
    double fps = 0.0;
    double decodeTimeMs = 0.0;
    int queueSize = 0;
    int droppedFrames = 0;
    QString status;
};
