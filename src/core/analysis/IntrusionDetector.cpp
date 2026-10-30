#include "IntrusionDetector.h"

#include <algorithm>

#include <QPointF>

QVector<IntrusionMatch> IntrusionDetector::detect(const DetectionResult &result,
                                                  const QSize &frameSize,
                                                  const QVector<RoiRule> &roiRules) const
{
    QVector<IntrusionMatch> matches;
    if (result.boxes.isEmpty() || roiRules.isEmpty() || !frameSize.isValid() || frameSize.isEmpty()) {
        return matches;
    }

    const QRect frameRect(0, 0, frameSize.width(), frameSize.height());
    const QRectF normalizedFrameRect(0.0, 0.0, 1.0, 1.0);
    for (const RoiRule &rule : roiRules) {
        if (!rule.enabled) {
            continue;
        }

        const QRectF roiRect = rule.rect.normalized().intersected(normalizedFrameRect);
        if (roiRect.isEmpty()) {
            continue;
        }

        for (const QRect &box : result.boxes) {
            const QRect boundedBox = box.intersected(frameRect);
            if (boundedBox.isEmpty()) {
                continue;
            }

            const QPointF center((static_cast<double>(boundedBox.x()) + boundedBox.width() / 2.0) / frameSize.width(),
                                 (static_cast<double>(boundedBox.y()) + boundedBox.height() / 2.0) / frameSize.height());
            if (!roiRect.contains(center)) {
                continue;
            }

            IntrusionMatch match;
            match.rule = rule;
            match.box = boundedBox;
            match.timestamp = result.timestamp;
            matches.append(match);
            break;
        }
    }

    return matches;
}
