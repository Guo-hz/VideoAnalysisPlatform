#pragma once

#include <QMetaType>
#include <QString>

struct AlarmEvent {
    int id = -1;
    int sourceId = 0;
    int videoSourceId = 0;
    QString legacySourceKey;
    QString sourceName;
    QString eventType;
    QString level;
    QString message;
    QString snapshotPath;
    QString videoClipPath;
    bool confirmed = false;
    QString timestamp;
};

Q_DECLARE_METATYPE(AlarmEvent)
