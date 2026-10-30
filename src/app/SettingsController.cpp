#include "SettingsController.h"

#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QUrl>

#include <algorithm>

#include "AppPaths.h"

namespace {
QString settingsKey()
{
    return QStringLiteral("settings/diskWarningThresholdPercent");
}
}

SettingsController::SettingsController(const AppPaths *paths, QObject *parent)
    : QObject(parent)
    , m_paths(paths)
{
    QSettings settings;
    m_diskWarningThresholdPercent = settings.value(settingsKey(), 10).toInt();
    setDiskWarningThresholdPercent(m_diskWarningThresholdPercent);
}

QString SettingsController::databasePath() const
{
    return m_paths ? m_paths->databasePath() : QString();
}

QString SettingsController::snapshotDirectory() const
{
    return m_paths ? m_paths->snapshotDirectory() : QString();
}

QString SettingsController::recordingDirectory() const
{
    return m_paths ? m_paths->recordingDirectory() : QString();
}

QString SettingsController::modelDirectory() const
{
    return m_paths ? m_paths->modelDirectory() : QString();
}

QString SettingsController::logDirectory() const
{
    return m_paths ? m_paths->logDirectory() : QString();
}

int SettingsController::diskWarningThresholdPercent() const
{
    return m_diskWarningThresholdPercent;
}

void SettingsController::setDiskWarningThresholdPercent(int value)
{
    const int normalized = std::clamp(value, 1, 50);
    if (m_diskWarningThresholdPercent == normalized) {
        return;
    }
    m_diskWarningThresholdPercent = normalized;
    QSettings settings;
    settings.setValue(settingsKey(), normalized);
    emit diskWarningThresholdPercentChanged();
}

void SettingsController::openDirectory(const QString &path) const
{
    const QFileInfo info(path);
    const QString directory = info.isDir() ? info.absoluteFilePath() : info.absolutePath();
    if (!directory.isEmpty()) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(directory));
    }
}

bool SettingsController::isModelPresent(const QString &modelFileName) const
{
    return !modelDirectory().isEmpty()
           && QFileInfo::exists(QDir(modelDirectory()).filePath(modelFileName));
}
