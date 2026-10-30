#include "VideoSource.h"

QString videoSourceTypeToString(VideoSourceType type)
{
    switch (type) {
    case VideoSourceType::LocalFile:
        return QStringLiteral("LocalFile");
    case VideoSourceType::Camera:
        return QStringLiteral("Camera");
    case VideoSourceType::Rtsp:
        return QStringLiteral("RTSP");
    case VideoSourceType::Http:
        return QStringLiteral("HTTP");
    }

    return QStringLiteral("Unknown");
}

VideoSourceType videoSourceTypeFromString(const QString &type)
{
    if (type.compare(QStringLiteral("Camera"), Qt::CaseInsensitive) == 0) {
        return VideoSourceType::Camera;
    }
    if (type.compare(QStringLiteral("RTSP"), Qt::CaseInsensitive) == 0) {
        return VideoSourceType::Rtsp;
    }
    if (type.compare(QStringLiteral("HTTP"), Qt::CaseInsensitive) == 0) {
        return VideoSourceType::Http;
    }

    return VideoSourceType::LocalFile;
}

QString videoSourceStatusToString(VideoSourceStatus status)
{
    switch (status) {
    case VideoSourceStatus::Stopped:
        return QStringLiteral("Idle");
    case VideoSourceStatus::Connecting:
        return QStringLiteral("Opening");
    case VideoSourceStatus::Playing:
        return QStringLiteral("Playing");
    case VideoSourceStatus::Paused:
        return QStringLiteral("Paused");
    case VideoSourceStatus::Reconnecting:
        return QStringLiteral("Reconnecting");
    case VideoSourceStatus::Error:
        return QStringLiteral("Error");
    case VideoSourceStatus::Ended:
        return QStringLiteral("Ended");
    case VideoSourceStatus::Disabled:
        return QStringLiteral("Disabled");
    }

    return QStringLiteral("Unknown");
}
