# 项目作战手册

## 项目用途

本项目是“实时多路视频流智能分析平台”的 Qt/QML 桌面端工程。长期目标是面向安防监控、工业视觉和视频分析场景，逐步实现视频源管理、多路视频墙、告警、截图录像、性能监控、RTSP/FFmpeg 和 AI 检测等能力。

当前代码范围已推进至阶段 18（ONNX Runtime AI 检测）。现有代码实现 Qt/QML/CMake 启动窗口、左侧导航、静态页面占位、基础工具栏、统一主题、视频源列表模型、SQLite 视频源持久化、基于 OpenCV `VideoCapture` 的最多四路本地 mp4 播放骨架、基于 FFmpeg 的 RTSP 解码入口、RTSP 后台自动重连、用于解码和画面分发解耦的线程安全帧队列、约 1 秒刷新一次的视频流性能监控面板、手动截图保存能力、手动录像能力、基于 OpenCV 帧差法的基础运动检测框叠加、告警事件表、仓库、列表模型、告警中心列表、由 `AlarmManager` 统一处理的 `MotionDetected` 自动告警和截图入库、矩形 ROI 绘制/显示/删除、ROI 规则持久化、基于 ROI 中心点命中的 `IntrusionDetected` 自动告警、默认用于视频帧/检测框/ROI/状态叠加绘制的自定义渲染项，以及可选 ONNX Runtime 目标检测、AI 框叠加和 `PersonDetected` / `VehicleDetected` 自动告警；不包含 9/16 路视频墙、硬件解码、告警前后录像或录像入库功能。真实 ONNX 模型推理、FFmpeg RTSP 集成和四路长时间播放仍需在具备对应运行时的环境中验收。

## 技术栈

- C++17
- Qt 6
- QML / Qt Quick / Qt Quick Controls
- CMake（最低 3.21）
- OpenCV（用于本地 mp4 解码，阶段 5 最多四路）
- FFmpeg（用于 RTSP 视频流解码；未配置时项目仍可构建，但 RTSP 播放会报 FFmpeg 未启用）
- ONNX Runtime（用于可选目标检测；未配置时项目仍可构建，但 AI 检测会降级为禁用状态）
- Qt Test（CMake 配置为必需组件；当前有 10 个 C++ 单元测试目标，覆盖帧队列、截图、录像、告警仓库/管理器、ROI 仓库、运动检测、ONNX 检测器、入侵检测和 FFmpeg 解码器）
- VS2022 + MSVC 2022 64-bit 作为当前推荐 Windows 构建环境

## 重要目录

- `CMakeLists.txt`：顶层 CMake 配置，定义 `VideoAnalysisPlatform` 可执行目标和 QML 模块。
- `src/main.cpp`：程序入口，创建 `QGuiApplication` 并加载 QML。
- `src/core/analysis/DetectionResult.h`：基础检测结果结构，包含 `sourceId`、`timestamp`、传统矩形框列表 `boxes`，以及带类别、置信度和事件类型的 `detections`。
- `src/core/analysis/IObjectDetector.h`：目标检测器接口，当前由 `OnnxObjectDetector` 实现。
- `src/core/analysis/IVideoAnalyzer.h`：视频分析器接口，当前由 `MotionDetector` 实现。
- `src/core/analysis/IntrusionDetector.h`：区域入侵检测器，只根据 `MotionDetector` 输出的 box 和 ROI 规则判断是否命中；第一版使用 box 中心点是否落入矩形 ROI，不操作数据库或告警模型。
- `src/core/analysis/MotionDetector.h`：使用 OpenCV 帧差法做基础运动检测，流程包含灰度转换、`absdiff`、阈值二值化、形态学处理、轮廓检测和面积过滤。
- `src/core/analysis/OnnxObjectDetector.h`：可选 ONNX Runtime 目标检测器，负责加载 ONNX 模型、图像预处理、推理和基础 YOLO 类后处理；未配置 ONNX Runtime 时会安全返回错误并保持禁用。
- `src/core/analysis/RoiRule.h`：矩形 ROI 规则结构，包含 `id`、`sourceId`、`ruleName`、归一化 `rect`、`enabled`、`cooldownSeconds` 和时间字段。
- `src/core/alarm/AlarmEvent.h`：告警事件数据结构，包含 `id`、`sourceId`、`sourceName`、`eventType`、`level`、`message`、`snapshotPath`、`videoClipPath`、`confirmed`、`timestamp`。
- `src/core/alarm/AlarmManager.h`：统一负责 `MotionDetected` / `IntrusionDetected` / `PersonDetected` / `VehicleDetected` 告警生成、冷却、自动截图、写入 `alarm_events` 和发出实时告警信号；运动告警默认同源冷却 10 秒，入侵告警按 source + ROI rule 使用每条规则的 `cooldownSeconds`，AI 告警按 source + eventType 冷却。
- `src/core/storage/AlarmEventRepository.h`：封装 `alarm_events` 表的加载、新增、确认和删除操作。
- `src/core/storage/DatabaseManager.h`：负责打开 `data/app.db` 并初始化当前阶段的数据表 `video_sources`、`alarm_events` 和 `roi_rules`。
- `src/core/storage/RoiRuleRepository.h`：封装 `roi_rules` 表的增删改查，矩形 ROI 坐标以归一化 `x/y/width/height` 写入 `points_json`。
- `src/core/storage/VideoSourceRepository.h`：封装 `video_sources` 表的加载、新增、删除和改名操作。
- `src/core/video/Frame.h`：视频帧结构，包含 `image`、`timestamp`、`width`、`height`、`frameIndex`。
- `src/core/video/FrameQueue.h`：线程安全帧队列，支持固定最大容量、push/pop/tryPop/clear/size、关闭唤醒和 `droppedFrames` 统计。
- `src/core/video/IVideoDecoder.h`：视频解码器接口，当前由 `OpenCVVideoDecoder` 和 `FFmpegVideoDecoder` 实现。
- `src/core/video/OpenCVVideoDecoder.h`：使用 OpenCV `VideoCapture` 读取本地 mp4，并将 `cv::Mat` 转换为 `QImage`。
- `src/core/video/FFmpegVideoDecoder.h`：使用 FFmpeg 打开本地视频或 RTSP 流，读取视频流信息，解码视频帧并转换为 RGBA `QImage`；支持由 `StreamSession` 传入停止中断标记；未配置 FFmpeg 时会安全返回错误。
- `src/core/video/RecordingManager.h`：负责手动录像文件创建、写帧和关闭；默认保存到 `data/records/YYYY-MM-DD/`，当前使用 OpenCV `VideoWriter` 写 MJPG `.avi` 文件，未启用 OpenCV 时返回错误。
- `src/core/video/SnapshotManager.h`：负责将 `QImage` 保存到本地截图文件，默认目录为 `data/snapshots/YYYY-MM-DD/`，文件名包含 sourceId、时间和事件类型。
- `src/core/video/StreamSession.h`：单路视频播放会话，负责启动、暂停、停止、状态、FPS、后台解码线程生命周期、RTSP 重连状态机、手动截图/录像触发、基础运动检测结果发布、ROI 列表加载/保存、ONNX 模型加载和异步 AI 推理调度，并在检测到运动、ROI 入侵或 AI 目标时发出 `motionDetected` / `intrusionDetected` / `objectDetected` 信号。
- `src/core/video/StreamSessionManager.h`：统一创建并持有 4 个 `StreamSession`，向 QML 提供 `sessionAt(index)` 和 `stopAll()`，并负责把每路 `StreamSession` 的运动、入侵和 AI 目标信号连接到 `AlarmManager`。
- `src/core/video/StreamPerformance.h`：单路性能快照结构，包含 `sourceName`、`fps`、`decodeTimeMs`、`queueSize`、`droppedFrames`、`status`。
- `src/core/video/VideoSource.h`：视频源数据结构、视频源类型和状态枚举。
- `src/models/PerformanceModel.h`：继承 `QAbstractListModel` 的性能监控模型，每秒从 `StreamSessionManager` 采样一次所有 `StreamSession` 的性能快照。
- `src/models/AlarmEventModel.h`：继承 `QAbstractListModel` 的告警事件列表模型，通过 `AlarmEventRepository` 加载和保存数据，向 QML 暴露 `alarmId`、`sourceId`、`sourceName`、`eventType`、`level`、`message`、`snapshotPath`、`videoClipPath`、`confirmed`、`timestamp` 等角色。
- `src/models/VideoSourceModel.h`：继承 `QAbstractListModel` 的视频源列表模型，通过 `VideoSourceRepository` 加载和保存数据，向 QML 暴露 `name`、`type`、`url`、`enabled`、`sourceEnabled`、`status`、`lastError` 等角色。
- `src/render/VideoFrameProvider.h`：QML `image://videoFrame/<streamId>` 图像提供器，按 stream id 缓存并显示后台解码得到的 `QImage`，同时提供当前帧拷贝供手动截图使用。
- `src/render/VideoRenderItem.h`：注册给 QML 的自定义视频渲染项，基于 `QQuickPaintedItem`（`QQuickItem` 子类）先用 `QPainter` 保证功能正确，默认负责绘制当前视频帧、运动检测框、AI 目标框、ROI 和基础状态叠加；暂不做复杂 GPU 零拷贝。
- `tests/FrameQueueTests.cpp`：FrameQueue 基础单元测试，覆盖 push/pop、tryPop、clear、容量丢帧和基础多线程。
- `tests/AlarmEventRepositoryTests.cpp`：告警事件仓库基础单元测试，覆盖新增、加载、确认和删除。
- `tests/RoiRuleRepositoryTests.cpp`：ROI 规则仓库基础单元测试，覆盖新增、按 source 加载、更新和删除。
- `tests/AlarmManagerTests.cpp`：告警管理器基础单元测试，覆盖 `MotionDetected` / `IntrusionDetected` / `PersonDetected` 写库、自动截图路径和冷却。
- `tests/MotionDetectorTests.cpp`：基础运动检测单元测试，覆盖两帧差异检测和 reset 行为。
- `tests/OnnxObjectDetectorTests.cpp`：ONNX 检测器基础单元测试，覆盖未配置或未加载模型时的优雅失败路径和阈值钳制。
- `tests/IntrusionDetectorTests.cpp`：入侵检测单元测试，覆盖 box 中心点进入 ROI、ROI 外和禁用 ROI 的行为。
- `tests/FFmpegVideoDecoderTests.cpp`：FFmpeg 解码器基础单元测试，覆盖 RTSP 类型字符串和空输入失败路径。
- `tests/RecordingManagerTests.cpp`：录像管理器基础单元测试，覆盖手动录像文件生成、日期目录、文件名和 OpenCV 可播放性。
- `tests/SnapshotManagerTests.cpp`：截图管理器基础单元测试，覆盖目录创建、文件保存、文件命名和空帧失败路径。
- `qml/Main.qml`：当前主窗口，只负责整体布局、左侧导航和页面切换。
- `PROJECT_REQUIREMENTS.md`：完整需求、路线图、推荐目录结构和阶段任务来源。
- `README.md`：项目简介与本地构建命令。
- `docs/`：后续项目文档目录，目前未填充。
- `src/app/`：应用层预留目录，目前为空；`src/core/`、`src/models/`、`src/render/`：当前已实现核心、模型和渲染模块。
- `qml/pages/`：基础页面目录，包含首页、视频墙、视频源、告警中心、回放和设置页面；其中 `DashboardPage.qml`、`PlaybackPage.qml`、`SettingsPage.qml` 仍是占位页，`SourceManagerPage.qml` 绑定 C++ `VideoSourceModel`，`AlarmCenterPage.qml` 绑定 C++ `AlarmEventModel`。
- `qml/components`：基础组件目录，当前包含导航项、页面占位容器、工具栏、默认使用 `VideoRenderItem` 且保留旧 `Image` fallback 的视频播放卡片 `VideoTile.qml`、ROI 绘制组件 `RoiEditor.qml`，以及性能面板 `PerformancePanel.qml`；`VideoTile.qml` 同时提供运动检测、AI 检测、ONNX 模型选择、阈值和推理间隔控件。
- `qml/styles/AppTheme.qml`：统一管理颜色、间距、圆角和字体大小。
- `build/` 和 `data/`：分别是构建产物和运行时数据目录，已被 `.gitignore` 忽略；不要把其中内容当作源码或稳定测试基线。

## 构建和运行

当前 README 推荐命令：

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="C:\Qt\6.x.x\msvc2022_64"
cmake --build build --config Release
.\build\Release\VideoAnalysisPlatform.exe
```

注意：`C:\Qt\6.x.x\msvc2022_64` 必须替换为本机实际 Qt 6 MSVC 目录。阶段 4 播放功能需要 OpenCV C++ 开发包；配置时可额外传入 `-DOpenCV_DIR="C:\opencv\build"`，该路径应包含 `OpenCVConfig.cmake`。阶段 14 RTSP 播放需要 FFmpeg C/C++ 开发包；配置时可额外传入 `-DFFMPEG_ROOT="C:\ffmpeg"`，该目录下应包含 `include/libavformat/avformat.h` 和对应 `lib` 目录。阶段 18 ONNX AI 检测需要 ONNX Runtime C/C++ 开发包；配置时可额外传入 `-DONNXRUNTIME_ROOT="C:\onnxruntime"`，该目录下应包含 ONNX Runtime 的 `include` 和 `lib` 目录。也可以用 VS2022 直接打开项目根目录，通过 CMake 工程配置、构建和运行。

Windows 构建仅在 CMake 找到 `windeployqt` 时为主程序和各测试目标添加 POST_BUILD 部署步骤，将 Qt DLL、平台插件和 QML/Quick 运行时依赖部署到对应配置输出目录；它不会自动部署 OpenCV、FFmpeg 或 ONNX Runtime DLL。Debug 和 Release 应优先通过重新构建触发部署后再运行。

测试命令：

```powershell
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

各 Qt 测试目标构建后也会自动调用 `windeployqt`，避免 CTest 在 Windows PATH 中误加载其他 Qt 套件的运行时 DLL。

## 当前入口和加载流程

1. `src/main.cpp` 创建 `QGuiApplication`。
2. `QQmlApplicationEngine` 加载 `qrc:/qt/qml/VideoAnalysisPlatform/qml/Main.qml`。
3. QML 创建 `ApplicationWindow`，显示左侧导航并通过 `StackLayout` 切换页面。
4. `main.cpp` 创建 `VideoFrameProvider` 并注册为 `image://videoFrame`。
5. `main.cpp` 创建 `DatabaseManager`，打开并初始化 `data/app.db`。
6. `main.cpp` 创建 `VideoSourceRepository`、`VideoSourceModel`、`AlarmEventRepository`、`AlarmEventModel`、`RoiRuleRepository`、`AlarmManager`、`StreamSessionManager` 和 `PerformanceModel`，通过 QML 根对象初始属性 `videoSourceModel`、`alarmEventModel`、`streamSessionManager`、`performanceModel` 暴露给 `Main.qml`。
7. `VideoSourceModel` 启动时从 `video_sources` 表加载历史视频源；`AlarmEventModel` 启动时从 `alarm_events` 表加载历史告警；每个 `StreamSession` 启动时按当前 stream/sourceId 从 `roi_rules` 加载 ROI；`VideoWallPage.qml` 通过 `StreamSessionManager.sessionAt(index)` 为每个 `VideoTile.qml` 绑定不同 `StreamSession`。
8. `StreamSession` 根据当前 `VideoSourceType` 选择解码器：本地文件默认使用 `OpenCVVideoDecoder`，RTSP 使用 `FFmpegVideoDecoder`。解码线程读取帧后写入 `FrameQueue`；分发线程从 `FrameQueue` 取帧后通过 queued invoke 更新 `VideoFrameProvider` 中的当前帧和 `frameSource`。RTSP 连续读帧或打开失败时进入 `Reconnecting`，按固定间隔后台重试，超过 `maxReconnectAttempts` 后进入 `Error`。
9. `VideoTile.qml` 默认使用 `VideoRenderItem` 显示视频；`VideoRenderItem` 绑定对应 `StreamSession`，在 `frameSourceChanged` 时读取 `StreamSession.currentFrame()`，并在 C++ 中绘制视频帧、检测框、ROI、FPS/状态和录像状态。旧 `image://videoFrame` + `Image` 路径仍保留，可通过 `VideoTile.useRenderItem = false` 作为 fallback。
10. `PerformanceModel` 约每秒读取一次每路 `StreamSession` 的 `StreamPerformance` 快照，并驱动 `PerformancePanel.qml` 显示 `sourceName`、`fps`、`decodeTimeMs`、`queueSize`、`droppedFrames`、`status`。
11. 用户在 `VideoTile.qml` 点击截图时，QML 调用 `StreamSession.takeSnapshot("jpg", "manual")`；`StreamSession` 从 `VideoFrameProvider` 拷贝当前帧，并让 `SnapshotManager` 在后台线程保存图片，完成后通过 `snapshotFinished(success, filePath, errorMessage)` 返回结果。
12. 用户在 `VideoTile.qml` 点击录像时，QML 调用 `StreamSession.startRecording/stopRecording`；`StreamSession` 使用当前帧尺寸和源 FPS 创建 `RecordingManager` 录像文件，并在解码线程每帧写入，停止播放、切换源、视频结束或错误时会关闭文件。
13. 用户在 `VideoTile.qml` 输入 RTSP 地址并点击“打开 RTSP”时，QML 调用 `StreamSession.openRtspUrl`，成功后复用 `play/pause/stop` 控制；RTSP 打开或读取失败会进入 `Reconnecting` 并显示当前重连次数，用户点击停止会中断重连。
14. 用户在 `VideoTile.qml` 开启运动检测时，QML 设置 `StreamSession.motionDetectionEnabled` 和 `minTargetArea`；`StreamSession` 在后台解码线程调用 `MotionDetector`，再把 `DetectionResult` 中的 boxes 转成归一化坐标通过 `detectionBoxes` 暴露给 `VideoRenderItem` 绘制，同时把检测框和 ROI 快照交给 `IntrusionDetector` 做中心点命中判断。
15. 用户在 `VideoTile.qml` 选择 ONNX 模型时，QML 调用 `StreamSession.loadAiModel`；模型加载和 AI 推理都在独立 AI 工作线程中执行。播放时 `StreamSession` 按 `aiInferenceInterval` 抽帧调度 `OnnxObjectDetector`，结果通过 `objectDetectionBoxes` 暴露给 `VideoRenderItem` 绘制。未配置 ONNX Runtime 时模型加载会返回降级错误，不影响运动检测和播放。
16. 若运动检测框非空，`StreamSession` 发出 `motionDetected`；`AlarmManager` 按 sourceKey 做冷却判断，保存自动截图，创建 `MotionDetected` 告警并写入 `alarm_events`；写库成功后发出 `alarmCreated`，`AlarmEventModel::addAlarm` 将新告警实时插入告警中心列表。
17. 若运动检测框中心点进入某条启用 ROI，`StreamSession` 发出 `intrusionDetected`；`AlarmManager` 按 source + ROI rule 和该规则的 `cooldownSeconds` 做冷却判断，保存自动截图，创建 `IntrusionDetected` 告警并写入 `alarm_events`。
18. 若 AI 检测到 person 或车辆类目标，`StreamSession` 发出 `objectDetected`；`AlarmManager` 按 source + eventType 做冷却判断，保存自动截图，创建 `PersonDetected` 或 `VehicleDetected` 告警并写入 `alarm_events`。
19. 用户在 `VideoTile.qml` 开启“绘制 ROI”后，`RoiEditor.qml` 根据 `VideoRenderItem` 或 fallback `Image` 的 `paintedWidth` / `paintedHeight` 把鼠标拖拽矩形转换为 0 到 1 的归一化视频坐标，并调用 `StreamSession.addRoiRect`；`StreamSession` 通过 `RoiRuleRepository` 保存到 `roi_rules`，`roiRules` 变化后由 `VideoRenderItem` 重新绘制 ROI。删除/清空 ROI 也由 `StreamSession` 同步删除数据库记录。
20. 用户在 `AlarmCenterPage.qml` 点击“添加测试告警”“确认”“删除”时，QML 只调用 `AlarmEventModel` 暴露的方法，实际 SQLite 写入、更新和删除由 `AlarmEventRepository` 负责。
21. 若 QML 根对象创建失败，程序以错误码退出。

## 代码约定

- 每次只完成一个明确阶段或子任务，避免一次性跨阶段实现。
- QML 只负责界面、布局和交互展示，复杂业务逻辑放到 C++ 层。
- 不要把后续业务逻辑继续堆进 `qml/Main.qml`；页面内容放在 `qml/pages`，复用控件放在 `qml/components`，视觉常量放在 `qml/styles/AppTheme.qml`。
- 视频源数据增删改由 `VideoSourceModel` 负责，并通过 `VideoSourceRepository` 持久化；QML 页面只展示模型数据并调用模型暴露的方法。
- 每路播放由独立 `StreamSession` 负责状态和线程生命周期，QML 只调用对应 session 的 `openLocalFile`、`openRtspUrl`、`play`、`pause`、`stop`、`startRecording`、`stopRecording` 并展示帧、FPS、重连状态、录像状态和错误信息。当前会话分支为 RTSP 使用 FFmpeg、其他类型使用 OpenCV；不要据 `FFmpegVideoDecoder` 类本身支持本地输入而改变会话选择逻辑。
- 多路播放由 `StreamSessionManager` 统一管理 session 数量；当前固定 4 路，不要在当前阶段顺手扩展 9/16 路。
- `StreamSession` 必须通过 `FrameQueue` 解耦解码和画面分发；队列最大容量当前在 `StreamSession.cpp` 中为 3，停止 session 时必须关闭并清空队列。RTSP 重连只允许影响当前 session，不能阻塞 UI，也不能改变本地 mp4 的 EOF/错误处理逻辑。
- `VideoTile.qml` 默认使用 `VideoRenderItem` 渲染视频帧、检测框、ROI 和基础状态叠加；旧 `Image` 方案通过 `useRenderItem = false` 保留为 fallback。后续不要把大量逐帧矩形 Repeater 逻辑重新堆回 QML。
- 性能面板数据由 C++ `PerformanceModel` 按约 1 秒频率采样，不要在 QML 中按帧计算或刷新性能列表。
- 截图保存由 C++ `SnapshotManager` 负责；QML 只触发手动截图并展示 `snapshotFinished` 返回的成功路径或错误信息。告警自动截图只能通过 `AlarmManager` 触发，并由 `AlarmManager` 把成功路径写入告警事件。
- 手动录像由 C++ `RecordingManager` 负责；QML 只触发开始/停止并展示 `recordingResult` 返回的成功路径或错误信息。当前录像保存为 `data/records/YYYY-MM-DD/*.avi`，不写数据库，不做告警前后录像。
- 基础运动检测由 C++ `MotionDetector` 负责；QML 只设置开关和 `minTargetArea`，检测框默认由 `VideoRenderItem` 根据 `detectionBoxes` 绘制，fallback `Image` 模式下才使用 QML 矩形叠加。关闭检测时必须清空检测框。`MotionDetector` 不能直接操作数据库或告警模型，只能由 `StreamSession` 通知 `AlarmManager`。
- ONNX AI 检测由 C++ `OnnxObjectDetector` 和 `StreamSession` 的 AI 工作线程负责；QML 只选择模型、设置 `aiDetectionEnabled`、`aiConfidenceThreshold` 和 `aiInferenceInterval`，AI 框默认由 `VideoRenderItem` 根据 `objectDetectionBoxes` 绘制。`OnnxObjectDetector` 不能直接操作数据库或告警模型，只能由 `StreamSession` 通知 `AlarmManager`。未配置 ONNX Runtime 时必须保持项目可构建、可运行，且不能影响运动检测。
- ROI 入侵判断由 C++ `IntrusionDetector` 负责；它只接收检测框、帧尺寸和 ROI 规则快照，第一版只判断 box 中心点是否落入矩形 ROI，不能写数据库或直接生成告警。
- 告警事件增删改查由 `AlarmEventModel` 和 `AlarmEventRepository` 负责；QML 页面只展示列表并调用模型方法。`MotionDetected`、`IntrusionDetected`、`PersonDetected` 和 `VehicleDetected` 自动告警由 `AlarmManager` 统一生成、截图和写库。
- ROI 规则由 `RoiRuleRepository` 持久化到 `roi_rules`，QML 通过 `RoiEditor.qml` 绘制，只调用 `StreamSession.addRoiRect/removeRoiRule/clearRoiRules`。默认渲染模式下已有 ROI 由 `VideoRenderItem` 绘制，`RoiEditor.qml` 主要负责拖拽草稿和编辑时的删除手柄。当前 sourceId 实际来自 `stream1` 到 `stream4` 的通道编号，尚未和视频源管理页中的数据库视频源 id 打通。
- C++ 模块按职责放置：应用控制在 `src/app`，核心处理在 `src/core`，QML 数据模型在 `src/models`，渲染相关在 `src/render`。
- `VideoSourceModel` 的 `video_sources.id` 与 `StreamSession` 的 `stream1` 到 `stream4` 当前是两条独立链路；视频源管理页的增删改不会自动绑定视频墙会话，切换文件时会复用通道级 ROI。需要接通时先明确 source/session 映射，再修改仓库和 QML 调用方。
- `DatabaseManager` 创建的 SQLite 连接和各 Repository 只在创建连接的主线程使用；不要从解码、截图或 AI 工作线程直接访问 Repository。当前 schema 只有 `CREATE TABLE IF NOT EXISTS`，没有版本/迁移机制，修改列或约束时必须同时设计迁移。
- 当前阶段只允许创建 `video_sources`、`alarm_events` 和 `roi_rules` 表，不要提前创建设置、录像、AI 等表。
- 不要擅自新增大型依赖或公共 API；当前允许使用 OpenCV 本地 mp4 解码、FFmpeg RTSP 解码和可选 ONNX Runtime 目标检测。ONNX Runtime 必须保持可选依赖，未找到时走降级路径。
- 修改复杂模块时同步更新 `README.md` 或 `docs/` 中长期有价值的说明。

## 禁止随意修改

- 不要在当前阶段扩展硬件解码、9/16 路视频墙、告警前后录像或录像入库功能。
- 不要删除 `PROJECT_REQUIREMENTS.md`，它是当前需求和路线图的主要依据。
- 不要无关重构 CMake、目录结构或 QML 模块 URI。

## 协作与子代理

- 简单、局部或强依赖的任务由主代理直接完成；复杂且可并行的任务采用“子代理调查，主代理修改”。
- 子代理只负责阅读、分析、比较、根因定位和轻量验证，不得修改代码、配置、测试或文档，也不得继续派遣下级子代理，除非用户明确要求。主代理必须保留需求和决策，等待影响修改决策的调查结果后统一编辑，并抽查关键证据。
- 派遣子代理时明确调查目标、范围、禁止范围、可执行命令、重点问题和返回格式。子代理返回至少包含调查范围、关键证据、文件/函数位置、命令及结果、根因判断、不确定项、影响范围、建议修改点、建议验证方法和主代理抽查点。
- 完整构建、完整回归测试和大型数据处理由主代理统一安排，避免子代理重复执行耗时操作；项目记忆只记录稳定的架构、边界、命令和长期风险，不记录一次性聊天或临时结果。

## 验证方法

- 最低验证：CMake 配置成功、构建成功、启动后能看到主窗口。
- C++ 单元测试：`ctest --test-dir build -C Release --output-on-failure`，当前注册 10 个目标：`FrameQueueTests`、`SnapshotManagerTests`、`RecordingManagerTests`、`AlarmEventRepositoryTests`、`RoiRuleRepositoryTests`、`AlarmManagerTests`、`MotionDetectorTests`、`OnnxObjectDetectorTests`、`IntrusionDetectorTests` 和 `FFmpegVideoDecoderTests`。这些测试不覆盖 QML、GUI、StreamSession 端到端流程；可选依赖未配置时，相关用例只验证降级或优雅失败路径。
- QML/手工验证：窗口标题正常，左侧导航可以切换页面；当前可操作页面主要是视频墙、视频源和告警中心，Dashboard、Playback、Settings 仍为占位页。视频源页可以添加本地或 RTSP 占位源、删除源、修改源名称，列表自动刷新，但目前不会自动驱动视频墙会话；重启程序后视频源列表从 `data/app.db` 恢复。告警中心可以添加测试告警、确认告警、删除告警，重启程序后历史告警从 `data/app.db` 恢复。视频墙页可以在 1 宫格和 4 宫格之间切换，每个格子可以独立选择本地 mp4 或输入 RTSP 地址，并通过播放、暂停、停止按钮观察独立状态和 FPS；默认 `VideoRenderItem` 应能显示视频帧、左上角状态/FPS 叠加、运动检测框、AI 检测框和 ROI，必要时可把 `VideoTile.useRenderItem` 设为 `false` 验证旧 `Image` fallback。未配置 FFmpeg 时打开 RTSP 应显示“FFmpeg 未启用”类错误；未配置 ONNX Runtime 时加载 ONNX 模型应显示降级错误且不影响运动检测；右侧性能面板显示所有通道的 `sourceName`、`fps`、`decodeTimeMs`、`queueSize`、`droppedFrames`、`status` 并约每秒更新。播放时点击截图按钮后，应在 `data/snapshots/YYYY-MM-DD/` 生成 jpg 文件；播放时点击录像按钮后，应显示 `REC`，停止后应在 `data/records/YYYY-MM-DD/` 生成可播放的 avi 文件。开启运动检测后，画面有明显变化时应出现黄色矩形框，并在告警中心实时出现 `MotionDetected` 告警；配置 ONNX Runtime 和真实模型后，才验证类别/置信度、`PersonDetected` / `VehicleDetected` 告警冷却。点击“绘制 ROI”后在实际视频画面区域拖拽应出现蓝色 ROI 框，ROI 应与视频画面缩放位置一致，并写入 `data/app.db` 的 `roi_rules`；重启后对应通道 ROI 应恢复；删除/清空 ROI 可立即移除界面和数据库记录。
- QML 静态检查：`cmake --build build --config Release --target all_qmllint`；必须先完成 Qt/CMake 配置并生成该目标。

## 当前风险和不确定信息

- 当前工作区存在 `.git` 目录，`git status` 可用；开始修改前仍需检查未提交改动，避免覆盖用户或其他工具的工作。
- 当前终端是否能直接找到 `cmake.exe`、Qt 6 安装路径以及 Windows SDK 访问权限，取决于本机 VS2022/Qt 环境配置。
- 现有 `build/CMakeCache.txt` 是环境快照而不是本轮重新配置结果：截至 2026-08-10，该缓存使用 Qt 6.11.0 和 OpenCV 4.13.0，未找到 FFmpeg 和 ONNX Runtime。重新配置后必须以 CMake 输出和目标宏为准；缺失可选依赖时对应能力应返回降级错误，不影响基础 QML 启动。
- `docs/` 和 `src/app/` 目前为空预留目录；`src/core/`、`src/models/`、`src/render/` 和 `tests/` 已有实现。不要把历史 `build/` 产物或 `data/` 数据库当作当前源码/测试结论。
- `DatabaseManager::open()` 的返回值目前未在 `main.cpp` 中阻止后续对象创建；数据库打不开时需优先检查 `data/app.db`、SQLite 驱动和 `DatabaseManager` 日志。schema 没有迁移版本，改表时必须补迁移策略；连接名固定为 `VideoAnalysisPlatformConnection`，未来多实例或并行测试时要避免连接复用冲突。
- `data/app.db`、`data/snapshots/` 和 `data/records/` 使用相对进程工作目录的路径；启动、手工验证和测试应固定工作目录，未来改为可配置或 AppData 路径时要同步更新文档。
- QML/GUI 没有自动化测试；四格布局在小窗口、ROI 与视频黑边对齐、VideoRenderItem 长时间资源占用仍需手工验证。
- ONNX 后处理按当前实现的常见 YOLO 输出和类别映射处理，未承诺通用模型格式；真实模型需验证输出布局、坐标映射、类别和置信度阈值。
- 运动检测告警当前只基于“检测框非空”触发，默认同源冷却 10 秒；ROI 入侵告警基于 box 中心点是否进入矩形 ROI，AI 告警当前只对 person 和车辆类目标生成 `PersonDetected` / `VehicleDetected`，尚未实现多边形 ROI、告警过滤、告警详情预览或自定义类别告警策略。
- ROI 当前按 stream/sourceId（1 到 4 路通道）持久化，尚未和视频源管理页的 `video_sources.id` 建立关系；同一通道切换不同本地文件时会复用该通道历史 ROI，这是后续接入真实视频源绑定时需要处理的限制。
- RTSP 自动重连当前为固定约 3 秒间隔、默认每路最多重试 3 次（上限 20 次）；现有缓存未找到 FFmpeg 开发包，仍需要在具备真实 RTSP 流和 FFmpeg DLL 的环境下做断线/恢复集成验证。
- 手动录像当前依赖 OpenCV `VideoWriter` 的 MJPG AVI 编码；不同 Windows 机器的播放器/编解码器支持可能有差异，仍需要用真实播放场景验证生成文件兼容性。
- `VideoRenderItem` 当前为第一版 `QQuickPaintedItem` / `QPainter` 实现，目标是减少 QML 层逐帧对象更新并保持功能正确；尚未做 GPU 纹理零拷贝，仍需要用真实四路视频长时间播放验证 UI 响应和资源占用。
