# 项目作战手册

## 项目用途

本项目是“实时多路视频流智能分析平台”的 Qt/QML 桌面端工程。长期目标是面向安防监控、工业视觉和视频分析场景，逐步实现视频源管理、多路视频墙、告警、截图录像、性能监控、RTSP/FFmpeg 和 AI 检测等能力。

当前代码范围已超出本文件上一版记录的阶段 18（ONNX Runtime AI 检测）。现有代码实现 Qt/QML/CMake 启动窗口、左侧六页导航（首页/视频墙/视频源/告警中心/回放/设置，均为真实页面）、统一主题、视频源列表模型、SQLite 视频源持久化、基于 OpenCV `VideoCapture` 的最多四路本地 mp4 播放、基于 FFmpeg 的 RTSP/HTTP 解码入口、RTSP 后台自动重连、用于解码和画面分发解耦的线程安全帧队列、约 1 秒刷新一次的视频流性能监控面板、手动截图与手动录像、基于 OpenCV **MOG2 背景建模**的基础运动检测、告警事件表/仓库/列表模型/告警中心、由 `AlarmManager` 统一处理的 `MotionDetected` / `IntrusionDetected` / `PersonDetected` / `VehicleDetected` 自动告警和截图入库、矩形 ROI 绘制/显示/删除与规则持久化、默认用于视频帧/检测框/ROI/状态叠加绘制的自定义渲染项，以及可选 ONNX Runtime 目标检测和 AI 框叠加。此外还已实现：视频源↔视频墙通道绑定（`channel_bindings` + `StreamSessionManager::bindSource`）、录像文件磁盘索引与本地回放页（`recordings` + `RecordingModel` + `PlaybackPage.qml`）、应用数据目录固定与旧数据自动迁移（`AppPaths`）、启动失败页（`StartupError.qml` + `StartupController`）、设置页（`SettingsPage.qml` + `SettingsController`）、RTSP/HTTP 凭据托管（`CredentialStore`，Windows Credential Manager），以及 SQLite schema 版本迁移与迁移前自动备份。

不包含：9/16 路视频墙、硬件解码、告警前后录像。注意“录像入库”目前只是占位实现——`recordings` 表仅由启动期或手动 Reindex 扫描磁盘 `.avi` 建立索引行（`source_id` 固定为 0、`source_name` 固定为 `Indexed recording`、时长/分辨率/帧率为 0），录像过程中不写库。真实 ONNX 模型推理、FFmpeg RTSP 集成和四路长时间播放仍需在具备对应运行时的环境中验收。

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
- `src/main.cpp`：程序入口；创建 `QGuiApplication` 并设置 org/app 名，按 `AppPaths` → `DatabaseManager` → 各 Repository/Model/Controller/Manager 的顺序初始化，通过 `setInitialProperties` 注入 7 个 QML 根属性后加载 `Main.qml`；任一前置步骤失败则改为加载 `StartupError.qml` 并结束启动。
- `src/app/AppPaths.h`：应用数据目录解析。根目录固定为 `QStandardPaths::AppLocalDataLocation`，下含 `app.db`、`snapshots/`、`records/`、`models/`、`logs/`；`prepare()` 在目标 `app.db` 不存在时自动从旧布局（`data/`、`build/<Config>/data` 等）迁移数据库，并用 SQLite 只读探针校验候选库，旧目录改名保留为 `*.preexisting-<时间戳>`。路径目前是只读属性，不可配置。
- `src/app/SettingsController.h`：设置页后端，向 QML 转发 5 个路径属性和一个可读写属性 `diskWarningThresholdPercent`（int，钳制 1-50），提供 `openDirectory()` 和 `isModelPresent()`。该阈值用 **QSettings** 持久化（不是 `app_settings` 表）。
- `src/core/analysis/DetectionResult.h`：基础检测结果结构，包含 `sourceId`、`timestamp`、传统矩形框列表 `boxes`，以及带类别、置信度和事件类型的 `detections`。
- `src/core/analysis/IObjectDetector.h`：目标检测器接口，当前由 `OnnxObjectDetector` 实现。
- `src/core/analysis/IVideoAnalyzer.h`：视频分析器接口，当前由 `MotionDetector` 实现。
- `src/core/analysis/IntrusionDetector.h`：区域入侵检测器，只根据 `MotionDetector` 输出的 box 和 ROI 规则判断是否命中；第一版使用 box 中心点是否落入矩形 ROI，不操作数据库或告警模型。
- `src/core/analysis/MotionDetector.h`：使用 OpenCV **MOG2 背景建模**做基础运动检测（`createBackgroundSubtractorMOG2(500, 16.0, true)`），流程包含灰度转换、MOG2 前景提取、阈值 200 去阴影、形态学开运算与膨胀、轮廓检测、面积过滤；启动预热 30 帧，并要求连续 3 帧检出才输出检测框。`analyze()` 唯一可调参数是 `minTargetArea`，history/varThreshold/预热帧数/确认帧数都硬编码。
- `src/core/analysis/OnnxObjectDetector.h`：可选 ONNX Runtime 目标检测器，负责加载 ONNX 模型、图像预处理、推理和基础 YOLO 类后处理；未配置 ONNX Runtime 时会安全返回错误并保持禁用。
- `src/core/analysis/RoiRule.h`：矩形 ROI 规则结构，包含 `id`、`sourceId`、`ruleName`、归一化 `rect`、`enabled`、`cooldownSeconds` 和时间字段。
- `src/core/alarm/AlarmEvent.h`：告警事件数据结构，包含 `id`、`sourceId`、`sourceName`、`eventType`、`level`、`message`、`snapshotPath`、`videoClipPath`、`confirmed`、`timestamp`。
- `src/core/alarm/AlarmManager.h`：统一负责 `MotionDetected` / `IntrusionDetected` / `PersonDetected` / `VehicleDetected` 告警生成、冷却、自动截图、写入 `alarm_events` 和发出实时告警信号；运动告警默认同源冷却 10 秒，入侵告警按 source + ROI rule 使用每条规则的 `cooldownSeconds`，AI 告警按 source + eventType 冷却。
- `src/core/storage/AlarmEventRepository.h`：封装 `alarm_events` 表的加载、新增、确认和删除操作。
- `src/core/storage/DatabaseManager.h`：负责打开并初始化 SQLite 数据库（主程序传入 `AppPaths::databasePath()`；核心类的 `data/app.db` 默认参数仅供测试使用）。当前建表：`video_sources`、`alarm_events`、`roi_rules`、`recordings`、`app_settings`、`channel_bindings`、`source_analysis_settings`，以及旧 ROI 冷备份表 `legacy_roi_rules`。用 `PRAGMA user_version` 管理 schema 版本，升级前先备份为 `<db>.bak-v<N>-<时间戳>`；连接启用 `foreign_keys`、WAL 和 `busy_timeout`，连接名带 UUID（不再是固定名）。
- `src/core/storage/RoiRuleRepository.h`：封装 `roi_rules` 表的增删改查，矩形 ROI 坐标以归一化 `x/y/width/height` 写入 `points_json`。
- `src/core/storage/VideoSourceRepository.h`：封装 `video_sources` 表的加载、新增、删除和改名操作；相比早期版本新增 `credential_ref`、`reconnect_max_attempts`、`reconnect_interval_ms`、`connect_timeout_ms`、`read_timeout_ms` 字段，并提供 `secureLegacyNetworkEndpoints()` 清洗历史网络源凭据。
- `src/core/storage/ChannelBindingRepository.h`：封装 `channel_bindings` 表（`channel_index` 0-3 主键、`source_id` 唯一外键并级联删除），提供 `loadAll`、`bind`、`unbindChannel`、`unbindSource`。`bind` 是替换语义：先删除同通道与同源的旧行再插入，因此一通道一源、一源一通道。
- `src/core/security/CredentialStore.h`：管理 RTSP/HTTP 凭据。reference 形如 `VideoAnalysisPlatform/source/<uuid>`；Windows 下写入 Credential Manager（`CRED_TYPE_GENERIC`，blob 为 `endpoint\\x1fusername\\x1fpassword`），入库 URL 由 `sanitizeEndpoint()` 脱敏（清空 user/password、query 置 `[redacted]`）。非 Windows 平台三个方法一律返回失败并报“当前平台不支持 Windows Credential Manager”。
- `src/core/video/Frame.h`：视频帧结构，包含 `image`、`timestamp`、`width`、`height`、`frameIndex`。
- `src/core/video/FrameQueue.h`：线程安全帧队列，支持固定最大容量、push/pop/tryPop/clear/size、关闭唤醒和 `droppedFrames` 统计。
- `src/core/video/IVideoDecoder.h`：视频解码器接口，当前由 `OpenCVVideoDecoder` 和 `FFmpegVideoDecoder` 实现。
- `src/core/video/OpenCVVideoDecoder.h`：使用 OpenCV `VideoCapture` 读取本地 mp4，并将 `cv::Mat` 转换为 `QImage`。
- `src/core/video/FFmpegVideoDecoder.h`：使用 FFmpeg 打开本地视频或 RTSP 流，读取视频流信息，解码视频帧并转换为 RGBA `QImage`；支持由 `StreamSession` 传入停止中断标记；未配置 FFmpeg 时会安全返回错误。
- `src/core/video/RecordingManager.h`：负责手动录像文件创建、写帧和关闭；默认保存到 `data/records/YYYY-MM-DD/`（主程序实际传入 `AppPaths::recordingDirectory()`），使用 OpenCV `VideoWriter` 写 MJPG `.avi` 文件，未启用 OpenCV 时返回错误。录像过程不写数据库，`stopRecording()` 恒返回成功。
- `src/core/video/SnapshotManager.h`：负责将 `QImage` 保存到本地截图文件，默认目录为 `data/snapshots/YYYY-MM-DD/`（主程序实际传入 `AppPaths::snapshotDirectory()`），文件名包含 sourceId、时间和事件类型。
- `src/core/video/StreamSession.h`：单路视频播放会话，负责启动、暂停、停止、状态、FPS、后台解码线程生命周期、RTSP 重连状态机、手动截图/录像触发、基础运动检测结果发布、ROI 列表加载/保存、ONNX 模型加载和异步 AI 推理调度，并在检测到运动、ROI 入侵或 AI 目标时发出 `motionDetected` / `intrusionDetected` / `objectDetected` 信号。`openSource()` 用于打开绑定到本通道的视频源，并据此维护 `sourceId`：经绑定打开时是 `video_sources.id`，走 `openLocalFile` / `openRtspUrl` / `openHttpUrl` 直连时仍是 `stream1`..`stream4` 的通道号（`sourceIdFromStreamId`）。
- `src/core/video/StreamSessionManager.h`：统一创建并持有 4 个 `StreamSession`，向 QML 提供 `sessionAt(index)` 和 `stopAll()`，并负责把每路 `StreamSession` 的运动、入侵和 AI 目标信号连接到 `AlarmManager`（在构造函数中完成）。额外提供 `bindSource(channelIndex, sourceId)`、`boundSourceId(channelIndex)`、`refreshSource(sourceId)` 承担通道↔视频源绑定：构造时从 `channel_bindings` 恢复绑定，绑定/刷新时会为 RTSP/HTTP 源取回 `CredentialStore` 中的凭据。
- `src/core/video/StreamPerformance.h`：单路性能快照结构，包含 `sourceName`、`fps`、`decodeTimeMs`、`queueSize`、`droppedFrames`、`status`。
- `src/core/video/VideoSource.h`：视频源数据结构、视频源类型和状态枚举。
- `src/models/PerformanceModel.h`：继承 `QAbstractListModel` 的性能监控模型，每秒从 `StreamSessionManager` 采样一次所有 `StreamSession` 的性能快照。
- `src/models/AlarmEventModel.h`：继承 `QAbstractListModel` 的告警事件列表模型，通过 `AlarmEventRepository` 加载和保存数据，向 QML 暴露 `alarmId`、`sourceId`、`sourceName`、`eventType`、`level`、`message`、`snapshotPath`、`videoClipPath`、`confirmed`、`timestamp` 等角色。
- `src/models/VideoSourceModel.h`：继承 `QAbstractListModel` 的视频源列表模型，通过 `VideoSourceRepository` 加载和保存数据，向 QML 暴露 `sourceId`、`name`、`type`、`url`、`enabled`、`sourceEnabled`、`status`、`lastError`、`credentialRef`、`reconnectMaxAttempts`、`reconnectIntervalMs` 等角色。它本身**没有**任何绑定 API，通道绑定由 `StreamSessionManager` 负责。
- `src/models/RecordingModel.h`：继承 `QAbstractListModel` 的录像索引模型。`reindex()` 扫描录像目录下的 `.avi` 并写入 `recordings` 表占位索引行，`loadFromDatabase()` 读取索引并跳过磁盘上已不存在的文件；暴露 `recordingId`、`sourceId`、`sourceName`、`path`、`startedAt`、`endedAt`、`durationMs`、`width`、`height`、`fps`、`status`。目前没有删除 API，也没有清理孤儿记录的逻辑。
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
- `qml/Main.qml`：当前主窗口，负责整体布局、左侧 6 项导航（首页/视频墙/视频源/告警中心/回放/设置）和 `StackLayout` 页面切换，并通过 7 个 `required property var` 接收 C++ 注入的根属性。
- `qml/StartupError.qml`：启动失败页，展示 `startupController.errorMessage` 和数据目录，提供“打开数据目录 / 退出 / 重试”三个按钮（重试会重启进程）。
- `PROJECT_REQUIREMENTS.md`：完整需求、路线图、推荐目录结构和阶段任务来源。
- `README.md`：项目简介与本地构建命令。
- `docs/`：后续项目文档目录，目前未填充。
- `src/app/`：应用层，含 `AppPaths`（数据目录解析与旧数据迁移）、`SettingsController`（设置页后端）、`StartupController`（启动失败页后端）；`src/core/`、`src/models/`、`src/render/`：核心、模型和渲染模块。
- `qml/pages/`：6 个页面均为真实实现。`DashboardPage.qml` 为总览页（通道数/告警数/录像索引数 + 通道状态列表）；`VideoWallPage.qml` 为 1/4 宫格视频墙；`SourceManagerPage.qml` 绑定 C++ `VideoSourceModel` 并负责添加/删除/改名视频源与通道绑定；`AlarmCenterPage.qml` 绑定 C++ `AlarmEventModel`；`PlaybackPage.qml` 为本地录像回放页（QtMultimedia）；`SettingsPage.qml` 为设置页。
- `qml/components`：基础组件目录，当前包含导航项、工具栏、默认使用 `VideoRenderItem` 且保留旧 `Image` fallback 的视频播放卡片 `VideoTile.qml`、ROI 绘制组件 `RoiEditor.qml`，以及性能面板 `PerformancePanel.qml`；`VideoTile.qml` 同时提供运动检测、AI 检测、ONNX 模型选择、阈值和推理间隔控件。`PagePlaceholder.qml` 仍留在目录并在 CMake 中登记，但已无任何引用。
- `qml/styles/AppTheme.qml`：统一管理颜色、间距、圆角和字体大小。
- `build/`：构建产物目录，已被 `.gitignore` 忽略（`build/` 与 `build*/` 均忽略，构建日志 `*.log` 也已忽略）；不要把其中内容当作源码或稳定测试基线。`data/` 是早期版本的运行时数据目录，主程序现已改用 `AppPaths` 的 `AppLocalDataLocation`；核心类中保留的 `data/...` 默认参数仅供测试使用。

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

1. `src/main.cpp` 创建 `QGuiApplication`，并把 `organizationName` / `applicationName` 都设为 `VideoAnalysisPlatform`。
2. 创建 `QQmlApplicationEngine`，创建 `VideoFrameProvider` 并注册为 `image://videoFrame`。
3. 创建 `AppPaths` 并调用 `prepare()`：确定数据根目录（`QStandardPaths::AppLocalDataLocation`），在目标 `app.db` 不存在时尝试迁移旧布局数据，并创建 `snapshots/`、`records/`、`models/`、`logs/`。
4. 创建 `DatabaseManager` 并 `open(appPaths.databasePath())`；随后创建 `VideoSourceRepository` 并调用 `secureLegacyNetworkEndpoints()` 清洗历史网络源凭据。
5. 步骤 3 或 4 失败时，只注入 `startupController` 并加载 `qml/StartupError.qml`，结束启动流程（不创建任何后续对象）；若该页面自身也创建失败，则 `exit(-1)`。
6. 成功则依次创建 `ChannelBindingRepository`、`AlarmEventRepository`、`RoiRuleRepository`、`VideoSourceModel`、`AlarmEventModel`（并设置媒体目录为截图目录）、`RecordingModel`（构造后立即 `reindex()`）、`SettingsController`、`AlarmManager`、`StreamSessionManager`、`PerformanceModel`。
7. `AlarmManager::alarmCreated` 连接到 `AlarmEventModel::addAlarm`；`VideoSourceModel::sourceChanged` 连接到 `StreamSessionManager::refreshSource`，使视频源被改名、禁用或删除时联动对应会话。
8. 通过 QML 根对象初始属性注入 7 个对象：`videoSourceModel`、`alarmEventModel`、`streamSessionManager`、`performanceModel`、`recordingModel`、`appPaths`、`settingsController`，然后加载 `qrc:/qt/qml/VideoAnalysisPlatform/qml/Main.qml`。
9. QML 创建 `ApplicationWindow`，显示左侧导航并通过 `StackLayout` 切换 6 个页面。
10. `VideoSourceModel` 启动时从 `video_sources` 表加载历史视频源；`AlarmEventModel` 启动时从 `alarm_events` 表加载历史告警；`RecordingModel` 从 `recordings` 读取索引并跳过磁盘上已不存在的文件；`VideoWallPage.qml` 通过 `StreamSessionManager.sessionAt(index)` 为每个 `VideoTile.qml` 绑定不同 `StreamSession`。
11. 通道绑定：用户在 `SourceManagerPage.qml` 选择通道后点 Bind，或在 `VideoWallPage.qml` 选择视频源后点 “Bind and play”，都会调用 `streamSessionManager.bindSource(channelIndex, sourceId)`。绑定写入 `channel_bindings`（0-3 对应 `stream1`-`stream4`），`StreamSession` 经 `openSource()` 打开该源后 `sourceId` 变为 `video_sources.id`，并按该 id 重新加载 ROI。`StreamSessionManager` 构造时会从 `channel_bindings` 恢复绑定；绑定/刷新 RTSP、HTTP 源时会先从 `CredentialStore` 取回真实 URL（入库 URL 是脱敏的）。`VideoTile.qml` 仍保留不经绑定的直连入口（`openLocalFile` / `openRtspUrl` / `openHttpUrl`），这些路径下 `sourceId` 仍是通道号。
12. `StreamSession` 根据当前 `VideoSourceType` 选择解码器：本地文件默认使用 `OpenCVVideoDecoder`，RTSP/HTTP 使用 `FFmpegVideoDecoder`。解码线程读取帧后写入 `FrameQueue`；分发线程从 `FrameQueue` 取帧后通过 queued invoke 更新 `VideoFrameProvider` 中的当前帧和 `frameSource`。RTSP 连续读帧或打开失败时进入 `Reconnecting`，按 `reconnectIntervalMs` 后台重试，超过 `maxReconnectAttempts` 后进入 `Error`。
13. `VideoTile.qml` 默认使用 `VideoRenderItem` 显示视频；`VideoRenderItem` 绑定对应 `StreamSession`，在 `frameSourceChanged` 时读取 `StreamSession.currentFrame()`，并在 C++ 中绘制视频帧、检测框、ROI、FPS/状态和录像状态。旧 `image://videoFrame` + `Image` 路径仍保留，可通过 `VideoTile.useRenderItem = false` 作为 fallback。
14. `PerformanceModel` 约每秒读取一次每路 `StreamSession` 的 `StreamPerformance` 快照，并驱动 `PerformancePanel.qml` 显示 `sourceName`、`fps`、`decodeTimeMs`、`queueSize`、`droppedFrames`、`status`。
15. 用户在 `VideoTile.qml` 点击截图时，QML 调用 `StreamSession.takeSnapshot("jpg", "manual")`；`StreamSession` 从 `VideoFrameProvider` 拷贝当前帧，并让 `SnapshotManager` 在后台线程保存图片，完成后通过 `snapshotFinished(success, filePath, errorMessage)` 返回结果。
16. 用户在 `VideoTile.qml` 点击录像时，QML 调用 `StreamSession.startRecording/stopRecording`；`StreamSession` 使用当前帧尺寸和源 FPS 创建 `RecordingManager` 录像文件，并在解码线程每帧写入，停止播放、切换源、视频结束或错误时会关闭文件。录像过程不写数据库。
17. 用户在 `VideoTile.qml` 输入 RTSP 地址并点击“打开 RTSP”时，QML 调用 `StreamSession.openRtspUrl`，成功后复用 `play/pause/stop` 控制；RTSP 打开或读取失败会进入 `Reconnecting` 并显示当前重连次数，用户点击停止会中断重连。
18. 用户在 `VideoTile.qml` 开启运动检测时，QML 设置 `StreamSession.motionDetectionEnabled` 和 `minTargetArea`；`StreamSession` 在后台解码线程调用 `MotionDetector`，再把 `DetectionResult` 中的 boxes 转成归一化坐标通过 `detectionBoxes` 暴露给 `VideoRenderItem` 绘制，同时把检测框和 ROI 快照交给 `IntrusionDetector` 做中心点命中判断。
19. 用户在 `VideoTile.qml` 选择 ONNX 模型时，QML 调用 `StreamSession.loadAiModel`；模型加载和 AI 推理都在独立 AI 工作线程中执行。播放时 `StreamSession` 按 `aiInferenceInterval` 抽帧调度 `OnnxObjectDetector`，结果通过 `objectDetectionBoxes` 暴露给 `VideoRenderItem` 绘制。未配置 ONNX Runtime 时模型加载会返回降级错误，不影响运动检测和播放。
20. 若运动检测框非空，`StreamSession` 发出 `motionDetected`；`AlarmManager` 按 sourceKey 做冷却判断，保存自动截图，创建 `MotionDetected` 告警并写入 `alarm_events`；写库成功后发出 `alarmCreated`，`AlarmEventModel::addAlarm` 将新告警实时插入告警中心列表。
21. 若运动检测框中心点进入某条启用 ROI，`StreamSession` 发出 `intrusionDetected`；`AlarmManager` 按 source + ROI rule 和该规则的 `cooldownSeconds` 做冷却判断，保存自动截图，创建 `IntrusionDetected` 告警并写入 `alarm_events`。
22. 若 AI 检测到 person 或车辆类目标，`StreamSession` 发出 `objectDetected`；`AlarmManager` 按 source + eventType 做冷却判断，保存自动截图，创建 `PersonDetected` 或 `VehicleDetected` 告警并写入 `alarm_events`。
23. 用户在 `VideoTile.qml` 开启“绘制 ROI”后，`RoiEditor.qml` 根据 `VideoRenderItem` 或 fallback `Image` 的 `paintedWidth` / `paintedHeight` 把鼠标拖拽矩形转换为 0 到 1 的归一化视频坐标，并调用 `StreamSession.addRoiRect`；`StreamSession` 通过 `RoiRuleRepository` 保存到 `roi_rules`，`roiRules` 变化后由 `VideoRenderItem` 重新绘制 ROI。删除/清空 ROI 也由 `StreamSession` 同步删除数据库记录。
24. 用户在 `AlarmCenterPage.qml` 点击“添加测试告警”“确认”“删除”时，QML 只调用 `AlarmEventModel` 暴露的方法，实际 SQLite 写入、更新和删除由 `AlarmEventRepository` 负责。
25. 用户在 `PlaybackPage.qml` 中通过 `RecordingModel` 浏览录像索引，并用 QtMultimedia `MediaPlayer` + `VideoOutput` 播放本地文件；页面提供播放/暂停/停止、跳过 ±10 秒、进度拖动，以及 Reindex / Reload 按钮刷新索引。
26. 用户在 `SettingsPage.qml` 中查看受管目录并用 Open 按钮打开，或修改磁盘告警阈值（该值经 `SettingsController` 存入 QSettings）。
27. 若 `Main.qml` 根对象创建失败，程序以错误码 -1 退出。

## 代码约定

- 每次只完成一个明确阶段或子任务，避免一次性跨阶段实现。
- QML 只负责界面、布局和交互展示，复杂业务逻辑放到 C++ 层。
- 不要把后续业务逻辑继续堆进 `qml/Main.qml`；页面内容放在 `qml/pages`，复用控件放在 `qml/components`，视觉常量放在 `qml/styles/AppTheme.qml`。
- 视频源数据增删改由 `VideoSourceModel` 负责，并通过 `VideoSourceRepository` 持久化；QML 页面只展示模型数据并调用模型暴露的方法。
- 每路播放由独立 `StreamSession` 负责状态和线程生命周期，QML 只调用对应 session 的 `openLocalFile`、`openRtspUrl`、`openHttpUrl`、`play`、`pause`、`stop`、`startRecording`、`stopRecording` 并展示帧、FPS、重连状态、录像状态和错误信息。经通道绑定打开视频源必须走 `StreamSessionManager::bindSource`，不要从 QML 直接调用 session 的 `openSource`。当前会话分支为 RTSP/HTTP 使用 FFmpeg、其他类型使用 OpenCV；不要据 `FFmpegVideoDecoder` 类本身支持本地输入而改变会话选择逻辑。
- 多路播放由 `StreamSessionManager` 统一管理 session 数量；当前固定 4 路，不要在当前阶段顺手扩展 9/16 路。
- `StreamSession` 必须通过 `FrameQueue` 解耦解码和画面分发；队列最大容量当前在 `StreamSession.cpp` 中为 3，停止 session 时必须关闭并清空队列。RTSP 重连只允许影响当前 session，不能阻塞 UI，也不能改变本地 mp4 的 EOF/错误处理逻辑。
- `VideoTile.qml` 默认使用 `VideoRenderItem` 渲染视频帧、检测框、ROI 和基础状态叠加；旧 `Image` 方案通过 `useRenderItem = false` 保留为 fallback。后续不要把大量逐帧矩形 Repeater 逻辑重新堆回 QML。
- 性能面板数据由 C++ `PerformanceModel` 按约 1 秒频率采样，不要在 QML 中按帧计算或刷新性能列表。
- 截图保存由 C++ `SnapshotManager` 负责；QML 只触发手动截图并展示 `snapshotFinished` 返回的成功路径或错误信息。告警自动截图只能通过 `AlarmManager` 触发，并由 `AlarmManager` 把成功路径写入告警事件。
- 手动录像由 C++ `RecordingManager` 负责；QML 只触发开始/停止并展示 `recordingResult` 返回的成功路径或错误信息。录像写入 `<AppPaths::recordingDirectory()>/YYYY-MM-DD/*.avi`，录像过程不写数据库，不做告警前后录像。`recordings` 表的索引行由 `RecordingModel::reindex()` 扫描磁盘 `.avi` 生成，不要把它当成录像流程中的入库实现。
- 基础运动检测由 C++ `MotionDetector` 负责（MOG2 背景建模）；QML 只设置开关和 `minTargetArea`，检测框默认由 `VideoRenderItem` 根据 `detectionBoxes` 绘制，fallback `Image` 模式下才使用 QML 矩形叠加。关闭检测时必须清空检测框，修改 `minTargetArea` 会重置背景模型。`MotionDetector` 不能直接操作数据库或告警模型，只能由 `StreamSession` 通知 `AlarmManager`。
- ONNX AI 检测由 C++ `OnnxObjectDetector` 和 `StreamSession` 的 AI 工作线程负责；QML 只选择模型、设置 `aiDetectionEnabled`、`aiConfidenceThreshold` 和 `aiInferenceInterval`，AI 框默认由 `VideoRenderItem` 根据 `objectDetectionBoxes` 绘制。`OnnxObjectDetector` 不能直接操作数据库或告警模型，只能由 `StreamSession` 通知 `AlarmManager`。未配置 ONNX Runtime 时必须保持项目可构建、可运行，且不能影响运动检测。
- ROI 入侵判断由 C++ `IntrusionDetector` 负责；它只接收检测框、帧尺寸和 ROI 规则快照，第一版只判断 box 中心点是否落入矩形 ROI，不能写数据库或直接生成告警。
- 告警事件增删改查由 `AlarmEventModel` 和 `AlarmEventRepository` 负责；QML 页面只展示列表并调用模型方法。`MotionDetected`、`IntrusionDetected`、`PersonDetected` 和 `VehicleDetected` 自动告警由 `AlarmManager` 统一生成、截图和写库。
- ROI 规则由 `RoiRuleRepository` 持久化到 `roi_rules`，QML 通过 `RoiEditor.qml` 绘制，只调用 `StreamSession.addRoiRect/removeRoiRule/clearRoiRules`。默认渲染模式下已有 ROI 由 `VideoRenderItem` 绘制，`RoiEditor.qml` 主要负责拖拽草稿和编辑时的删除手柄。`roi_rules.source_id` 是指向 `video_sources(id)` 的外键（`ON DELETE CASCADE`）：经通道绑定打开视频源时按 `video_sources.id` 存取 ROI；走 `VideoTile.qml` 直连路径时仍按 1-4 通道号存取，此时若对应 `video_sources.id` 不存在会违反外键约束导致新增失败，这是已知遗留。
- C++ 模块按职责放置：应用控制在 `src/app`，核心处理在 `src/core`，QML 数据模型在 `src/models`，渲染相关在 `src/render`。
- 视频源与视频墙通道的绑定已实现：`channel_bindings`（`channel_index` 0-3 主键、`source_id` 唯一外键）+ `StreamSessionManager::bindSource/boundSourceId/refreshSource`，0-3 对应 `stream1`-`stream4`，一源一通道，`VideoSourceModel::sourceChanged` 会联动 `refreshSource`。但 `VideoTile.qml` 仍保留按通道号直连的 `openLocalFile` / `openRtspUrl` / `openHttpUrl` 路径（配合 `sourceIdFromStreamId` 回退），这部分没有走绑定；改动绑定或 ROI 归因逻辑时必须同时考虑这条遗留链路。
- `DatabaseManager` 创建的 SQLite 连接和各 Repository 只在创建连接的主线程使用；不要从解码、截图或 AI 工作线程直接访问 Repository。schema 现在基于 `PRAGMA user_version` 做版本管理（`CurrentSchemaVersion`），升级前会自动备份为 `<db>.bak-v<N>-<时间戳>`；新增表或列时必须同步更新 `migrate()` 并提升版本号，且要为已有库设计 `ALTER TABLE` 或数据搬迁路径。
- 当前已建表：`video_sources`、`alarm_events`、`roi_rules`、`recordings`、`app_settings`、`channel_bindings`、`source_analysis_settings`，以及旧 ROI 冷备份表 `legacy_roi_rules`。其中 `app_settings` 和 `source_analysis_settings` 目前只建表、没有任何读写方（设置项实际存 QSettings，运动/AI 参数是 `StreamSession` 的运行时属性），新增表前先确认没有与它们重复。
- 不要擅自新增大型依赖或公共 API；当前允许使用 OpenCV 本地 mp4 解码、FFmpeg RTSP 解码和可选 ONNX Runtime 目标检测。ONNX Runtime 必须保持可选依赖，未找到时走降级路径。
- 修改复杂模块时同步更新 `README.md` 或 `docs/` 中长期有价值的说明。

## 禁止随意修改

- 不要在当前阶段扩展硬件解码或 9/16 路视频墙。“录像入库”目前只是 `RecordingModel::reindex()` 的磁盘扫描占位索引（`source_id` 固定 0、无时长/分辨率/帧率），不要在录像过程中直接写库，也不要把它当作完整录像入库能力。
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
- QML/手工验证：窗口标题正常，左侧导航可以切换页面；6 个页面均为真实页面。程序数据默认位于 `QStandardPaths::AppLocalDataLocation`（Windows 约 `%LOCALAPPDATA%\VideoAnalysisPlatform\VideoAnalysisPlatform\`），包含 `app.db`、`snapshots/`、`records/`、`models/`、`logs/`；首次启动若检测到旧布局 `data/` 目录会尝试自动迁移。视频源页可以添加本地或 RTSP/HTTP 源、删除源、修改源名称，列表自动刷新，并可按通道绑定视频源（Bind）；重启后视频源列表和通道绑定分别从 `video_sources`、`channel_bindings` 恢复。告警中心可以添加测试告警、确认告警、删除告警，重启程序后历史告警从 `alarm_events` 恢复。视频墙页可以在 1 宫格和 4 宫格之间切换，每个格子可以独立选择本地 mp4 或输入 RTSP 地址，并通过播放、暂停、停止按钮观察独立状态和 FPS；默认 `VideoRenderItem` 应能显示视频帧、左上角状态/FPS 叠加、运动检测框、AI 检测框和 ROI，必要时可把 `VideoTile.useRenderItem` 设为 `false` 验证旧 `Image` fallback。未配置 FFmpeg 时打开 RTSP 应显示“FFmpeg 未启用”类错误；未配置 ONNX Runtime 时加载 ONNX 模型应显示降级错误且不影响运动检测；右侧性能面板显示所有通道的 `sourceName`、`fps`、`decodeTimeMs`、`queueSize`、`droppedFrames`、`status` 并约每秒更新。播放时点击截图按钮后，应在 `<数据根目录>/snapshots/YYYY-MM-DD/` 生成 jpg 文件；播放时点击录像按钮后，应显示 `REC`，停止后应在 `<数据根目录>/records/YYYY-MM-DD/` 生成可播放的 avi 文件。开启运动检测后，画面有明显变化时应出现黄色矩形框（注意需先经过 30 帧预热和连续 3 帧确认），并在告警中心实时出现 `MotionDetected` 告警；配置 ONNX Runtime 和真实模型后，才验证类别/置信度、`PersonDetected` / `VehicleDetected` 告警冷却。点击“绘制 ROI”后在实际视频画面区域拖拽应出现蓝色 ROI 框，ROI 应与视频画面缩放位置一致，并写入当前 `app.db` 的 `roi_rules`；重新绑定同一视频源后其 ROI 应恢复；删除/清空 ROI 可立即移除界面和数据库记录。注意：未绑定通道走直连路径时按 1-4 通道号存取 ROI，若对应 `video_sources.id` 不存在会因外键约束新增失败。回放页应能列出 `records/` 下的 `.avi`（来自启动时或手动触发的 `reindex()` 索引）并点击播放；首页应显示通道数、告警数和录像索引数；设置页应显示受管目录并可用 Open 打开，修改磁盘告警阈值后写入 QSettings（该阈值目前没有运行时消费方）。
- QML 静态检查：`cmake --build build --config Release --target all_qmllint`；必须先完成 Qt/CMake 配置并生成该目标。

## 当前风险和不确定信息

- 当前工作区存在 `.git` 目录，`git status` 可用；开始修改前仍需检查未提交改动，避免覆盖用户或其他工具的工作。
- 当前终端是否能直接找到 `cmake.exe`、Qt 6 安装路径以及 Windows SDK 访问权限，取决于本机 VS2022/Qt 环境配置。
- 现有 `build/CMakeCache.txt` 是环境快照而不是本轮重新配置结果：截至 2026-08-10，该缓存使用 Qt 6.11.0 和 OpenCV 4.13.0，未找到 FFmpeg 和 ONNX Runtime。重新配置后必须以 CMake 输出和目标宏为准；缺失可选依赖时对应能力应返回降级错误，不影响基础 QML 启动。
- `docs/` 目前为空预留目录；`src/app/`（`AppPaths`、`SettingsController`、`StartupController`）、`src/core/`、`src/models/`、`src/render/` 和 `tests/` 均已实现。不要把历史 `build/` 产物或旧 `data/` 数据库当作当前源码/测试结论。
- `DatabaseManager::open()` 失败会中断启动并进入 `StartupError.qml`；数据库打不开时优先检查 `AppPaths::databasePath()` 指向的文件、SQLite 驱动和 `DatabaseManager` 日志。schema 已改用 `user_version` 做版本管理并在迁移前备份，改表时仍必须同步更新 `migrate()`。连接名现在带 UUID，多实例冲突问题已缓解。
- `main.cpp` 有两处启动失败分支：第一处（`AppPaths`/`DatabaseManager` 失败）带有 `StartupError.qml` 加载失败的 `exit(-1)` 兜底连接，第二处（`secureLegacyNetworkEndpoints()` 失败）没有。不确定是有意还是遗漏；若该页面加载失败，第二处可能无窗口停留在事件循环。
- 主程序运行时路径已固定为 `AppPaths` 的 `AppLocalDataLocation`，不再是相对工作目录；但核心类中仍保留 `data/app.db`、`data/snapshots/`、`data/records/` 默认参数，仅供测试使用。改动这些默认值时要同时检查测试和 `AppPaths` 的旧数据迁移候选清单。
- QML/GUI 没有自动化测试；四格布局在小窗口、ROI 与视频黑边对齐、VideoRenderItem 长时间资源占用仍需手工验证。
- ONNX 后处理按当前实现的常见 YOLO 输出和类别映射处理，未承诺通用模型格式；真实模型需验证输出布局、坐标映射、类别和置信度阈值。
- 运动检测告警当前只基于“检测框非空”触发，默认同源冷却 10 秒；ROI 入侵告警基于 box 中心点是否进入矩形 ROI。AI 告警只对 person 和车辆类（id 2/3/5/7 或类名含 car/bus/truck/motorcycle）生成 `PersonDetected` / `VehicleDetected`；`source_analysis_settings.ai_alert_categories` 默认列出的 6 个类别**没有被任何代码读取**，因此 `bicycle` 既不在 id 列表也不匹配车辆类名，会落到 `ObjectDetected` 而不产生告警。NMS 阈值是编译期常量 0.45，未使用 `ai_nms_threshold`。尚未实现多边形 ROI、告警过滤、告警详情预览或自定义类别告警策略。
- ROI 归因已部分打通：经通道绑定打开的视频源按 `video_sources.id` 存取 ROI；但 `VideoTile.qml` 直连路径仍按 1-4 通道号存取，两条路径会看到不同的 ROI 集合，且直连路径下新增 ROI 可能因外键约束失败。另外 `DatabaseManager::migrateLegacyRoiRules()` 会把旧库的通道级 ROI 整体搬进 `legacy_roi_rules` 后 `DROP TABLE roi_rules` 并重建空表，而**没有任何代码读回或回填**（`assigned_source_id` 始终为 NULL）：旧库升级后这些 ROI 不会自动出现在应用里，需要人工从 `legacy_roi_rules` 恢复或重新绘制。
- 解绑没有 UI 入口：`unbindSource()` 无调用者，`unbindChannel()` 只在 `refreshSource()` 处理源被删除或禁用时使用；解绑后 `StreamSession` 内存中的 `m_sourceId` 和旧 ROI 不会被清空。
- RTSP 自动重连的间隔和最大次数现在取自 `video_sources.reconnect_interval_ms` / `reconnect_max_attempts`（默认 3000 ms / 3 次）。`connect_timeout_ms` 目前没有任何消费者；`read_timeout_ms` 虽被 `FFmpegVideoDecoder` 读取，但 `StreamSession::play()` 构造 `VideoSource` 时没有拷贝这两个超时字段，解码器实际始终拿到默认 10000 ms——即这两个字段当前对播放没有实际效果。现有缓存未找到 FFmpeg 开发包，仍需要在具备真实 RTSP 流和 FFmpeg DLL 的环境下做断线/恢复集成验证。
- 手动录像当前依赖 OpenCV `VideoWriter` 的 MJPG AVI 编码；不同 Windows 机器的播放器/编解码器支持可能有差异，仍需要用真实播放场景验证生成文件兼容性。
- `recordings` 表目前只有 `RecordingModel::reindex()` 的磁盘扫描写入方：`source_id` 固定 0、`source_name` 固定为 `Indexed recording`、`status` 恒为 `Completed`、时长/分辨率/帧率恒为 0；没有 `DELETE` 语句，磁盘文件被删除后数据库行会永久残留（`loadFromDatabase()` 只是跳过显示）。`RecordingManager::stopRecording()` 恒返回成功，录像中断也不会反映到 `status`。
- `CredentialStore` 依赖 Windows Credential Manager，非 Windows 平台 `write`/`read`/`remove` 一律失败；而 `VideoSourceModel::persistCredential()` 对 RTSP/HTTP 源是**无条件**写入，因此非 Windows 平台上连不带凭据的网络源也无法保存。
- `app_settings` 和 `source_analysis_settings` 是只建表、无任何读写方的空表：设置项实际存 QSettings（`diskWarningThresholdPercent` 也还没有运行时消费方），运动/AI 参数是 `StreamSession` 的运行时属性，既不持久化，也不会在切换视频源时重置。
- `VideoRenderItem` 当前为第一版 `QQuickPaintedItem` / `QPainter` 实现，目标是减少 QML 层逐帧对象更新并保持功能正确；尚未做 GPU 纹理零拷贝，仍需要用真实四路视频长时间播放验证 UI 响应和资源占用。
