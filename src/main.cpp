#include <QCoreApplication>
#include <QDebug>
#include <QGuiApplication>
#include <QMetaType>
#include <QObject>
#include <QQmlApplicationEngine>
#include <QUrl>
#include <QVariant>
#include <QVariantMap>

#include "app/AppPaths.h"
#include "app/SettingsController.h"
#include "app/StartupController.h"
#include "core/alarm/AlarmManager.h"
#include "core/alarm/AlarmEvent.h"
#include "core/storage/AlarmEventRepository.h"
#include "core/storage/ChannelBindingRepository.h"
#include "core/storage/DatabaseManager.h"
#include "core/storage/RoiRuleRepository.h"
#include "core/storage/VideoSourceRepository.h"
#include "core/video/StreamSessionManager.h"
#include "models/AlarmEventModel.h"
#include "models/PerformanceModel.h"
#include "models/RecordingModel.h"
#include "models/VideoSourceModel.h"
#include "render/VideoFrameProvider.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("VideoAnalysisPlatform"));
    QCoreApplication::setApplicationName(QStringLiteral("VideoAnalysisPlatform"));
    qRegisterMetaType<AlarmEvent>("AlarmEvent");

    QQmlApplicationEngine engine;
    auto *videoFrameProvider = new VideoFrameProvider();
    engine.addImageProvider(QStringLiteral("videoFrame"), videoFrameProvider);

    AppPaths appPaths;
    QString startupError;
    if (!appPaths.prepare()) {
        startupError = appPaths.lastError();
    }

    DatabaseManager databaseManager;
    if (startupError.isEmpty() && !databaseManager.open(appPaths.databasePath())) {
        startupError = databaseManager.lastError();
    }

    if (!startupError.isEmpty()) {
        StartupController startupController(startupError, appPaths.rootDirectory());
        engine.setInitialProperties({
            {QStringLiteral("startupController"), QVariant::fromValue(static_cast<QObject *>(&startupController))},
        });
        const QUrl startupQmlUrl(QStringLiteral("qrc:/qt/qml/VideoAnalysisPlatform/qml/StartupError.qml"));
        QObject::connect(&engine,
                         &QQmlApplicationEngine::objectCreated,
                         &app,
                         [startupQmlUrl](QObject *object, const QUrl &objectUrl) {
                             if (!object && objectUrl == startupQmlUrl) {
                                 QCoreApplication::exit(-1);
                             }
                         },
                         Qt::QueuedConnection);
        engine.load(startupQmlUrl);
        return app.exec();
    }

    VideoSourceRepository videoSourceRepository(databaseManager.database());
    if (startupError.isEmpty() && !videoSourceRepository.secureLegacyNetworkEndpoints()) {
        startupError = videoSourceRepository.lastError();
    }
    if (!startupError.isEmpty()) {
        StartupController startupController(startupError, appPaths.rootDirectory());
        engine.setInitialProperties({
            {QStringLiteral("startupController"), QVariant::fromValue(static_cast<QObject *>(&startupController))},
        });
        const QUrl startupQmlUrl(QStringLiteral("qrc:/qt/qml/VideoAnalysisPlatform/qml/StartupError.qml"));
        engine.load(startupQmlUrl);
        return app.exec();
    }
    ChannelBindingRepository channelBindingRepository(databaseManager.database());
    AlarmEventRepository alarmEventRepository(databaseManager.database());
    RoiRuleRepository roiRuleRepository(databaseManager.database());
    VideoSourceModel videoSourceModel(&videoSourceRepository);
    AlarmEventModel alarmEventModel(&alarmEventRepository);
    alarmEventModel.setMediaDirectory(appPaths.snapshotDirectory());
    RecordingModel recordingModel(databaseManager.database(), appPaths.recordingDirectory());
    recordingModel.reindex();
    SettingsController settingsController(&appPaths);
    AlarmManager alarmManager(&alarmEventRepository, appPaths.snapshotDirectory());
    QObject::connect(&alarmManager,
                     &AlarmManager::alarmCreated,
                     &alarmEventModel,
                     &AlarmEventModel::addAlarm);
    QObject::connect(&alarmManager, &AlarmManager::alarmFailed, [](const QString &message) {
        qWarning().noquote() << message;
    });
    StreamSessionManager streamSessionManager(videoFrameProvider,
                                              &alarmManager,
                                              &roiRuleRepository,
                                              &videoSourceRepository,
                                              &channelBindingRepository,
                                              appPaths.snapshotDirectory(),
                                              appPaths.recordingDirectory());
    QObject::connect(&videoSourceModel,
                     &VideoSourceModel::sourceChanged,
                     &streamSessionManager,
                     &StreamSessionManager::refreshSource);
    PerformanceModel performanceModel(&streamSessionManager);
    engine.setInitialProperties({
        {QStringLiteral("videoSourceModel"), QVariant::fromValue(static_cast<QObject *>(&videoSourceModel))},
        {QStringLiteral("alarmEventModel"), QVariant::fromValue(static_cast<QObject *>(&alarmEventModel))},
        {QStringLiteral("streamSessionManager"), QVariant::fromValue(static_cast<QObject *>(&streamSessionManager))},
        {QStringLiteral("performanceModel"), QVariant::fromValue(static_cast<QObject *>(&performanceModel))},
        {QStringLiteral("recordingModel"), QVariant::fromValue(static_cast<QObject *>(&recordingModel))},
        {QStringLiteral("appPaths"), QVariant::fromValue(static_cast<QObject *>(&appPaths))},
        {QStringLiteral("settingsController"), QVariant::fromValue(static_cast<QObject *>(&settingsController))},
    });

    const QUrl mainQmlUrl(QStringLiteral("qrc:/qt/qml/VideoAnalysisPlatform/qml/Main.qml"));

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreated,
        &app,
        [mainQmlUrl](QObject *object, const QUrl &objectUrl) {
            if (!object && objectUrl == mainQmlUrl) {
                QCoreApplication::exit(-1);
            }
        },
        Qt::QueuedConnection);

    engine.load(mainQmlUrl);

    return app.exec();
}
