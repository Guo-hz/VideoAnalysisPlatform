#pragma once

#include <QObject>
#include <QString>
#include <QVector>

class StreamSession;
class VideoFrameProvider;
class AlarmManager;
class RoiRuleRepository;
class VideoSourceRepository;
class ChannelBindingRepository;

class StreamSessionManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int count READ count CONSTANT)

public:
    explicit StreamSessionManager(VideoFrameProvider *frameProvider,
                                  AlarmManager *alarmManager = nullptr,
                                  RoiRuleRepository *roiRuleRepository = nullptr,
                                  VideoSourceRepository *videoSourceRepository = nullptr,
                                  ChannelBindingRepository *channelBindingRepository = nullptr,
                                  QString snapshotDirectory = QString(),
                                  QString recordingDirectory = QString(),
                                  QObject *parent = nullptr);
    ~StreamSessionManager() override;

    int count() const;
    QVector<StreamSession *> sessions() const;

    Q_INVOKABLE QObject *sessionAt(int index) const;
    Q_INVOKABLE bool bindSource(int channelIndex, int sourceId);
    Q_INVOKABLE int boundSourceId(int channelIndex) const;
    Q_INVOKABLE void refreshSource(int sourceId);
    Q_INVOKABLE QString lastError() const;
    Q_INVOKABLE void stopAllRecordings();
    Q_INVOKABLE void stopAll();

private:
    QVector<StreamSession *> m_sessions;
    QVector<int> m_boundSourceIds;
    VideoSourceRepository *m_videoSourceRepository = nullptr;
    ChannelBindingRepository *m_channelBindingRepository = nullptr;
    QString m_lastError;
};
