cmake_minimum_required(VERSION 3.21)

project(VideoAnalysisPlatform
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Quick)

qt_policy(SET QTP0001 NEW)
qt_policy(SET QTP0004 NEW)

qt_add_executable(VideoAnalysisPlatform
    src/main.cpp
)

qt_add_qml_module(VideoAnalysisPlatform
    URI VideoAnalysisPlatform
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/components/NavigationItem.qml
        qml/components/PagePlaceholder.qml
        qml/components/Toolbar.qml
        qml/pages/AlarmCenterPage.qml
        qml/pages/DashboardPage.qml
        qml/pages/PlaybackPage.qml
        qml/pages/SettingsPage.qml
        qml/pages/SourceManagerPage.qml
        qml/pages/VideoWallPage.qml
        qml/styles/AppTheme.qml
)

target_link_libraries(VideoAnalysisPlatform
    PRIVATE
        Qt6::Quick
)

set_target_properties(VideoAnalysisPlatform PROPERTIES
    WIN32_EXECUTABLE TRUE
)
