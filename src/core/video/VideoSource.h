#pragma once

#include <QString>

enum class VideoSourceStatus {
    Stopped,
    Connecting,
    Playing,
    Paused,
    Reconnecting,
    Error,
    Ended,
    Disabled
};

enum class VideoSourceType {
    LocalFile,
    Camera,
    Rtsp,
    Http
};

struct VideoSource {
    int id = 0;
    QString name;
    VideoSourceType type = VideoSourceType::LocalFile;
    QString url;
    QString displayUrl;
    QString credentialRef;
    bool enabled = true;
    int reconnectMaxAttempts = 3;
    int reconnectIntervalMs = 3000;
    int connectTimeoutMs = 5000;
    int readTimeoutMs = 10000;
    VideoSourceStatus status = VideoSourceStatus::Stopped;
    QString lastError;
};

QString videoSourceTypeToString(VideoSourceType type);
VideoSourceType videoSourceTypeFromString(const QString &type);
QString videoSourceStatusToString(VideoSourceStatus status);
