#pragma once

#include <QImage>
#include <QString>

struct SnapshotResult {
    bool success = false;
    QString filePath;
    QString errorMessage;
};

class SnapshotManager
{
public:
    explicit SnapshotManager(QString baseDirectory = QStringLiteral("data/snapshots"));

    SnapshotResult saveSnapshot(const QImage &image,
                                const QString &sourceId,
                                const QString &eventType,
                                const QString &format) const;

private:
    QString dayDirectoryPath() const;
    QString makeFileName(const QString &sourceId, const QString &eventType, const QString &format) const;
    QString normalizedFormat(const QString &format) const;
    QString sanitizedToken(const QString &value, const QString &fallback) const;

    QString m_baseDirectory;
};
