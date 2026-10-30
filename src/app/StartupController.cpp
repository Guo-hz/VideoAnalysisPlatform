#include "StartupController.h"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QProcess>
#include <QUrl>

#include <utility>

StartupController::StartupController(QString errorMessage,
                                     QString dataDirectory,
                                     QObject *parent)
    : QObject(parent)
    , m_errorMessage(std::move(errorMessage))
    , m_dataDirectory(std::move(dataDirectory))
{
}

QString StartupController::errorMessage() const
{
    return m_errorMessage;
}

QString StartupController::dataDirectory() const
{
    return m_dataDirectory;
}

void StartupController::retry()
{
    if (QProcess::startDetached(QCoreApplication::applicationFilePath(),
                                QCoreApplication::arguments().mid(1))) {
        QCoreApplication::quit();
    }
}

void StartupController::openDataDirectory()
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(m_dataDirectory));
}

void StartupController::quit()
{
    QCoreApplication::quit();
}
