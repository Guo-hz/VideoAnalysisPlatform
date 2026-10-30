#pragma once

#include <QObject>
#include <QString>

class StartupController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString errorMessage READ errorMessage CONSTANT)
    Q_PROPERTY(QString dataDirectory READ dataDirectory CONSTANT)

public:
    StartupController(QString errorMessage, QString dataDirectory, QObject *parent = nullptr);

    QString errorMessage() const;
    QString dataDirectory() const;

    Q_INVOKABLE void retry();
    Q_INVOKABLE void openDataDirectory();
    Q_INVOKABLE void quit();

private:
    QString m_errorMessage;
    QString m_dataDirectory;
};
