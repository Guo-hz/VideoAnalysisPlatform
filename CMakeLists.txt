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

find_package(Qt6 REQUIRED COMPONENTS Gui Quick QuickDialogs2 Sql Multimedia)
include(CTest)
if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test)
endif()
find_package(OpenCV QUIET COMPONENTS core imgproc videoio)

set(ONNXRUNTIME_ROOT "" CACHE PATH "ONNX Runtime install root directory")
set(_ONNXRUNTIME_SEARCH_ROOTS
    "${ONNXRUNTIME_ROOT}"
    "$ENV{ONNXRUNTIME_ROOT}"
    "$ENV{ONNXRuntime_ROOT}"
    "$ENV{ONNXRUNTIME_DIR}"
    "C:/onnxruntime"
    "D:/onnxruntime"
    "D:/ONNXRuntime"
    "D:/Programs/onnxruntime"
)

find_path(ONNXRUNTIME_INCLUDE_DIR
    NAMES onnxruntime_cxx_api.h
    HINTS ${_ONNXRUNTIME_SEARCH_ROOTS}
    PATH_SUFFIXES include include/onnxruntime include/onnxruntime/core/session
)

find_library(ONNXRUNTIME_LIBRARY
    NAMES onnxruntime
    HINTS ${_ONNXRUNTIME_SEARCH_ROOTS}
    PATH_SUFFIXES lib lib/x64 lib/Release bin
)

set(FFMPEG_ROOT "" CACHE PATH "FFmpeg install root directory")
set(_FFMPEG_SEARCH_ROOTS
    "${FFMPEG_ROOT}"
    "$ENV{FFMPEG_ROOT}"
    "$ENV{FFmpeg_ROOT}"
    "$ENV{FFMPEG_DIR}"
    "C:/ffmpeg"
    "D:/ffmpeg"
    "D:/FFmpeg"
    "D:/Programs/ffmpeg"
)

find_path(FFMPEG_INCLUDE_DIR
    NAMES libavformat/avformat.h
    HINTS ${_FFMPEG_SEARCH_ROOTS}
    PATH_SUFFIXES include
)

find_library(FFMPEG_AVFORMAT_LIBRARY
    NAMES avformat libavformat
    HINTS ${_FFMPEG_SEARCH_ROOTS}
    PATH_SUFFIXES lib lib/x64 bin
)

find_library(FFMPEG_AVCODEC_LIBRARY
    NAMES avcodec libavcodec
    HINTS ${_FFMPEG_SEARCH_ROOTS}
    PATH_SUFFIXES lib lib/x64 bin
)

find_library(FFMPEG_AVUTIL_LIBRARY
    NAMES avutil libavutil
    HINTS ${_FFMPEG_SEARCH_ROOTS}
    PATH_SUFFIXES lib lib/x64 bin
)

find_library(FFMPEG_SWSCALE_LIBRARY
    NAMES swscale libswscale
    HINTS ${_FFMPEG_SEARCH_ROOTS}
    PATH_SUFFIXES lib lib/x64 bin
)

if(FFMPEG_INCLUDE_DIR AND FFMPEG_AVFORMAT_LIBRARY AND FFMPEG_AVCODEC_LIBRARY AND FFMPEG_AVUTIL_LIBRARY AND FFMPEG_SWSCALE_LIBRARY)
    set(FFMPEG_FOUND TRUE)
    set(FFMPEG_LIBRARIES
        ${FFMPEG_AVFORMAT_LIBRARY}
        ${FFMPEG_AVCODEC_LIBRARY}
        ${FFMPEG_AVUTIL_LIBRARY}
        ${FFMPEG_SWSCALE_LIBRARY}
    )
    message(STATUS "FFmpeg found: ${FFMPEG_INCLUDE_DIR}")
else()
    set(FFMPEG_FOUND FALSE)
    message(WARNING "FFmpeg was not found. The project will build, but RTSP playback will report an error until FFMPEG_ROOT is configured.")
endif()

if(OpenCV_FOUND)
    message(STATUS "OpenCV found: ${OpenCV_VERSION}")
else()
    message(WARNING "OpenCV was not found. The project will build, but local video playback will report an error until OpenCV_DIR is configured.")
endif()

if(ONNXRUNTIME_INCLUDE_DIR AND ONNXRUNTIME_LIBRARY)
    set(ONNXRUNTIME_FOUND TRUE)
    message(STATUS "ONNX Runtime found: ${ONNXRUNTIME_INCLUDE_DIR}")
else()
    set(ONNXRUNTIME_FOUND FALSE)
    message(WARNING "ONNX Runtime was not found. The project will build, but AI detection will be disabled until ONNXRUNTIME_ROOT is configured.")
endif()

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
        qml/components/PerformancePanel.qml
        qml/components/RoiEditor.qml
        qml/components/Toolbar.qml
        qml/components/VideoTile.qml
        qml/pages/AlarmCenterPage.qml
        qml/pages/DashboardPage.qml
        qml/pages/PlaybackPage.qml
        qml/pages/SettingsPage.qml
        qml/pages/SourceManagerPage.qml
        qml/pages/VideoWallPage.qml
        qml/styles/AppTheme.qml
        qml/StartupError.qml
    SOURCES
        src/app/AppPaths.cpp
        src/app/AppPaths.h
        src/app/SettingsController.cpp
        src/app/SettingsController.h
        src/app/StartupController.cpp
        src/app/StartupController.h
        src/core/analysis/DetectionResult.h
        src/core/analysis/IObjectDetector.h
        src/core/analysis/IVideoAnalyzer.h
        src/core/analysis/IntrusionDetector.cpp
        src/core/analysis/IntrusionDetector.h
        src/core/analysis/MotionDetector.cpp
        src/core/analysis/MotionDetector.h
        src/core/analysis/OnnxObjectDetector.cpp
        src/core/analysis/OnnxObjectDetector.h
        src/core/analysis/RoiRule.h
        src/core/alarm/AlarmEvent.h
        src/core/alarm/AlarmManager.cpp
        src/core/alarm/AlarmManager.h
        src/core/storage/AlarmEventRepository.cpp
        src/core/storage/AlarmEventRepository.h
        src/core/storage/ChannelBindingRepository.cpp
        src/core/storage/ChannelBindingRepository.h
        src/core/storage/DatabaseManager.cpp
        src/core/storage/DatabaseManager.h
        src/core/storage/RoiRuleRepository.cpp
        src/core/storage/RoiRuleRepository.h
        src/core/storage/VideoSourceRepository.cpp
        src/core/storage/VideoSourceRepository.h
        src/core/security/CredentialStore.cpp
        src/core/security/CredentialStore.h
        src/core/video/Frame.h
        src/core/video/FrameQueue.cpp
        src/core/video/FrameQueue.h
        src/core/video/FFmpegVideoDecoder.cpp
        src/core/video/FFmpegVideoDecoder.h
        src/core/video/IVideoDecoder.h
        src/core/video/OpenCVVideoDecoder.cpp
        src/core/video/OpenCVVideoDecoder.h
        src/core/video/RecordingManager.cpp
        src/core/video/RecordingManager.h
        src/core/video/SnapshotManager.cpp
        src/core/video/SnapshotManager.h
        src/core/video/StreamSession.cpp
        src/core/video/StreamSession.h
        src/core/video/StreamSessionManager.cpp
        src/core/video/StreamSessionManager.h
        src/core/video/StreamPerformance.h
        src/core/video/VideoSource.cpp
        src/core/video/VideoSource.h
        src/models/AlarmEventModel.cpp
        src/models/AlarmEventModel.h
        src/models/VideoSourceModel.cpp
        src/models/VideoSourceModel.h
        src/models/PerformanceModel.cpp
        src/models/PerformanceModel.h
        src/models/RecordingModel.cpp
        src/models/RecordingModel.h
        src/render/VideoFrameProvider.cpp
        src/render/VideoFrameProvider.h
        src/render/VideoRenderItem.cpp
        src/render/VideoRenderItem.h
)

target_include_directories(VideoAnalysisPlatform
    PRIVATE
        src
        src/render
)

target_link_libraries(VideoAnalysisPlatform
    PRIVATE
        Qt6::Quick
        Qt6::QuickDialogs2
        Qt6::Sql
        Qt6::Multimedia
)

if(WIN32)
    target_link_libraries(VideoAnalysisPlatform PRIVATE Advapi32)
endif()

if(OpenCV_FOUND)
    target_compile_definitions(VideoAnalysisPlatform PRIVATE VIDEOANALYSIS_HAS_OPENCV=1)
    target_include_directories(VideoAnalysisPlatform PRIVATE ${OpenCV_INCLUDE_DIRS})
    target_link_libraries(VideoAnalysisPlatform PRIVATE ${OpenCV_LIBS})
endif()

if(FFMPEG_FOUND)
    target_compile_definitions(VideoAnalysisPlatform PRIVATE VIDEOANALYSIS_HAS_FFMPEG=1)
    target_include_directories(VideoAnalysisPlatform PRIVATE ${FFMPEG_INCLUDE_DIR})
    target_link_libraries(VideoAnalysisPlatform PRIVATE ${FFMPEG_LIBRARIES})
endif()

if(ONNXRUNTIME_FOUND)
    target_compile_definitions(VideoAnalysisPlatform PRIVATE VIDEOANALYSIS_HAS_ONNXRUNTIME=1)
    target_include_directories(VideoAnalysisPlatform PRIVATE ${ONNXRUNTIME_INCLUDE_DIR})
    target_link_libraries(VideoAnalysisPlatform PRIVATE ${ONNXRUNTIME_LIBRARY})
endif()

if(BUILD_TESTING)

qt_add_executable(FrameQueueTests
    tests/FrameQueueTests.cpp
    src/core/video/Frame.h
    src/core/video/FrameQueue.cpp
    src/core/video/FrameQueue.h
)

target_include_directories(FrameQueueTests
    PRIVATE
        src
)

target_link_libraries(FrameQueueTests
    PRIVATE
        Qt6::Gui
        Qt6::Test
)

add_test(NAME FrameQueueTests COMMAND FrameQueueTests)

qt_add_executable(SnapshotManagerTests
    tests/SnapshotManagerTests.cpp
    src/core/video/SnapshotManager.cpp
    src/core/video/SnapshotManager.h
)

target_include_directories(SnapshotManagerTests
    PRIVATE
        src
)

target_link_libraries(SnapshotManagerTests
    PRIVATE
        Qt6::Gui
        Qt6::Test
)

add_test(NAME SnapshotManagerTests COMMAND SnapshotManagerTests)

qt_add_executable(RecordingManagerTests
    tests/RecordingManagerTests.cpp
    src/core/video/RecordingManager.cpp
    src/core/video/RecordingManager.h
)

target_include_directories(RecordingManagerTests
    PRIVATE
        src
)

target_link_libraries(RecordingManagerTests
    PRIVATE
        Qt6::Gui
        Qt6::Test
)

if(OpenCV_FOUND)
    target_compile_definitions(RecordingManagerTests PRIVATE VIDEOANALYSIS_HAS_OPENCV=1)
    target_include_directories(RecordingManagerTests PRIVATE ${OpenCV_INCLUDE_DIRS})
    target_link_libraries(RecordingManagerTests PRIVATE ${OpenCV_LIBS})
endif()

add_test(NAME RecordingManagerTests COMMAND RecordingManagerTests)

qt_add_executable(AlarmEventRepositoryTests
    tests/AlarmEventRepositoryTests.cpp
    src/core/alarm/AlarmEvent.h
    src/core/storage/AlarmEventRepository.cpp
    src/core/storage/AlarmEventRepository.h
)

target_include_directories(AlarmEventRepositoryTests
    PRIVATE
        src
)

target_link_libraries(AlarmEventRepositoryTests
    PRIVATE
        Qt6::Sql
        Qt6::Test
)

add_test(NAME AlarmEventRepositoryTests COMMAND AlarmEventRepositoryTests)

qt_add_executable(RoiRuleRepositoryTests
    tests/RoiRuleRepositoryTests.cpp
    src/core/analysis/RoiRule.h
    src/core/storage/RoiRuleRepository.cpp
    src/core/storage/RoiRuleRepository.h
)

target_include_directories(RoiRuleRepositoryTests
    PRIVATE
        src
)

target_link_libraries(RoiRuleRepositoryTests
    PRIVATE
        Qt6::Sql
        Qt6::Test
)

add_test(NAME RoiRuleRepositoryTests COMMAND RoiRuleRepositoryTests)

qt_add_executable(AlarmManagerTests
    tests/AlarmManagerTests.cpp
    src/core/alarm/AlarmEvent.h
    src/core/alarm/AlarmManager.cpp
    src/core/alarm/AlarmManager.h
    src/core/storage/AlarmEventRepository.cpp
    src/core/storage/AlarmEventRepository.h
    src/core/video/SnapshotManager.cpp
    src/core/video/SnapshotManager.h
)

target_include_directories(AlarmManagerTests
    PRIVATE
        src
)

target_link_libraries(AlarmManagerTests
    PRIVATE
        Qt6::Gui
        Qt6::Sql
        Qt6::Test
)

add_test(NAME AlarmManagerTests COMMAND AlarmManagerTests)

qt_add_executable(MotionDetectorTests
    tests/MotionDetectorTests.cpp
    src/core/analysis/DetectionResult.h
    src/core/analysis/IVideoAnalyzer.h
    src/core/analysis/MotionDetector.cpp
    src/core/analysis/MotionDetector.h
)

target_include_directories(MotionDetectorTests
    PRIVATE
        src
)

target_link_libraries(MotionDetectorTests
    PRIVATE
        Qt6::Gui
        Qt6::Test
)

if(OpenCV_FOUND)
    target_compile_definitions(MotionDetectorTests PRIVATE VIDEOANALYSIS_HAS_OPENCV=1)
    target_include_directories(MotionDetectorTests PRIVATE ${OpenCV_INCLUDE_DIRS})
    target_link_libraries(MotionDetectorTests PRIVATE ${OpenCV_LIBS})
endif()

add_test(NAME MotionDetectorTests COMMAND MotionDetectorTests)

qt_add_executable(OnnxObjectDetectorTests
    tests/OnnxObjectDetectorTests.cpp
    src/core/analysis/DetectionResult.h
    src/core/analysis/IObjectDetector.h
    src/core/analysis/OnnxObjectDetector.cpp
    src/core/analysis/OnnxObjectDetector.h
)

target_include_directories(OnnxObjectDetectorTests
    PRIVATE
        src
)

target_link_libraries(OnnxObjectDetectorTests
    PRIVATE
        Qt6::Gui
        Qt6::Test
)

if(ONNXRUNTIME_FOUND)
    target_compile_definitions(OnnxObjectDetectorTests PRIVATE VIDEOANALYSIS_HAS_ONNXRUNTIME=1)
    target_include_directories(OnnxObjectDetectorTests PRIVATE ${ONNXRUNTIME_INCLUDE_DIR})
    target_link_libraries(OnnxObjectDetectorTests PRIVATE ${ONNXRUNTIME_LIBRARY})
endif()

add_test(NAME OnnxObjectDetectorTests COMMAND OnnxObjectDetectorTests)

qt_add_executable(IntrusionDetectorTests
    tests/IntrusionDetectorTests.cpp
    src/core/analysis/DetectionResult.h
    src/core/analysis/IntrusionDetector.cpp
    src/core/analysis/IntrusionDetector.h
    src/core/analysis/RoiRule.h
)

target_include_directories(IntrusionDetectorTests
    PRIVATE
        src
)

target_link_libraries(IntrusionDetectorTests
    PRIVATE
        Qt6::Gui
        Qt6::Test
)

add_test(NAME IntrusionDetectorTests COMMAND IntrusionDetectorTests)

qt_add_executable(FFmpegVideoDecoderTests
    tests/FFmpegVideoDecoderTests.cpp
    src/core/video/FFmpegVideoDecoder.cpp
    src/core/video/FFmpegVideoDecoder.h
    src/core/video/IVideoDecoder.h
    src/core/video/VideoSource.cpp
    src/core/video/VideoSource.h
)

target_include_directories(FFmpegVideoDecoderTests
    PRIVATE
        src
)

target_link_libraries(FFmpegVideoDecoderTests
    PRIVATE
        Qt6::Gui
        Qt6::Test
)

if(FFMPEG_FOUND)
    target_compile_definitions(FFmpegVideoDecoderTests PRIVATE VIDEOANALYSIS_HAS_FFMPEG=1)
    target_include_directories(FFmpegVideoDecoderTests PRIVATE ${FFMPEG_INCLUDE_DIR})
    target_link_libraries(FFmpegVideoDecoderTests PRIVATE ${FFMPEG_LIBRARIES})
endif()

add_test(NAME FFmpegVideoDecoderTests COMMAND FFmpegVideoDecoderTests)

endif()

set_target_properties(VideoAnalysisPlatform PROPERTIES
    WIN32_EXECUTABLE TRUE
)

if(WIN32)
    get_target_property(QT_QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(QT_BIN_DIR "${QT_QMAKE_EXECUTABLE}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${QT_BIN_DIR}")

    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET VideoAnalysisPlatform POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}"
                "--$<IF:$<CONFIG:Debug>,debug,release>"
                --qmldir "${CMAKE_SOURCE_DIR}/qml"
                "$<TARGET_FILE:VideoAnalysisPlatform>"
            COMMENT "Deploying Qt runtime dependencies"
            VERBATIM
        )
    else()
        message(WARNING "windeployqt was not found. Built executables may not run outside a Qt-enabled environment.")
    endif()
endif()
