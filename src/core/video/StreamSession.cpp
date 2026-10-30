#include "StreamSession.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>

#include <QElapsedTimer>
#include <QFileInfo>
#include <QDir>
#include <QMetaObject>
#include <QDebug>
#include <QMutexLocker>
#include <QVariantMap>

#include "core/storage/RoiRuleRepository.h"
#include "core/video/FFmpegVideoDecoder.h"
#include "core/video/OpenCVVideoDecoder.h"
#include "render/VideoFrameProvider.h"

namespace {
constexpr int kFallbackFrameDelayMs = 33;
constexpr std::size_t kFrameQueueMaxSize = 3;
constexpr int kReconnectDelayMs = 3000;
constexpr int kReconnectPollMs = 100;
constexpr int kMaxReconnectAttemptsLimit = 20;
constexpr int kMaxAiInferenceInterval = 300;

double normalizedConfidenceThreshold(double threshold)
{
    if (!std::isfinite(threshold)) {
        return 0.45;
    }
    return std::clamp(threshold, 0.01, 0.99);
}

int sourceIdFromStreamId(const QString &streamId)
{
    QString digits;
    for (const QChar &character : streamId) {
        if (character.isDigit()) {
            digits.append(character);
        }
    }

    bool ok = false;
    const int sourceId = digits.toInt(&ok);
    return ok ? sourceId : 0;
}

QString sourceNameFromPath(const QString &filePath, const QString &fallback)
{
    const QFileInfo fileInfo(filePath);
    return fileInfo.exists() ? fileInfo.fileName() : fallback;
}

QString sourceNameForSource(VideoSourceType type, const QString &url, const QString &fallback)
{
    if (type == VideoSourceType::Rtsp || type == VideoSourceType::Http) {
        const QUrl parsedUrl(url);
        if (!parsedUrl.host().isEmpty()) {
            return parsedUrl.host();
        }
        return url.trimmed().isEmpty() ? fallback : url.trimmed();
    }

    return sourceNameFromPath(url, fallback);
}

QRectF normalizedRoiRect(double x, double y, double width, double height)
{
    const double left = std::clamp(x, 0.0, 1.0);
    const double top = std::clamp(y, 0.0, 1.0);
    const double right = std::clamp(x + width, 0.0, 1.0);
    const double bottom = std::clamp(y + height, 0.0, 1.0);

    return QRectF(QPointF(std::min(left, right), std::min(top, bottom)),
                  QPointF(std::max(left, right), std::max(top, bottom))).normalized();
}

QVariantMap roiRuleToMap(const RoiRule &rule, int index)
{
    QVariantMap item;
    item.insert(QStringLiteral("index"), index);
    item.insert(QStringLiteral("id"), rule.id);
    item.insert(QStringLiteral("sourceId"), rule.sourceId);
    item.insert(QStringLiteral("ruleName"), rule.ruleName);
    item.insert(QStringLiteral("x"), rule.rect.x());
    item.insert(QStringLiteral("y"), rule.rect.y());
    item.insert(QStringLiteral("width"), rule.rect.width());
    item.insert(QStringLiteral("height"), rule.rect.height());
    item.insert(QStringLiteral("enabled"), rule.enabled);
    item.insert(QStringLiteral("cooldownSeconds"), rule.cooldownSeconds);
    return item;
}
}

class ObjectDetectionWorker : public QObject
{
public:
    bool loadModel(const QString &modelPath, double confidenceThreshold)
    {
        m_detector.setConfidenceThreshold(static_cast<float>(confidenceThreshold));
        return m_detector.loadModel(modelPath);
    }

    DetectionResult detect(const QImage &frame, const QString &sourceId, double confidenceThreshold)
    {
        m_detector.setConfidenceThreshold(static_cast<float>(confidenceThreshold));
        return m_detector.detect(frame, sourceId);
    }

    QString lastError() const
    {
        return m_detector.lastError();
    }

private:
    OnnxObjectDetector m_detector;
};

StreamSession::StreamSession(QString streamId,
                             VideoFrameProvider *frameProvider,
                             AlarmManager *alarmManager,
                             RoiRuleRepository *roiRuleRepository,
                             QString snapshotDirectory,
                             QString recordingDirectory,
                             QObject *parent)
    : QObject(parent)
    , m_frameProvider(frameProvider)
    , m_alarmManager(alarmManager)
    , m_roiRuleRepository(roiRuleRepository)
    , m_streamId(std::move(streamId))
    , m_frameQueue(kFrameQueueMaxSize)
    , m_snapshotManager(snapshotDirectory.trimmed().isEmpty()
                            ? QStringLiteral("data/snapshots")
                            : std::move(snapshotDirectory))
    , m_recordingManager(recordingDirectory.trimmed().isEmpty()
                             ? QStringLiteral("data/records")
                             : std::move(recordingDirectory))
{
    setStatus(VideoSourceStatus::Stopped);
    setAiStatus(QStringLiteral("AI 模型未加载。"));
    m_objectDetectionThread = new QThread();
    m_objectDetectionWorker = new ObjectDetectionWorker();
    m_objectDetectionWorker->moveToThread(m_objectDetectionThread);
    m_objectDetectionThread->start();
    loadRoiRulesFromRepository();
}

StreamSession::~StreamSession()
{
    stopRecordingInternal(false);
    stopRecordingInternal(true);
    stopWorker();
    waitForSnapshotWorkers();
    stopObjectDetectionWorker();
}

QString StreamSession::status() const
{
    return videoSourceStatusToString(m_status);
}

QString StreamSession::streamId() const
{
    return m_streamId;
}

int StreamSession::sourceId() const
{
    return m_sourceId;
}

QString StreamSession::sourceName() const
{
    return m_sourceName;
}

double StreamSession::fps() const
{
    return m_fps;
}

QString StreamSession::frameSource() const
{
    return m_frameSource;
}

QImage StreamSession::currentFrame() const
{
    return m_frameProvider ? m_frameProvider->currentFrame(m_streamId) : QImage();
}

QString StreamSession::sourcePath() const
{
    return m_sourcePath;
}

QString StreamSession::lastError() const
{
    return m_lastError;
}

QString StreamSession::lastSnapshotPath() const
{
    return m_lastSnapshotPath;
}

bool StreamSession::snapshotBusy() const
{
    return m_snapshotBusy.load();
}

bool StreamSession::recording() const
{
    return m_recordingManager.isRecording();
}

QString StreamSession::recordingPath() const
{
    return m_recordingPath;
}

bool StreamSession::motionDetectionEnabled() const
{
    return m_motionDetectionEnabled.load();
}

void StreamSession::setMotionDetectionEnabled(bool enabled)
{
    if (m_motionDetectionEnabled.load() == enabled) {
        return;
    }

    m_motionDetectionEnabled.store(enabled);
    m_motionDetectorResetRequested.store(true);
    if (!enabled) {
        clearDetectionBoxes();
    }
    emit motionDetectionEnabledChanged();
}

int StreamSession::minTargetArea() const
{
    return m_minTargetArea.load();
}

void StreamSession::setMinTargetArea(int area)
{
    const int normalizedArea = std::max(1, area);
    if (m_minTargetArea.load() == normalizedArea) {
        return;
    }

    m_minTargetArea.store(normalizedArea);
    m_motionDetectorResetRequested.store(true);
    emit minTargetAreaChanged();
}

bool StreamSession::aiDetectionEnabled() const
{
    return m_aiDetectionEnabled.load();
}

void StreamSession::setAiDetectionEnabled(bool enabled)
{
    if (m_aiDetectionEnabled.load() == enabled) {
        return;
    }

    m_aiDetectionEnabled.store(enabled);
    if (!enabled) {
        clearObjectDetectionBoxes();
        setAiStatus(m_aiModelReady.load() ? QStringLiteral("AI 检测已关闭。")
                                          : QStringLiteral("AI 模型未加载。"));
    } else if (!m_aiModelReady.load()) {
        setAiStatus(m_aiModelLoading.load() ? QStringLiteral("AI 模型正在加载...")
                                            : QStringLiteral("请先加载 ONNX 模型。"));
    } else {
        setAiStatus(QStringLiteral("AI 检测已启用。"));
    }

    emit aiDetectionEnabledChanged();
}

QString StreamSession::aiModelPath() const
{
    return m_aiModelPath;
}

QString StreamSession::aiStatus() const
{
    return m_aiStatus;
}

bool StreamSession::aiModelReady() const
{
    return m_aiModelReady.load();
}

bool StreamSession::aiModelLoading() const
{
    return m_aiModelLoading.load();
}

double StreamSession::aiConfidenceThreshold() const
{
    return m_aiConfidenceThreshold.load();
}

void StreamSession::setAiConfidenceThreshold(double threshold)
{
    const double normalizedThreshold = normalizedConfidenceThreshold(threshold);
    if (qFuzzyCompare(m_aiConfidenceThreshold.load(), normalizedThreshold)) {
        return;
    }

    m_aiConfidenceThreshold.store(normalizedThreshold);
    emit aiConfidenceThresholdChanged();
}

int StreamSession::aiInferenceInterval() const
{
    return m_aiInferenceInterval.load();
}

void StreamSession::setAiInferenceInterval(int interval)
{
    const int normalizedInterval = std::clamp(interval, 1, kMaxAiInferenceInterval);
    if (m_aiInferenceInterval.load() == normalizedInterval) {
        return;
    }

    m_aiInferenceInterval.store(normalizedInterval);
    emit aiInferenceIntervalChanged();
}

int StreamSession::maxReconnectAttempts() const
{
    return m_maxReconnectAttempts.load();
}

void StreamSession::setMaxReconnectAttempts(int attempts)
{
    const int normalizedAttempts = std::clamp(attempts, 0, kMaxReconnectAttemptsLimit);
    if (m_maxReconnectAttempts.load() == normalizedAttempts) {
        return;
    }

    m_maxReconnectAttempts.store(normalizedAttempts);
    emit maxReconnectAttemptsChanged();
}

int StreamSession::reconnectAttempt() const
{
    return m_reconnectAttempt.load();
}

QVariantList StreamSession::detectionBoxes() const
{
    return m_detectionBoxes;
}

QVariantList StreamSession::objectDetectionBoxes() const
{
    return m_objectDetectionBoxes;
}

QVariantList StreamSession::roiRules() const
{
    QVariantList rules;
    QMutexLocker locker(&m_roiRulesMutex);
    for (int index = 0; index < m_roiRules.size(); ++index) {
        rules.append(roiRuleToMap(m_roiRules.at(index), index));
    }
    return rules;
}

StreamPerformance StreamSession::performanceSnapshot() const
{
    StreamPerformance performance;
    performance.sourceName = m_sourceName.trimmed().isEmpty()
                                 ? sourceNameForSource(m_sourceType, m_sourcePath, m_streamId)
                                 : m_sourceName;
    performance.fps = m_currentFps.load();
    performance.decodeTimeMs = m_decodeTimeMs.load();
    performance.queueSize = static_cast<int>(m_frameQueue.size());
    performance.droppedFrames = static_cast<int>(m_frameQueue.droppedFrames());
    performance.status = status();
    return performance;
}

bool StreamSession::openLocalFile(const QUrl &fileUrl)
{
    const QString localPath = fileUrl.isLocalFile() ? fileUrl.toLocalFile() : fileUrl.toString();
    if (localPath.trimmed().isEmpty()) {
        setLastError(QStringLiteral("请选择本地 mp4 文件。"));
        return false;
    }

    VideoSource source;
    source.id = sourceIdFromStreamId(m_streamId);
    source.name = sourceNameFromPath(localPath, m_streamId);
    source.type = VideoSourceType::LocalFile;
    source.url = localPath;
    source.displayUrl = localPath;
    return openSource(source);
}

bool StreamSession::openRtspUrl(const QString &url)
{
    const QString trimmedUrl = url.trimmed();
    const QUrl parsedUrl(trimmedUrl);
    if (trimmedUrl.isEmpty()
        || !parsedUrl.isValid()
        || parsedUrl.scheme().compare(QStringLiteral("rtsp"), Qt::CaseInsensitive) != 0
        || parsedUrl.host().isEmpty()) {
        setLastError(QStringLiteral("请输入有效的 RTSP 地址，例如 rtsp://host/path。"));
        return false;
    }

    VideoSource source;
    source.id = sourceIdFromStreamId(m_streamId);
    source.name = sourceNameForSource(VideoSourceType::Rtsp, trimmedUrl, m_streamId);
    source.type = VideoSourceType::Rtsp;
    source.url = trimmedUrl;
    source.displayUrl = trimmedUrl;
    return openSource(source);
}

bool StreamSession::openHttpUrl(const QString &url)
{
    const QString trimmedUrl = url.trimmed();
    const QUrl parsedUrl(trimmedUrl);
    const QString scheme = parsedUrl.scheme().toLower();
    if (trimmedUrl.isEmpty() || !parsedUrl.isValid() || parsedUrl.host().isEmpty()
        || (scheme != QStringLiteral("http") && scheme != QStringLiteral("https"))) {
        setLastError(QStringLiteral("请输入有效的 HTTP/HLS/MJPEG 直链地址。"));
        return false;
    }

    VideoSource source;
    source.id = sourceIdFromStreamId(m_streamId);
    source.name = sourceNameForSource(VideoSourceType::Http, trimmedUrl, m_streamId);
    source.type = VideoSourceType::Http;
    source.url = trimmedUrl;
    source.displayUrl = trimmedUrl;
    return openSource(source);
}

bool StreamSession::openSource(const VideoSource &source)
{
    if (!source.enabled || source.url.trimmed().isEmpty()) {
        setLastError(QStringLiteral("视频源已禁用或地址为空。"));
        return false;
    }
    stopRecordingInternal(true);
    stopWorker();
    m_sourceId = source.id;
    m_sourceName = source.name.trimmed().isEmpty() ? m_streamId : source.name.trimmed();
    m_sourcePath = source.displayUrl.trimmed().isEmpty() ? source.url : source.displayUrl;
    m_openUrl = source.url;
    m_sourceType = source.type;
    m_maxReconnectAttempts.store(std::clamp(source.reconnectMaxAttempts, 0, kMaxReconnectAttemptsLimit));
    m_reconnectIntervalMs.store(std::clamp(source.reconnectIntervalMs, 250, 60000));
    emit sourceChanged();
    emit sourcePathChanged();
    clearFrame();
    clearDetectionBoxes();
    clearObjectDetectionBoxes();
    loadRoiRulesFromRepository();
    m_motionDetector.reset();
    m_motionDetectorResetRequested.store(true);
    setFps(0.0);
    setReconnectAttempt(0);
    setLastError(QString());
    setStatus(VideoSourceStatus::Stopped);
    return true;
}

void StreamSession::play()
{
    if (m_openUrl.trimmed().isEmpty()) {
        setLastError(QStringLiteral("请先选择本地 mp4 文件或输入 RTSP 地址。"));
        return;
    }

    if (m_decodeThread && m_decodeThread->isRunning() && !m_stopRequested.load()) {
        m_pauseRequested.store(false);
        if (m_status != VideoSourceStatus::Reconnecting) {
            setStatus(VideoSourceStatus::Playing);
        }
        return;
    }

    stopWorker();
    m_stopRequested.store(false);
    m_pauseRequested.store(false);
    m_frameQueue.clear();
    m_frameQueue.reopen();
    m_currentFps.store(0.0);
    m_decodeTimeMs.store(0.0);
    m_sourceFrameRate.store(25.0);
    m_motionDetectorResetRequested.store(true);
    setReconnectAttempt(0);
    setLastError(QString());
    setStatus(VideoSourceStatus::Connecting);

    VideoSource source;
    source.id = m_sourceId;
    source.name = m_sourceName.trimmed().isEmpty()
                      ? sourceNameForSource(m_sourceType, m_sourcePath, m_streamId)
                      : m_sourceName;
    source.type = m_sourceType;
    source.url = m_openUrl;
    source.displayUrl = m_sourcePath;
    source.enabled = true;

    const quint64 generation = ++m_runGeneration;
    m_decodeThread = QThread::create([this, source, generation]() {
        runDecodeLoop(source, generation);
    });
    QThread *decodeThread = m_decodeThread;
    connect(m_decodeThread, &QThread::finished, m_decodeThread, &QObject::deleteLater);
    connect(m_decodeThread, &QThread::finished, this, [this, decodeThread]() {
        if (m_decodeThread == decodeThread) {
            m_decodeThread = nullptr;
        }
    });

    m_dispatchThread = QThread::create([this, generation]() {
        runFrameDispatchLoop(generation);
    });
    QThread *dispatchThread = m_dispatchThread;
    connect(m_dispatchThread, &QThread::finished, m_dispatchThread, &QObject::deleteLater);
    connect(m_dispatchThread, &QThread::finished, this, [this, dispatchThread]() {
        if (m_dispatchThread == dispatchThread) {
            m_dispatchThread = nullptr;
        }
    });

    m_dispatchThread->start();
    m_decodeThread->start();
}

void StreamSession::pause()
{
    if (!m_decodeThread || !m_decodeThread->isRunning()) {
        return;
    }

    m_pauseRequested.store(true);
    setStatus(VideoSourceStatus::Paused);
}

void StreamSession::stop()
{
    stopRecordingInternal(true);
    stopWorker();
    clearFrame();
    clearDetectionBoxes();
    clearObjectDetectionBoxes();
    m_motionDetector.reset();
    setFps(0.0);
    setReconnectAttempt(0);
    setStatus(VideoSourceStatus::Stopped);
}

bool StreamSession::takeSnapshot(const QString &format, const QString &eventType)
{
    if (m_snapshotBusy.exchange(true)) {
        const QString error = QStringLiteral("截图正在保存，请稍候。");
        emit snapshotFinished(false, QString(), error);
        return false;
    }
    if (!m_frameProvider) {
        m_snapshotBusy.store(false);
        const QString error = QStringLiteral("截图失败：视频帧提供器不可用。");
        emit snapshotFinished(false, QString(), error);
        return false;
    }

    const QImage frame = m_frameProvider->currentFrame(m_streamId);
    if (frame.isNull()) {
        m_snapshotBusy.store(false);
        const QString error = QStringLiteral("截图失败：当前没有可保存的视频画面。");
        emit snapshotFinished(false, QString(), error);
        return false;
    }

    const QString requestedFormat = format.trimmed().isEmpty() ? QStringLiteral("jpg") : format;
    const QString requestedEventType = eventType.trimmed().isEmpty() ? QStringLiteral("manual") : eventType;
    const QString sourceId = m_streamId;
    const SnapshotManager snapshotManager = m_snapshotManager;

    QThread *snapshotThread = QThread::create([this, snapshotManager, frame, sourceId, requestedEventType, requestedFormat]() {
        const SnapshotResult result = snapshotManager.saveSnapshot(frame, sourceId, requestedEventType, requestedFormat);
        m_snapshotBusy.store(false);
        QMetaObject::invokeMethod(this, [this]() { emit snapshotBusyChanged(); }, Qt::QueuedConnection);
        QMetaObject::invokeMethod(this, [this, result]() {
            if (result.success) {
                if (m_lastSnapshotPath != result.filePath) {
                    m_lastSnapshotPath = result.filePath;
                    emit lastSnapshotPathChanged();
                }
            } else {
                qWarning().noquote() << result.errorMessage;
            }

            emit snapshotFinished(result.success, result.filePath, result.errorMessage);
        }, Qt::QueuedConnection);
    });

    m_snapshotThreads.append(snapshotThread);
    connect(snapshotThread, &QThread::finished, this, [this, snapshotThread]() {
        m_snapshotThreads.removeOne(snapshotThread);
    });
    connect(snapshotThread, &QThread::finished, snapshotThread, &QObject::deleteLater);
    snapshotThread->start();
    emit snapshotBusyChanged();
    return true;
}

bool StreamSession::startRecording()
{
    if (m_status != VideoSourceStatus::Playing) {
        const QString error = QStringLiteral("录像只能在播放状态下开始。");
        setLastError(error);
        emit recordingResult(false, QString(), error);
        return false;
    }
    if (m_recordingManager.isRecording()) {
        return true;
    }

    if (!m_frameProvider) {
        const QString error = QStringLiteral("Recording failed: video frame provider is unavailable.");
        setLastError(error);
        emit recordingResult(false, QString(), error);
        return false;
    }

    const QImage frame = m_frameProvider->currentFrame(m_streamId);
    if (frame.isNull()) {
        const QString error = QStringLiteral("Recording failed: no video frame is available.");
        setLastError(error);
        emit recordingResult(false, QString(), error);
        return false;
    }

    double recordingFps = m_sourceFrameRate.load();
    if (recordingFps <= 0.0 || !std::isfinite(recordingFps)) {
        recordingFps = m_currentFps.load();
    }
    if (recordingFps <= 0.0 || !std::isfinite(recordingFps)) {
        recordingFps = 25.0;
    }

    RecordingResult result = m_recordingManager.startRecording(m_streamId, frame.size(), recordingFps);
    if (!result.success) {
        setLastError(result.errorMessage);
        emit recordingResult(false, result.filePath, result.errorMessage);
        return false;
    }

    const RecordingResult firstFrameResult = m_recordingManager.writeFrame(frame);
    if (!firstFrameResult.success) {
        m_recordingManager.stopRecording();
        setLastError(firstFrameResult.errorMessage);
        emit recordingChanged();
        emit recordingResult(false, result.filePath, firstFrameResult.errorMessage);
        return false;
    }

    m_recordingPath = result.filePath;
    emit recordingPathChanged();
    emit recordingChanged();
    emit recordingResult(true, result.filePath, QString());
    return true;
}

bool StreamSession::stopRecording()
{
    return stopRecordingInternal(true);
}

bool StreamSession::loadAiModel(const QUrl &modelUrl)
{
    const QString modelPath = modelUrl.isLocalFile() ? modelUrl.toLocalFile() : modelUrl.toString();
    const QString normalizedPath = QDir::toNativeSeparators(modelPath.trimmed());
    if (normalizedPath.isEmpty()) {
        const QString error = QStringLiteral("请选择 ONNX 模型文件。");
        setAiStatus(error);
        setLastError(error);
        return false;
    }

    if (!m_objectDetectionWorker || !m_objectDetectionThread || !m_objectDetectionThread->isRunning()) {
        const QString error = QStringLiteral("AI 检测线程不可用。");
        setAiStatus(error);
        setLastError(error);
        return false;
    }

    if (m_aiModelLoading.exchange(true)) {
        setAiStatus(QStringLiteral("AI 模型正在加载..."));
        return false;
    }

    m_aiModelReady.store(false);
    m_aiModelPath = normalizedPath;
    clearObjectDetectionBoxes();
    emit aiModelPathChanged();
    emit aiModelReadyChanged();
    emit aiModelLoadingChanged();
    setAiStatus(QStringLiteral("AI 模型正在加载..."));

    ObjectDetectionWorker *worker = m_objectDetectionWorker;
    const double threshold = m_aiConfidenceThreshold.load();
    const bool invoked = QMetaObject::invokeMethod(worker,
                                                   [this, worker, normalizedPath, threshold]() {
                                                       const bool success = worker->loadModel(normalizedPath, threshold);
                                                       const QString error = worker->lastError();
                                                       QMetaObject::invokeMethod(this,
                                                                                 [this, normalizedPath, success, error]() {
                                                                                     if (m_aiModelPath != normalizedPath) {
                                                                                         return;
                                                                                     }

                                                                                     m_aiModelLoading.store(false);
                                                                                     m_aiModelReady.store(success);
                                                                                     emit aiModelLoadingChanged();
                                                                                     emit aiModelReadyChanged();

                                                                                     if (success) {
                                                                                         setAiStatus(QStringLiteral("AI 模型已加载。"));
                                                                                         return;
                                                                                     }

                                                                                     const QString message = error.trimmed().isEmpty()
                                                                                                                 ? QStringLiteral("AI 模型加载失败。")
                                                                                                                 : error;
                                                                                     setAiStatus(message);
                                                                                     setLastError(message);
                                                                                     clearObjectDetectionBoxes();
                                                                                 },
                                                                                 Qt::QueuedConnection);
                                                   },
                                                   Qt::QueuedConnection);
    if (!invoked) {
        m_aiModelLoading.store(false);
        emit aiModelLoadingChanged();
        const QString error = QStringLiteral("提交 AI 模型加载任务失败。");
        setAiStatus(error);
        setLastError(error);
        return false;
    }

    return true;
}

int StreamSession::addRoiRect(double x, double y, double width, double height)
{
    const QRectF rect = normalizedRoiRect(x, y, width, height);
    if (rect.width() < 0.005 || rect.height() < 0.005) {
        return -1;
    }

    RoiRule rule;
    rule.sourceId = m_sourceId > 0 ? m_sourceId : sourceIdFromStreamId(m_streamId);
    {
        QMutexLocker locker(&m_roiRulesMutex);
        rule.ruleName = QStringLiteral("ROI %1").arg(m_nextRoiRuleIndex);
    }
    rule.rect = rect;
    rule.enabled = true;
    rule.cooldownSeconds = 10;

    if (m_roiRuleRepository && !m_roiRuleRepository->add(&rule)) {
        setLastError(m_roiRuleRepository->lastError());
        return -1;
    }

    int index = -1;
    {
        QMutexLocker locker(&m_roiRulesMutex);
        m_roiRules.append(rule);
        refreshNextRoiRuleIndexLocked();
        index = m_roiRules.size() - 1;
    }
    emit roiRulesChanged();
    return index;
}

bool StreamSession::removeRoiRule(int index)
{
    RoiRule rule;
    {
        QMutexLocker locker(&m_roiRulesMutex);
        if (index < 0 || index >= m_roiRules.size()) {
            return false;
        }
        rule = m_roiRules.at(index);
    }

    if (m_roiRuleRepository && rule.id > 0 && !m_roiRuleRepository->remove(rule.id)) {
        setLastError(m_roiRuleRepository->lastError());
        return false;
    }

    {
        QMutexLocker locker(&m_roiRulesMutex);
        if (index >= 0 && index < m_roiRules.size()
            && (rule.id <= 0 || m_roiRules.at(index).id == rule.id)) {
            m_roiRules.removeAt(index);
        } else {
            auto it = std::find_if(m_roiRules.begin(), m_roiRules.end(), [rule](const RoiRule &item) {
                return item.id > 0 && item.id == rule.id;
            });
            if (it == m_roiRules.end()) {
                return false;
            }
            m_roiRules.erase(it);
        }
        refreshNextRoiRuleIndexLocked();
    }
    emit roiRulesChanged();
    return true;
}

void StreamSession::clearRoiRules()
{
    if (m_roiRuleRepository
        && !m_roiRuleRepository->removeBySource(m_sourceId > 0 ? m_sourceId
                                                                : sourceIdFromStreamId(m_streamId))) {
        setLastError(m_roiRuleRepository->lastError());
        return;
    }

    clearRoiRulesInternal();
}

void StreamSession::runDecodeLoop(VideoSource source, quint64 generation)
{
    const bool reconnectEnabledForSource = source.type == VideoSourceType::Rtsp
                                           || source.type == VideoSourceType::Http;
    qint64 frameIndex = 0;
    int reconnectAttempt = 0;

    const auto createDecoder = [this, reconnectEnabledForSource]() -> std::unique_ptr<IVideoDecoder> {
        if (reconnectEnabledForSource) {
            auto decoder = std::make_unique<FFmpegVideoDecoder>();
            decoder->setInterruptFlag(&m_stopRequested);
            return decoder;
        }
        return std::make_unique<OpenCVVideoDecoder>();
    };

    const auto finishWithStatus = [this, generation](VideoSourceStatus status, const QString &error) {
        if (m_runGeneration.load() != generation) {
            return;
        }
        m_stopRequested.store(true);
        const quint64 terminalGeneration = m_runGeneration.fetch_add(1) + 1;
        m_frameQueue.close();
        m_activeObjectDetectionTaskId.store(0);
        m_objectDetectionBusy.store(false);
        QMetaObject::invokeMethod(this, [this, terminalGeneration, status, error]() {
            if (m_runGeneration.load() != terminalGeneration) {
                return;
            }
            setLastError(error);
            setStatus(status);
            setFps(0.0);
            setReconnectAttempt(0);
            m_currentFps.store(0.0);
            m_decodeTimeMs.store(0.0);
            stopRecordingInternal(true);
            clearDetectionBoxes();
            clearObjectDetectionBoxes();
        }, Qt::QueuedConnection);
    };

    const auto publishReconnecting = [this, generation](int attempt, const QString &error) {
        QMetaObject::invokeMethod(this, [this, generation, attempt, error]() {
            if (m_runGeneration.load() != generation) {
                return;
            }
            setLastError(error);
            setStatus(VideoSourceStatus::Reconnecting);
            setFps(0.0);
            setReconnectAttempt(attempt);
            m_currentFps.store(0.0);
            m_decodeTimeMs.store(0.0);
            clearDetectionBoxes();
            clearObjectDetectionBoxes();
        }, Qt::QueuedConnection);
    };

    const auto scheduleReconnect = [this,
                                    &reconnectAttempt,
                                    reconnectEnabledForSource,
                                    &publishReconnecting,
                                    &finishWithStatus](const QString &error) -> bool {
        if (!reconnectEnabledForSource || m_stopRequested.load()) {
            return false;
        }

        const int maxAttempts = m_maxReconnectAttempts.load();
        if (maxAttempts <= 0 || reconnectAttempt >= maxAttempts) {
            finishWithStatus(VideoSourceStatus::Error,
                             error.isEmpty() ? QStringLiteral("网络视频源重连失败。") : error);
            return false;
        }

        ++reconnectAttempt;
        publishReconnecting(reconnectAttempt, error);
        return waitForReconnectDelay();
    };

    while (!m_stopRequested.load()) {
        std::unique_ptr<IVideoDecoder> decoder = createDecoder();
        if (!decoder->open(source)) {
            const QString error = decoder->lastError();
            decoder->close();
            if (scheduleReconnect(error)) {
                continue;
            }
            if (!m_stopRequested.load() && !reconnectEnabledForSource) {
                finishWithStatus(VideoSourceStatus::Error, error);
            }
            break;
        }

        const double sourceFps = decoder->frameRate() > 0.0 ? decoder->frameRate() : 30.0;
        m_sourceFrameRate.store(sourceFps);
        const int frameDelayMs = std::max(1, static_cast<int>(1000.0 / sourceFps));
        QElapsedTimer fpsTimer;
        fpsTimer.start();
        int framesInWindow = 0;
        double currentFps = sourceFps;
        bool reconnectRequested = false;

        QMetaObject::invokeMethod(this, [this, generation]() {
            if (m_runGeneration.load() != generation) {
                return;
            }
            setStatus(VideoSourceStatus::Playing);
        }, Qt::QueuedConnection);

        while (!m_stopRequested.load()) {
            if (m_pauseRequested.load()) {
                QThread::msleep(20);
                continue;
            }

            QElapsedTimer frameTimer;
            frameTimer.start();

            QImage image;
            if (!decoder->readFrame(image)) {
                const bool ended = decoder->isEndOfStream();
                const QString error = decoder->lastError();
                decoder->close();

                if (reconnectEnabledForSource && !ended && !error.isEmpty()) {
                    m_motionDetectorResetRequested.store(true);
                    reconnectRequested = scheduleReconnect(error);
                    break;
                }

                finishWithStatus((!ended && !error.isEmpty()) ? VideoSourceStatus::Error
                                                              : VideoSourceStatus::Ended,
                                 (!ended && !error.isEmpty()) ? error : QString());
                break;
            }

            if (reconnectAttempt > 0) {
                reconnectAttempt = 0;
                QMetaObject::invokeMethod(this, [this, generation]() {
                    if (m_runGeneration.load() != generation) {
                        return;
                    }
                    setLastError(QString());
                    setReconnectAttempt(0);
                    setStatus(VideoSourceStatus::Playing);
                }, Qt::QueuedConnection);
            }

            m_decodeTimeMs.store(static_cast<double>(frameTimer.nsecsElapsed()) / 1000000.0);

            ++framesInWindow;
            ++frameIndex;
            const qint64 elapsedMs = fpsTimer.elapsed();
            if (elapsedMs >= 1000) {
                currentFps = framesInWindow * 1000.0 / elapsedMs;
                framesInWindow = 0;
                fpsTimer.restart();
            }
            m_currentFps.store(currentFps);

            if (m_motionDetectorResetRequested.exchange(false)) {
                m_motionDetector.reset();
            }

            if (m_motionDetectionEnabled.load()) {
                const DetectionResult detectionResult = m_motionDetector.analyze(image,
                                                                                 m_streamId,
                                                                                 m_minTargetArea.load());
                const QSize imageSize = image.size();
                if (m_alarmManager && !detectionResult.boxes.isEmpty()) {
                    const QString sourceName = m_sourceName.trimmed().isEmpty()
                                                    ? sourceNameForSource(source.type, source.displayUrl.isEmpty()
                                                                             ? source.url
                                                                             : source.displayUrl,
                                                                           m_streamId)
                                                    : m_sourceName;
                    emit motionDetected(m_streamId,
                                        source.id > 0 ? source.id : sourceIdFromStreamId(m_streamId),
                                        sourceName,
                                        image,
                                        detectionResult.boxes.size(),
                                        detectionResult.timestamp);

                    const QVector<RoiRule> roiRules = roiRulesSnapshot();
                    const QVector<IntrusionMatch> matches = m_intrusionDetector.detect(detectionResult,
                                                                                       imageSize,
                                                                                       roiRules);
                    for (const IntrusionMatch &match : matches) {
                        emit intrusionDetected(m_streamId,
                                               source.id > 0 ? source.id : sourceIdFromStreamId(m_streamId),
                                               sourceName,
                                               match.rule.id,
                                               match.rule.ruleName,
                                               match.rule.cooldownSeconds,
                                               image,
                                               detectionResult.boxes.size(),
                                               match.timestamp);
                    }
                }

                QMetaObject::invokeMethod(this, [this, generation, detectionResult, imageSize]() {
                    if (m_runGeneration.load() != generation) {
                        return;
                    }
                    publishDetectionResult(detectionResult, imageSize);
                }, Qt::QueuedConnection);
            }

            scheduleObjectDetection(image, source, generation, frameIndex);
            recordFrameIfNeeded(image, generation);
            m_frameQueue.push(Frame::fromImage(image, frameIndex));

            const qint64 decodeElapsedMs = frameTimer.elapsed();
            if (decodeElapsedMs < frameDelayMs) {
                QThread::msleep(static_cast<unsigned long>(frameDelayMs - decodeElapsedMs));
            } else {
                QThread::msleep(kFallbackFrameDelayMs / 3);
            }
        }

        decoder->close();
        if (!reconnectRequested) {
            break;
        }
    }
}

void StreamSession::runFrameDispatchLoop(quint64 generation)
{
    while (!m_stopRequested.load()) {
        Frame frame;
        if (!m_frameQueue.pop(frame)) {
            break;
        }

        const double currentFps = m_currentFps.load();
        QMetaObject::invokeMethod(this, [this, generation, frame, currentFps]() {
            if (m_runGeneration.load() != generation) {
                return;
            }
            publishFrame(frame, currentFps);
        }, Qt::QueuedConnection);
    }
}

void StreamSession::setStatus(VideoSourceStatus status)
{
    if (m_status == status) {
        return;
    }

    m_status = status;
    emit statusChanged();
}

void StreamSession::setFps(double fps)
{
    if (qFuzzyCompare(m_fps, fps)) {
        return;
    }

    m_fps = fps;
    emit fpsChanged();
}

void StreamSession::setLastError(const QString &lastError)
{
    if (m_lastError == lastError) {
        return;
    }

    m_lastError = lastError;
    emit lastErrorChanged();
}

void StreamSession::setAiStatus(const QString &status)
{
    if (m_aiStatus == status) {
        return;
    }

    m_aiStatus = status;
    emit aiStatusChanged();
}

void StreamSession::setReconnectAttempt(int attempt)
{
    const int normalizedAttempt = std::max(0, attempt);
    if (m_reconnectAttempt.load() == normalizedAttempt) {
        return;
    }

    m_reconnectAttempt.store(normalizedAttempt);
    emit reconnectAttemptChanged();
}

void StreamSession::recordFrameIfNeeded(const QImage &image, quint64 generation)
{
    if (!m_recordingManager.isRecording()) {
        return;
    }

    const RecordingResult result = m_recordingManager.writeFrame(image);
    if (result.success) {
        return;
    }

    m_recordingManager.stopRecording();
    QMetaObject::invokeMethod(this, [this, generation, result]() {
        if (m_runGeneration.load() != generation) {
            return;
        }
        setLastError(result.errorMessage);
        emit recordingChanged();
        emit recordingResult(false, result.filePath, result.errorMessage);
    }, Qt::QueuedConnection);
}

bool StreamSession::stopRecordingInternal(bool notify)
{
    const bool wasRecording = m_recordingManager.isRecording();
    const RecordingResult result = m_recordingManager.stopRecording();

    if (!result.filePath.isEmpty() && m_recordingPath != result.filePath) {
        m_recordingPath = result.filePath;
        emit recordingPathChanged();
    }

    if (wasRecording) {
        emit recordingChanged();
    }

    if (!result.success) {
        setLastError(result.errorMessage);
    }

    if (notify && wasRecording) {
        emit recordingResult(result.success, result.filePath, result.errorMessage);
    }

    return result.success;
}

void StreamSession::scheduleObjectDetection(const QImage &image,
                                            const VideoSource &source,
                                            quint64 generation,
                                            qint64 frameIndex)
{
    if (!m_aiDetectionEnabled.load()
        || !m_aiModelReady.load()
        || m_aiModelLoading.load()
        || image.isNull()
        || !m_objectDetectionWorker
        || !m_objectDetectionThread
        || !m_objectDetectionThread->isRunning()) {
        return;
    }

    const int interval = std::max(1, m_aiInferenceInterval.load());
    if (frameIndex % interval != 0) {
        return;
    }

    if (m_objectDetectionBusy.exchange(true)) {
        return;
    }

    ObjectDetectionWorker *worker = m_objectDetectionWorker;
    const QImage frame = image.copy();
    const QSize frameSize = frame.size();
    const QString sourceKey = m_streamId;
    const int sourceId = source.id > 0 ? source.id : sourceIdFromStreamId(m_streamId);
    const QString sourceName = m_sourceName.trimmed().isEmpty()
                                   ? sourceNameForSource(source.type,
                                                         source.displayUrl.isEmpty() ? source.url : source.displayUrl,
                                                         m_streamId)
                                   : m_sourceName;
    const double threshold = m_aiConfidenceThreshold.load();
    const quint64 taskId = m_nextObjectDetectionTaskId.fetch_add(1) + 1;
    m_activeObjectDetectionTaskId.store(taskId);

    const bool invoked = QMetaObject::invokeMethod(worker,
                                                   [this,
                                                    worker,
                                                    frame,
                                                    frameSize,
                                                    sourceKey,
                                                    sourceId,
                                                    sourceName,
                                                    threshold,
                                                    generation,
                                                    taskId]() {
                                                       const DetectionResult result = worker->detect(frame,
                                                                                                     sourceKey,
                                                                                                     threshold);
                                                       const QString error = worker->lastError();
                                                       QMetaObject::invokeMethod(this,
                                                                                 [this,
                                                                                  frame,
                                                                                  frameSize,
                                                                                  sourceKey,
                                                                                  sourceId,
                                                                                  sourceName,
                                                                                  result,
                                                                                  error,
                                                                                  generation,
                                                                                  taskId]() {
                                                                                     if (m_activeObjectDetectionTaskId.load() != taskId) {
                                                                                         return;
                                                                                     }
                                                                                     m_objectDetectionBusy.store(false);
                                                                                     if (m_runGeneration.load() != generation) {
                                                                                         return;
                                                                                     }

                                                                                     if (!m_aiDetectionEnabled.load()) {
                                                                                         clearObjectDetectionBoxes();
                                                                                         return;
                                                                                     }

                                                                                     if (!error.trimmed().isEmpty()
                                                                                         && result.detections.isEmpty()) {
                                                                                         setAiStatus(error);
                                                                                     } else if (!m_aiModelLoading.load()
                                                                                                && m_aiModelReady.load()) {
                                                                                         setAiStatus(QStringLiteral("AI 检测运行中。"));
                                                                                     }

                                                                                     publishObjectDetectionResult(result, frameSize);

                                                                                     if (!m_alarmManager || result.detections.isEmpty()) {
                                                                                         return;
                                                                                     }

                                                                                     int personCount = 0;
                                                                                     int vehicleCount = 0;
                                                                                     double bestPersonConfidence = 0.0;
                                                                                     double bestVehicleConfidence = 0.0;
                                                                                     for (const DetectionBox &detection : result.detections) {
                                                                                         if (detection.eventType == QStringLiteral("PersonDetected")) {
                                                                                             ++personCount;
                                                                                             bestPersonConfidence = std::max(bestPersonConfidence,
                                                                                                                             static_cast<double>(detection.confidence));
                                                                                         } else if (detection.eventType == QStringLiteral("VehicleDetected")) {
                                                                                             ++vehicleCount;
                                                                                             bestVehicleConfidence = std::max(bestVehicleConfidence,
                                                                                                                              static_cast<double>(detection.confidence));
                                                                                         }
                                                                                     }

                                                                                     if (personCount > 0) {
                                                                                         emit objectDetected(sourceKey,
                                                                                                             sourceId,
                                                                                                             sourceName,
                                                                                                             QStringLiteral("PersonDetected"),
                                                                                                             QStringLiteral("person"),
                                                                                                             bestPersonConfidence,
                                                                                                             frame,
                                                                                                             personCount,
                                                                                                             result.timestamp);
                                                                                     }

                                                                                     if (vehicleCount > 0) {
                                                                                         emit objectDetected(sourceKey,
                                                                                                             sourceId,
                                                                                                             sourceName,
                                                                                                             QStringLiteral("VehicleDetected"),
                                                                                                             QStringLiteral("vehicle"),
                                                                                                             bestVehicleConfidence,
                                                                                                             frame,
                                                                                                             vehicleCount,
                                                                                                             result.timestamp);
                                                                                     }
                                                                                 },
                                                                                 Qt::QueuedConnection);
                                                   },
                                                   Qt::QueuedConnection);

    if (!invoked && m_activeObjectDetectionTaskId.load() == taskId) {
        m_activeObjectDetectionTaskId.store(0);
        m_objectDetectionBusy.store(false);
        setAiStatus(QStringLiteral("提交 AI 推理任务失败。"));
    }
}

void StreamSession::publishFrame(const Frame &frame, double fps)
{
    if (!m_frameProvider || frame.image.isNull()) {
        return;
    }

    m_frameProvider->setFrame(m_streamId, frame.image);
    ++m_frameSerial;
    m_frameSource = QStringLiteral("image://videoFrame/%1?serial=%2").arg(m_streamId).arg(m_frameSerial);
    emit frameSourceChanged();
    setFps(fps);
}

void StreamSession::publishDetectionResult(const DetectionResult &result, const QSize &frameSize)
{
    if (!m_motionDetectionEnabled.load() || !frameSize.isValid() || frameSize.isEmpty()) {
        clearDetectionBoxes();
        return;
    }

    QVariantList boxes;
    const QRect frameRect(0, 0, frameSize.width(), frameSize.height());
    for (const QRect &box : result.boxes) {
        const QRect boundedBox = box.intersected(frameRect);
        if (boundedBox.isEmpty()) {
            continue;
        }

        QVariantMap item;
        item.insert(QStringLiteral("sourceId"), result.sourceId);
        item.insert(QStringLiteral("timestamp"), result.timestamp);
        item.insert(QStringLiteral("x"), static_cast<double>(boundedBox.x()) / frameSize.width());
        item.insert(QStringLiteral("y"), static_cast<double>(boundedBox.y()) / frameSize.height());
        item.insert(QStringLiteral("width"), static_cast<double>(boundedBox.width()) / frameSize.width());
        item.insert(QStringLiteral("height"), static_cast<double>(boundedBox.height()) / frameSize.height());
        boxes.append(item);
    }

    if (m_detectionBoxes == boxes) {
        return;
    }

    m_detectionBoxes = boxes;
    emit detectionBoxesChanged();
}

void StreamSession::publishObjectDetectionResult(const DetectionResult &result, const QSize &frameSize)
{
    if (!m_aiDetectionEnabled.load() || !frameSize.isValid() || frameSize.isEmpty()) {
        clearObjectDetectionBoxes();
        return;
    }

    QVariantList boxes;
    const QRect frameRect(0, 0, frameSize.width(), frameSize.height());
    for (const DetectionBox &detection : result.detections) {
        const QRect boundedBox = detection.box.intersected(frameRect);
        if (boundedBox.isEmpty()) {
            continue;
        }

        QVariantMap item;
        item.insert(QStringLiteral("sourceId"), result.sourceId);
        item.insert(QStringLiteral("timestamp"), result.timestamp);
        item.insert(QStringLiteral("x"), static_cast<double>(boundedBox.x()) / frameSize.width());
        item.insert(QStringLiteral("y"), static_cast<double>(boundedBox.y()) / frameSize.height());
        item.insert(QStringLiteral("width"), static_cast<double>(boundedBox.width()) / frameSize.width());
        item.insert(QStringLiteral("height"), static_cast<double>(boundedBox.height()) / frameSize.height());
        item.insert(QStringLiteral("classId"), detection.classId);
        item.insert(QStringLiteral("className"), detection.className);
        item.insert(QStringLiteral("eventType"), detection.eventType);
        item.insert(QStringLiteral("confidence"), detection.confidence);
        boxes.append(item);
    }

    if (m_objectDetectionBoxes == boxes) {
        return;
    }

    m_objectDetectionBoxes = boxes;
    emit objectDetectionBoxesChanged();
}

void StreamSession::clearFrame()
{
    if (m_frameProvider) {
        m_frameProvider->clearFrame(m_streamId);
    }

    if (m_frameSource.isEmpty()) {
        return;
    }

    m_frameSource.clear();
    emit frameSourceChanged();
}

void StreamSession::clearDetectionBoxes()
{
    if (m_detectionBoxes.isEmpty()) {
        return;
    }

    m_detectionBoxes.clear();
    emit detectionBoxesChanged();
}

void StreamSession::clearObjectDetectionBoxes()
{
    if (m_objectDetectionBoxes.isEmpty()) {
        return;
    }

    m_objectDetectionBoxes.clear();
    emit objectDetectionBoxesChanged();
}

void StreamSession::clearRoiRulesInternal()
{
    bool changed = false;
    {
        QMutexLocker locker(&m_roiRulesMutex);
        if (m_roiRules.isEmpty()) {
            m_nextRoiRuleIndex = 1;
            return;
        }

        m_roiRules.clear();
        m_nextRoiRuleIndex = 1;
        changed = true;
    }

    if (changed) {
        emit roiRulesChanged();
    }
}

void StreamSession::loadRoiRulesFromRepository()
{
    if (!m_roiRuleRepository) {
        clearRoiRulesInternal();
        return;
    }

    const QVector<RoiRule> rules = m_roiRuleRepository->loadBySource(m_sourceId > 0 ? m_sourceId
                                                                     : sourceIdFromStreamId(m_streamId));
    {
        QMutexLocker locker(&m_roiRulesMutex);
        m_roiRules = rules;
        refreshNextRoiRuleIndexLocked();
    }
    emit roiRulesChanged();
}

QVector<RoiRule> StreamSession::roiRulesSnapshot() const
{
    QMutexLocker locker(&m_roiRulesMutex);
    return m_roiRules;
}

void StreamSession::refreshNextRoiRuleIndexLocked()
{
    m_nextRoiRuleIndex = m_roiRules.size() + 1;
    for (const RoiRule &rule : m_roiRules) {
        if (!rule.ruleName.startsWith(QStringLiteral("ROI "))) {
            continue;
        }

        bool ok = false;
        const int ruleNumber = rule.ruleName.mid(4).toInt(&ok);
        if (ok && ruleNumber >= m_nextRoiRuleIndex) {
            m_nextRoiRuleIndex = ruleNumber + 1;
        }
    }
}

bool StreamSession::waitForReconnectDelay() const
{
    int waitedMs = 0;
    const int delayMs = std::max(250, m_reconnectIntervalMs.load());
    while (!m_stopRequested.load() && waitedMs < delayMs) {
        QThread::msleep(kReconnectPollMs);
        waitedMs += kReconnectPollMs;
    }
    return !m_stopRequested.load();
}

void StreamSession::stopWorker()
{
    ++m_runGeneration;
    m_stopRequested.store(true);
    m_pauseRequested.store(false);
    m_motionDetectorResetRequested.store(true);
    m_currentFps.store(0.0);
    m_decodeTimeMs.store(0.0);
    m_frameQueue.close();

    QThread *decodeThread = m_decodeThread;
    if (decodeThread && decodeThread->isRunning()) {
        decodeThread->wait();
    }

    QThread *dispatchThread = m_dispatchThread;
    if (dispatchThread && dispatchThread->isRunning()) {
        dispatchThread->wait();
    }

    m_decodeThread = nullptr;
    m_dispatchThread = nullptr;
    m_frameQueue.clear();
    stopRecordingInternal(false);
    m_activeObjectDetectionTaskId.store(0);
    m_objectDetectionBusy.store(false);
}

void StreamSession::waitForSnapshotWorkers()
{
    const QVector<QThread *> workers = m_snapshotThreads;
    for (QThread *worker : workers) {
        if (!worker) {
            continue;
        }

        if (worker->isRunning()) {
            worker->wait();
        }

        worker->deleteLater();
    }

    m_snapshotThreads.clear();
}

void StreamSession::stopObjectDetectionWorker()
{
    if (!m_objectDetectionThread) {
        m_objectDetectionWorker = nullptr;
        return;
    }

    ObjectDetectionWorker *worker = m_objectDetectionWorker;
    m_objectDetectionWorker = nullptr;
    if (worker && m_objectDetectionThread->isRunning()) {
        QMetaObject::invokeMethod(worker, [worker]() {
            delete worker;
        }, Qt::BlockingQueuedConnection);
    } else {
        delete worker;
    }

    if (m_objectDetectionThread->isRunning()) {
        m_objectDetectionThread->quit();
        m_objectDetectionThread->wait();
    }
    delete m_objectDetectionThread;
    m_objectDetectionThread = nullptr;
    m_activeObjectDetectionTaskId.store(0);
    m_objectDetectionBusy.store(false);
}
