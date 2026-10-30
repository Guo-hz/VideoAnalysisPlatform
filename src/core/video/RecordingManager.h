#pragma once

#include <QImage>
#include <QMutex>
#include <QSize>
#include <QString>

struct RecordingResult {
    bool success = false;
    QString filePath;
    QString errorMessage;
};

class RecordingManager
{
public:
    explicit RecordingManager(QString baseDirectory = QStringLiteral("data/records"));
    ~RecordingManager();

    RecordingResult startRecording(const QString &sourceId, const QSize &frameSize, double fps);
    RecordingResult writeFrame(const QImage &image);
    RecordingResult stopRecording();

    bool isRecording() const;
    QString currentFilePath() const;
    QString lastError() const;

private:
    QString dayDirectoryPath() const;
    QString makeFileName(const QString &sourceId) const;
    QString sanitizedToken(const QString &value, const QString &fallback) const;

    mutable QMutex m_mutex;
    QString m_baseDirectory;
    QString m_currentFilePath;
    QString m_lastError;
    QSize m_frameSize;
    double m_fps = 0.0;

#ifdef VIDEOANALYSIS_HAS_OPENCV
    class WriterHolder;
    WriterHolder *m_writerHolder = nullptr;
#endif
};
