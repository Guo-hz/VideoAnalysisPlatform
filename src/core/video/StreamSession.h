#pragma once

#include <atomic>

#include <QImage>
#include <QMutex>
#include <QObject>
#include <QSize>
#include <QString>
#include <QThread>
#include <QUrl>
#include <QVariantList>
#include <QVector>

#include "core/analysis/DetectionResult.h"
#include "core/analysis/IntrusionDetector.h"
#include "core/analysis/MotionDetector.h"
#include "core/analysis/OnnxObjectDetector.h"
#include "core/analysis/RoiRule.h"
#include "core/video/FrameQueue.h"
#include "core/video/RecordingManager.h"
#include "core/video/SnapshotManager.h"
#include "core/video/StreamPerformance.h"
#include "core/video/VideoSource.h"

class VideoFrameProvider;
class AlarmManager;
class RoiRuleRepository;
class ObjectDetectionWorker;

class StreamSession : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString streamId READ streamId CONSTANT)
    Q_PROPERTY(int sourceId READ sourceId NOTIFY sourceChanged)
    Q_PROPERTY(QString sourceName READ sourceName NOTIFY sourceChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(double fps READ fps NOTIFY fpsChanged)
    Q_PROPERTY(QString frameSource READ frameSource NOTIFY frameSourceChanged)
    Q_PROPERTY(QString sourcePath READ sourcePath NOTIFY sourcePathChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    Q_PROPERTY(QString lastSnapshotPath READ lastSnapshotPath NOTIFY lastSnapshotPathChanged)
    Q_PROPERTY(bool snapshotBusy READ snapshotBusy NOTIFY snapshotBusyChanged)
    Q_PROPERTY(bool recording READ recording NOTIFY recordingChanged)
    Q_PROPERTY(QString recordingPath READ recordingPath NOTIFY recordingPathChanged)
    Q_PROPERTY(bool motionDetectionEnabled READ motionDetectionEnabled WRITE setMotionDetectionEnabled NOTIFY motionDetectionEnabledChanged)
    Q_PROPERTY(int minTargetArea READ minTargetArea WRITE setMinTargetArea NOTIFY minTargetAreaChanged)
    Q_PROPERTY(bool aiDetectionEnabled READ aiDetectionEnabled WRITE setAiDetectionEnabled NOTIFY aiDetectionEnabledChanged)
    Q_PROPERTY(QString aiModelPath READ aiModelPath NOTIFY aiModelPathChanged)
    Q_PROPERTY(QString aiStatus READ aiStatus NOTIFY aiStatusChanged)
    Q_PROPERTY(bool aiModelReady READ aiModelReady NOTIFY aiModelReadyChanged)
    Q_PROPERTY(bool aiModelLoading READ aiModelLoading NOTIFY aiModelLoadingChanged)
    Q_PROPERTY(double aiConfidenceThreshold READ aiConfidenceThreshold WRITE setAiConfidenceThreshold NOTIFY aiConfidenceThresholdChanged)
    Q_PROPERTY(int aiInferenceInterval READ aiInferenceInterval WRITE setAiInferenceInterval NOTIFY aiInferenceIntervalChanged)
    Q_PROPERTY(int maxReconnectAttempts READ maxReconnectAttempts WRITE setMaxReconnectAttempts NOTIFY maxReconnectAttemptsChanged)
    Q_PROPERTY(int reconnectAttempt READ reconnectAttempt NOTIFY reconnectAttemptChanged)
    Q_PROPERTY(QVariantList detectionBoxes READ detectionBoxes NOTIFY detectionBoxesChanged)
    Q_PROPERTY(QVariantList objectDetectionBoxes READ objectDetectionBoxes NOTIFY objectDetectionBoxesChanged)
    Q_PROPERTY(QVariantList roiRules READ roiRules NOTIFY roiRulesChanged)

public:
    explicit StreamSession(QString streamId,
                           VideoFrameProvider *frameProvider,
                           AlarmManager *alarmManager = nullptr,
                           RoiRuleRepository *roiRuleRepository = nullptr,
                           QString snapshotDirectory = QString(),
                           QString recordingDirectory = QString(),
                           QObject *parent = nullptr);
    ~StreamSession() override;

    QString streamId() const;
    int sourceId() const;
    QString sourceName() const;
    QString status() const;
    double fps() const;
    QString frameSource() const;
    QImage currentFrame() const;
    QString sourcePath() const;
    QString lastError() const;
    QString lastSnapshotPath() const;
    bool snapshotBusy() const;
    bool recording() const;
    QString recordingPath() const;
    bool motionDetectionEnabled() const;
    void setMotionDetectionEnabled(bool enabled);
    int minTargetArea() const;
    void setMinTargetArea(int area);
    bool aiDetectionEnabled() const;
    void setAiDetectionEnabled(bool enabled);
    QString aiModelPath() const;
    QString aiStatus() const;
    bool aiModelReady() const;
    bool aiModelLoading() const;
    double aiConfidenceThreshold() const;
    void setAiConfidenceThreshold(double threshold);
    int aiInferenceInterval() const;
    void setAiInferenceInterval(int interval);
    int maxReconnectAttempts() const;
    void setMaxReconnectAttempts(int attempts);
    int reconnectAttempt() const;
    QVariantList detectionBoxes() const;
    QVariantList objectDetectionBoxes() const;
    QVariantList roiRules() const;
    StreamPerformance performanceSnapshot() const;

    Q_INVOKABLE bool openLocalFile(const QUrl &fileUrl);
    Q_INVOKABLE bool openRtspUrl(const QString &url);
    Q_INVOKABLE bool openHttpUrl(const QString &url);
    bool openSource(const VideoSource &source);
    Q_INVOKABLE void play();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void stop();
    Q_INVOKABLE bool takeSnapshot(const QString &format, const QString &eventType);
    Q_INVOKABLE bool startRecording();
    Q_INVOKABLE bool stopRecording();
    Q_INVOKABLE bool loadAiModel(const QUrl &modelUrl);
    Q_INVOKABLE int addRoiRect(double x, double y, double width, double height);
    Q_INVOKABLE bool removeRoiRule(int index);
    Q_INVOKABLE void clearRoiRules();

signals:
    void sourceChanged();
    void statusChanged();
    void fpsChanged();
    void frameSourceChanged();
    void sourcePathChanged();
    void lastErrorChanged();
    void lastSnapshotPathChanged();
    void snapshotBusyChanged();
    void recordingChanged();
    void recordingPathChanged();
    void motionDetectionEnabledChanged();
    void minTargetAreaChanged();
    void aiDetectionEnabledChanged();
    void aiModelPathChanged();
    void aiStatusChanged();
    void aiModelReadyChanged();
    void aiModelLoadingChanged();
    void aiConfidenceThresholdChanged();
    void aiInferenceIntervalChanged();
    void maxReconnectAttemptsChanged();
    void reconnectAttemptChanged();
    void detectionBoxesChanged();
    void objectDetectionBoxesChanged();
    void roiRulesChanged();
    void snapshotFinished(bool success, const QString &filePath, const QString &errorMessage);
    void recordingResult(bool success, const QString &filePath, const QString &errorMessage);
    void motionDetected(const QString &sourceKey,
                        int sourceId,
                        const QString &sourceName,
                        const QImage &frame,
                        int boxCount,
                        qint64 timestamp);
    void intrusionDetected(const QString &sourceKey,
                           int sourceId,
                           const QString &sourceName,
                           int ruleId,
                           const QString &ruleName,
                           int cooldownSeconds,
                           const QImage &frame,
                           int boxCount,
                           qint64 timestamp);
    void objectDetected(const QString &sourceKey,
                        int sourceId,
                        const QString &sourceName,
                        const QString &eventType,
                        const QString &className,
                        double confidence,
                        const QImage &frame,
                        int boxCount,
                        qint64 timestamp);

private:
    void runDecodeLoop(VideoSource source, quint64 generation);
    void runFrameDispatchLoop(quint64 generation);
    void setStatus(VideoSourceStatus status);
    void setFps(double fps);
    void setLastError(const QString &lastError);
    void setAiStatus(const QString &status);
    void setReconnectAttempt(int attempt);
    void recordFrameIfNeeded(const QImage &image, quint64 generation);
    bool stopRecordingInternal(bool notify);
    void scheduleObjectDetection(const QImage &image,
                                 const VideoSource &source,
                                 quint64 generation,
                                 qint64 frameIndex);
    void publishFrame(const Frame &frame, double fps);
    void publishDetectionResult(const DetectionResult &result, const QSize &frameSize);
    void publishObjectDetectionResult(const DetectionResult &result, const QSize &frameSize);
    void clearFrame();
    void clearDetectionBoxes();
    void clearObjectDetectionBoxes();
    void clearRoiRulesInternal();
    void loadRoiRulesFromRepository();
    QVector<RoiRule> roiRulesSnapshot() const;
    void refreshNextRoiRuleIndexLocked();
    bool waitForReconnectDelay() const;
    void stopWorker();
    void waitForSnapshotWorkers();
    void stopObjectDetectionWorker();

    VideoFrameProvider *m_frameProvider = nullptr;
    AlarmManager *m_alarmManager = nullptr;
    RoiRuleRepository *m_roiRuleRepository = nullptr;
    QThread *m_objectDetectionThread = nullptr;
    ObjectDetectionWorker *m_objectDetectionWorker = nullptr;
    QString m_streamId;
    int m_sourceId = 0;
    QString m_sourceName;
    QThread *m_decodeThread = nullptr;
    QThread *m_dispatchThread = nullptr;
    FrameQueue m_frameQueue;
    std::atomic_bool m_stopRequested = false;
    std::atomic_bool m_pauseRequested = false;
    std::atomic_bool m_motionDetectionEnabled = false;
    std::atomic_bool m_aiDetectionEnabled = false;
    std::atomic_bool m_aiModelReady = false;
    std::atomic_bool m_aiModelLoading = false;
    std::atomic_bool m_objectDetectionBusy = false;
    std::atomic_bool m_snapshotBusy = false;
    std::atomic<quint64> m_nextObjectDetectionTaskId = 0;
    std::atomic<quint64> m_activeObjectDetectionTaskId = 0;
    std::atomic_bool m_motionDetectorResetRequested = false;
    std::atomic<quint64> m_runGeneration = 0;
    std::atomic<int> m_minTargetArea = 500;
    std::atomic<int> m_aiInferenceInterval = 5;
    std::atomic<int> m_maxReconnectAttempts = 3;
    std::atomic<int> m_reconnectIntervalMs = 3000;
    std::atomic<int> m_reconnectAttempt = 0;
    std::atomic<double> m_currentFps = 0.0;
    std::atomic<double> m_decodeTimeMs = 0.0;
    std::atomic<double> m_sourceFrameRate = 25.0;
    std::atomic<double> m_aiConfidenceThreshold = 0.45;
    VideoSourceStatus m_status = VideoSourceStatus::Stopped;
    double m_fps = 0.0;
    QString m_frameSource;
    QString m_sourcePath;
    QString m_openUrl;
    VideoSourceType m_sourceType = VideoSourceType::LocalFile;
    QString m_lastError;
    QString m_lastSnapshotPath;
    QString m_recordingPath;
    QString m_aiModelPath;
    QString m_aiStatus;
    QVariantList m_detectionBoxes;
    QVariantList m_objectDetectionBoxes;
    mutable QMutex m_roiRulesMutex;
    QVector<RoiRule> m_roiRules;
    int m_nextRoiRuleIndex = 1;
    quint64 m_frameSerial = 0;
    MotionDetector m_motionDetector;
    IntrusionDetector m_intrusionDetector;
    SnapshotManager m_snapshotManager;
    RecordingManager m_recordingManager;
    QVector<QThread *> m_snapshotThreads;
};
