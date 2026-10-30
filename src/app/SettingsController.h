#pragma once

#include <QObject>
#include <QString>

class AppPaths;

class SettingsController final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString databasePath READ databasePath CONSTANT)
    Q_PROPERTY(QString snapshotDirectory READ snapshotDirectory CONSTANT)
    Q_PROPERTY(QString recordingDirectory READ recordingDirectory CONSTANT)
    Q_PROPERTY(QString modelDirectory READ modelDirectory CONSTANT)
    Q_PROPERTY(QString logDirectory READ logDirectory CONSTANT)
    Q_PROPERTY(int diskWarningThresholdPercent READ diskWarningThresholdPercent WRITE setDiskWarningThresholdPercent NOTIFY diskWarningThresholdPercentChanged)

public:
    explicit SettingsController(const AppPaths *paths, QObject *parent = nullptr);

    QString databasePath() const;
    QString snapshotDirectory() const;
    QString recordingDirectory() const;
    QString modelDirectory() const;
    QString logDirectory() const;
    int diskWarningThresholdPercent() const;
    void setDiskWarningThresholdPercent(int value);

    Q_INVOKABLE void openDirectory(const QString &path) const;
    Q_INVOKABLE bool isModelPresent(const QString &modelFileName) const;

signals:
    void diskWarningThresholdPercentChanged();

private:
    const AppPaths *m_paths = nullptr;
    int m_diskWarningThresholdPercent = 10;
};
