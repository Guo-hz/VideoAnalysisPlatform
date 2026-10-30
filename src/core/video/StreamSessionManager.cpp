#include "StreamSessionManager.h"

#include "core/alarm/AlarmManager.h"
#include "core/security/CredentialStore.h"
#include "core/storage/ChannelBindingRepository.h"
#include "core/storage/RoiRuleRepository.h"
#include "core/storage/VideoSourceRepository.h"
#include "core/video/StreamSession.h"
#include "render/VideoFrameProvider.h"

namespace {
constexpr int kMaxStreamSessions = 4;
}

StreamSessionManager::StreamSessionManager(VideoFrameProvider *frameProvider,
                                           AlarmManager *alarmManager,
                                           RoiRuleRepository *roiRuleRepository,
                                           VideoSourceRepository *videoSourceRepository,
                                           ChannelBindingRepository *channelBindingRepository,
                                           QString snapshotDirectory,
                                           QString recordingDirectory,
                                           QObject *parent)
    : QObject(parent)
    , m_boundSourceIds(kMaxStreamSessions, 0)
    , m_videoSourceRepository(videoSourceRepository)
    , m_channelBindingRepository(channelBindingRepository)
{
    m_sessions.reserve(kMaxStreamSessions);
    for (int index = 0; index < kMaxStreamSessions; ++index) {
        const QString streamId = QStringLiteral("stream%1").arg(index + 1);
        auto *session = new StreamSession(streamId,
                                          frameProvider,
                                          alarmManager,
                                          roiRuleRepository,
                                          snapshotDirectory,
                                          recordingDirectory,
                                          this);
        if (alarmManager) {
            connect(session,
                    &StreamSession::motionDetected,
                    alarmManager,
                    &AlarmManager::reportMotionDetected,
                    Qt::QueuedConnection);
            connect(session,
                    &StreamSession::intrusionDetected,
                    alarmManager,
                    &AlarmManager::reportIntrusionDetected,
                    Qt::QueuedConnection);
            connect(session,
                    &StreamSession::objectDetected,
                    alarmManager,
                    &AlarmManager::reportObjectDetected,
                    Qt::QueuedConnection);
        }
        m_sessions.append(session);
    }

    if (m_channelBindingRepository) {
        const QHash<int, int> bindings = m_channelBindingRepository->loadAll();
        for (auto it = bindings.constBegin(); it != bindings.constEnd(); ++it) {
            bindSource(it.key(), it.value());
        }
    }
}

StreamSessionManager::~StreamSessionManager()
{
    stopAll();
}

int StreamSessionManager::count() const
{
    return m_sessions.size();
}

QVector<StreamSession *> StreamSessionManager::sessions() const
{
    return m_sessions;
}

QObject *StreamSessionManager::sessionAt(int index) const
{
    if (index < 0 || index >= m_sessions.size()) {
        return nullptr;
    }

    return m_sessions.at(index);
}

bool StreamSessionManager::bindSource(int channelIndex, int sourceId)
{
    if (channelIndex < 0 || channelIndex >= m_sessions.size() || sourceId <= 0) {
        m_lastError = QStringLiteral("绑定视频源失败：通道或视频源无效。");
        return false;
    }
    if (!m_videoSourceRepository) {
        m_lastError = QStringLiteral("绑定视频源失败：视频源仓库未初始化。");
        return false;
    }
    VideoSource source;
    if (!m_videoSourceRepository->findById(sourceId, &source) || !source.enabled) {
        m_lastError = m_videoSourceRepository->lastError().isEmpty()
                          ? QStringLiteral("视频源不存在或已禁用。")
                          : m_videoSourceRepository->lastError();
        return false;
    }
    source.displayUrl = source.url;
    if ((source.type == VideoSourceType::Rtsp || source.type == VideoSourceType::Http)
        && !source.credentialRef.isEmpty()) {
        CredentialStore::Secret secret;
        QString error;
        if (!CredentialStore::read(source.credentialRef, &secret, &error)) {
            m_lastError = QStringLiteral("读取视频源凭据失败：%1").arg(error);
            return false;
        }
        source.url = secret.endpoint;
    }

    int previousChannelForSource = -1;
    for (int index = 0; index < m_boundSourceIds.size(); ++index) {
        if (index != channelIndex && m_boundSourceIds.at(index) == sourceId) {
            previousChannelForSource = index;
            break;
        }
    }
    if (!m_sessions.at(channelIndex)->openSource(source)) {
        m_lastError = m_sessions.at(channelIndex)->lastError();
        return false;
    }
    if (m_channelBindingRepository && !m_channelBindingRepository->bind(channelIndex, sourceId)) {
        m_lastError = m_channelBindingRepository->lastError();
        m_sessions.at(channelIndex)->stop();
        return false;
    }
    if (previousChannelForSource >= 0) {
        m_sessions.at(previousChannelForSource)->stop();
        m_boundSourceIds[previousChannelForSource] = 0;
    }
    m_boundSourceIds[channelIndex] = sourceId;
    m_lastError.clear();
    return true;
}

int StreamSessionManager::boundSourceId(int channelIndex) const
{
    if (channelIndex < 0 || channelIndex >= m_boundSourceIds.size()) {
        return 0;
    }
    return m_boundSourceIds.at(channelIndex);
}

void StreamSessionManager::refreshSource(int sourceId)
{
    if (sourceId <= 0) {
        return;
    }
    for (int index = 0; index < m_boundSourceIds.size(); ++index) {
        if (m_boundSourceIds.at(index) == sourceId) {
            VideoSource source;
            if (!m_videoSourceRepository || !m_videoSourceRepository->findById(sourceId, &source)
                || !source.enabled) {
                m_sessions.at(index)->stop();
                m_boundSourceIds[index] = 0;
                if (m_channelBindingRepository) {
                    m_channelBindingRepository->unbindChannel(index);
                }
                continue;
            }
            source.displayUrl = source.url;
            if ((source.type == VideoSourceType::Rtsp || source.type == VideoSourceType::Http)
                && !source.credentialRef.isEmpty()) {
                CredentialStore::Secret secret;
                if (!CredentialStore::read(source.credentialRef, &secret, &m_lastError)) {
                    m_sessions.at(index)->stop();
                    continue;
                }
                source.url = secret.endpoint;
            }
            if (!m_sessions.at(index)->openSource(source)) {
                m_lastError = m_sessions.at(index)->lastError();
            }
        }
    }
}

QString StreamSessionManager::lastError() const
{
    return m_lastError;
}

void StreamSessionManager::stopAllRecordings()
{
    for (StreamSession *session : m_sessions) {
        if (session) {
            session->stopRecording();
        }
    }
}

void StreamSessionManager::stopAll()
{
    for (StreamSession *session : m_sessions) {
        if (session) {
            session->stop();
        }
    }
}
