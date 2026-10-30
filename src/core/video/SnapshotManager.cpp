#include "SnapshotManager.h"

#include <QDate>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QImageWriter>
#include <QTime>
#include <QUuid>

SnapshotManager::SnapshotManager(QString baseDirectory)
    : m_baseDirectory(QDir::cleanPath(baseDirectory))
{
}

SnapshotResult SnapshotManager::saveSnapshot(const QImage &image,
                                             const QString &sourceId,
                                             const QString &eventType,
                                             const QString &format) const
{
    SnapshotResult result;
    if (image.isNull()) {
        result.errorMessage = QStringLiteral("当前没有可保存的视频画面。");
        return result;
    }

    const QString imageFormat = normalizedFormat(format);
    if (imageFormat.isEmpty()) {
        result.errorMessage = QStringLiteral("截图格式仅支持 jpg 或 png。");
        return result;
    }

    const QString directoryPath = dayDirectoryPath();
    QDir directory(directoryPath);
    if (!directory.exists() && !QDir().mkpath(directoryPath)) {
        result.errorMessage = QStringLiteral("创建截图目录失败：%1").arg(directoryPath);
        return result;
    }

    const QString filePath = directory.absoluteFilePath(makeFileName(sourceId, eventType, imageFormat));
    const QString temporaryPath = filePath + QStringLiteral(".tmp-")
                                  + QUuid::createUuid().toString(QUuid::WithoutBraces);
    QImageWriter writer(temporaryPath, imageFormat.toLatin1());
    if (imageFormat == QStringLiteral("jpg")) {
        writer.setQuality(92);
    }

    if (!writer.write(image)) {
        result.errorMessage = QStringLiteral("保存截图失败：%1").arg(writer.errorString());
        QFile::remove(temporaryPath);
        return result;
    }

    if (!QFile::rename(temporaryPath, filePath)) {
        QFile::remove(temporaryPath);
        result.errorMessage = QStringLiteral("保存截图失败：无法提交截图文件。");
        return result;
    }

    result.success = true;
    result.filePath = QFileInfo(filePath).absoluteFilePath();
    return result;
}

QString SnapshotManager::dayDirectoryPath() const
{
    const QString datePart = QDate::currentDate().toString(QStringLiteral("yyyy-MM-dd"));
    return QDir(m_baseDirectory).filePath(datePart);
}

QString SnapshotManager::makeFileName(const QString &sourceId, const QString &eventType, const QString &format) const
{
    const QString safeSourceId = sanitizedToken(sourceId, QStringLiteral("source"));
    const QString safeEventType = sanitizedToken(eventType, QStringLiteral("manual"));
    const QString timePart = QTime::currentTime().toString(QStringLiteral("HHmmss_zzz"));
    const QString uniquePart = QUuid::createUuid().toString(QUuid::WithoutBraces);
    return QStringLiteral("%1_%2_%3_%4.%5")
        .arg(safeSourceId, timePart, safeEventType, uniquePart, format);
}

QString SnapshotManager::normalizedFormat(const QString &format) const
{
    const QString value = format.trimmed().toLower();
    if (value == QStringLiteral("jpg") || value == QStringLiteral("jpeg")) {
        return QStringLiteral("jpg");
    }

    if (value == QStringLiteral("png")) {
        return QStringLiteral("png");
    }

    return {};
}

QString SnapshotManager::sanitizedToken(const QString &value, const QString &fallback) const
{
    QString token = value.trimmed();
    if (token.isEmpty()) {
        token = fallback;
    }

    for (QChar &character : token) {
        if (!character.isLetterOrNumber() && character != QLatin1Char('_') && character != QLatin1Char('-')) {
            character = QLatin1Char('_');
        }
    }

    return token;
}
