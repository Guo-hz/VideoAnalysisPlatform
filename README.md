# 实时多路视频流智能分析平台

这是一个基于 C++17、Qt 6、QML 和 CMake 的桌面端项目。当前已完成“阶段 18：ONNX Runtime AI 检测”，包含主程序入口、QML 主窗口、左侧导航、基础页面、视频源列表模型、SQLite 持久化、最多四路本地 mp4 播放骨架、FFmpeg/RTSP 解码入口、RTSP 自动重连、用于解码和画面分发解耦的线程安全帧队列、按秒刷新的视频流性能监控面板、手动截图保存能力、手动录像能力、基于 OpenCV 帧差法的基础运动检测框叠加、告警事件表、仓库、列表模型、告警中心列表、运动检测自动告警、视频画面上的矩形 ROI 绘制、ROI 数据库存储、基于 ROI 中心点命中的区域入侵告警、自定义 VideoRenderItem，以及可选 ONNX Runtime 目标检测、AI 框叠加和 Person/Vehicle 告警。

当前不包含 9/16 路视频墙、硬件解码、告警前后录像或录像数据库记录相关实现。

## 环境要求

- Visual Studio 2022
- Qt 6，建议安装 MSVC 2022 64-bit 套件
- CMake 3.21 或更高版本
- OpenCV C++ 开发包。未配置 OpenCV 时项目仍可编译运行，但视频播放页会提示 OpenCV 未启用。
- FFmpeg C/C++ 开发包。未配置 FFmpeg 时项目仍可编译运行，但 RTSP 播放会提示 FFmpeg 未启用。
- ONNX Runtime C/C++ 开发包。未配置 ONNX Runtime 时项目仍可编译运行，AI 检测会优雅降级为禁用状态，不影响运动检测。

## 构建方式

请将下面命令中的 `C:\Qt\6.x.x\msvc2022_64` 替换为本机实际 Qt 6 安装路径。如需启用本地视频播放，请同时设置 `OpenCV_DIR`，其值应指向包含 `OpenCVConfig.cmake` 的目录。如需启用 RTSP/FFmpeg，请设置 `FFMPEG_ROOT`，该目录下应包含 `include/libavformat/avformat.h` 和对应 `lib` 目录。如需启用 ONNX AI 检测，请设置 `ONNXRUNTIME_ROOT`，该目录下应包含 ONNX Runtime 的 `include` 和 `lib` 目录。

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="C:\Qt\6.x.x\msvc2022_64"
cmake --build build --config Release
.\build\Release\VideoAnalysisPlatform.exe
```

启用 OpenCV 的示例：

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="C:\Qt\6.x.x\msvc2022_64" -DOpenCV_DIR="C:\opencv\build"
cmake --build build --config Release
.\build\Release\VideoAnalysisPlatform.exe
```

启用 OpenCV 和 FFmpeg 的示例：

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="C:\Qt\6.x.x\msvc2022_64" -DOpenCV_DIR="C:\opencv\build" -DFFMPEG_ROOT="C:\ffmpeg"
cmake --build build --config Release
.\build\Release\VideoAnalysisPlatform.exe
```

启用 OpenCV、FFmpeg 和 ONNX Runtime 的示例：

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="C:\Qt\6.x.x\msvc2022_64" -DOpenCV_DIR="C:\opencv\build" -DFFMPEG_ROOT="C:\ffmpeg" -DONNXRUNTIME_ROOT="C:\onnxruntime"
cmake --build build --config Release
.\build\Release\VideoAnalysisPlatform.exe
```

运行时还需要确保 OpenCV、FFmpeg 和 ONNX Runtime DLL 能被找到，例如将对应 `bin` 目录加入 `PATH`，或将所需 DLL 放到可执行文件同目录。

CMake 在 Windows 下会在构建 `VideoAnalysisPlatform` 后自动调用 `windeployqt`，把 Qt DLL、平台插件、QML/Quick 依赖部署到当前配置的输出目录。因此 Debug 和 Release 构建后都应能直接从 VS2022 或 `build\Debug` / `build\Release` 目录运行。

也可以使用 VS2022 直接打开项目根目录，选择对应的 Qt 6 CMake Kit 后进行配置、构建和运行。

## 当前目录说明

- `src/`：C++ 源码目录，当前包含程序入口、视频源数据结构、告警事件数据结构、ROI 规则结构、告警管理器、SQLite 数据库管理、视频源仓库、告警事件仓库、ROI 规则仓库、视频源列表模型、告警事件列表模型、性能监控模型、视频解码会话管理器、OpenCV/FFmpeg 解码器、线程安全帧队列、截图管理器、录像管理器、基础运动检测器、入侵检测器、ONNX 目标检测器、QML 图像帧提供器和自定义视频渲染项。
- `qml/`：QML 界面目录，当前包含主窗口、页面、基础组件、主题、视频源列表展示、告警中心列表、1/4 宫格视频墙、ROI 绘制组件、性能监控面板、手动截图/录像按钮、AI 检测开关和 ONNX 模型选择入口，以及默认使用 `VideoRenderItem` 的视频显示和旧 `Image` fallback。
- `docs/`：项目文档目录。
- `assets/`：图标和示例资源目录。
- `tests/`：Qt Test 单元测试目录。
- `third_party/`：后续第三方库或说明目录。

## 数据库

程序启动时会自动创建 `data/app.db`，当前阶段初始化 `video_sources`、`alarm_events` 和 `roi_rules` 三张表：

```sql
CREATE TABLE IF NOT EXISTS video_sources (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    type TEXT NOT NULL,
    url TEXT NOT NULL,
    enabled INTEGER NOT NULL DEFAULT 1,
    created_at TEXT NOT NULL,
    updated_at TEXT NOT NULL
);
```

视频源新增、删除和修改名称会立即保存到 SQLite；程序重启后会自动加载历史视频源。

```sql
CREATE TABLE IF NOT EXISTS alarm_events (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    source_id INTEGER NOT NULL,
    source_name TEXT NOT NULL,
    event_type TEXT NOT NULL,
    level TEXT NOT NULL,
    message TEXT,
    snapshot_path TEXT,
    video_clip_path TEXT,
    confirmed INTEGER NOT NULL DEFAULT 0,
    timestamp TEXT NOT NULL
);
```

告警中心通过 `AlarmEventModel` 加载历史告警，并支持手动添加测试告警、确认告警和删除告警。运动检测产生的 `MotionDetected` 告警会通过 `AlarmManager` 写入该表，自动截图成功时 `snapshot_path` 会保存对应文件路径。

```sql
CREATE TABLE IF NOT EXISTS roi_rules (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    source_id INTEGER NOT NULL,
    rule_name TEXT NOT NULL,
    enabled INTEGER NOT NULL DEFAULT 1,
    roi_type TEXT NOT NULL DEFAULT 'rect',
    points_json TEXT NOT NULL,
    sensitivity REAL NOT NULL DEFAULT 0.5,
    min_target_area INTEGER NOT NULL DEFAULT 0,
    cooldown_seconds INTEGER NOT NULL DEFAULT 10,
    created_at TEXT NOT NULL,
    updated_at TEXT NOT NULL
);
```

`RoiRuleRepository` 负责 `roi_rules` 的增删改查。当前只使用矩形 ROI，坐标以归一化 `x/y/width/height` 写入 `points_json`；`cooldown_seconds` 用于同一 ROI 规则的入侵告警冷却。

## 四路视频墙和 RTSP

`VideoWallPage.qml` 当前支持 1 宫格和 4 宫格切换。`StreamSessionManager` 统一持有 4 个 `StreamSession`，每个 `VideoTile` 绑定一个独立会话，可分别选择本地 mp4 或输入 RTSP 地址，并独立播放、暂停、停止。解码逻辑在后台线程中运行，本地文件默认使用 `OpenCVVideoDecoder`，RTSP 使用 `FFmpegVideoDecoder`。

`FFmpegVideoDecoder` 实现 `IVideoDecoder`，使用 FFmpeg 打开本地视频或 RTSP 视频流，读取视频流信息，解码视频包并通过 `sws_scale` 转为 RGBA `QImage`，再由 `StreamSession` 转成内部 `Frame` 写入队列。RTSP 读取异常会让对应 `StreamSession` 进入 `Reconnecting`，按约 3 秒间隔在后台重连；每个 `VideoTile` 可配置最大重连次数，超过次数后进入 `Error` 并保存 `lastError`。用户点击停止会中断重连；本地 mp4 播放仍按原有停止/错误逻辑处理。当前阶段不做硬件解码。

`StreamSession` 内部使用 `FrameQueue` 将解码产帧和画面分发解耦。`FrameQueue` 有固定最大容量，队列满时丢弃旧帧并累计 `droppedFrames`，避免无限占用内存；停止视频流时会关闭并清空队列。画面帧仍由 `VideoFrameProvider` 按 stream id 缓存，`VideoTile.qml` 默认使用 `VideoRenderItem` 从 `StreamSession` 取当前帧并绘制视频、检测框、ROI 和基础状态叠加；旧的 `image://videoFrame` + `Image` 显示路径保留为 fallback。

## 性能监控面板

`PerformanceModel` 每秒从 `StreamSessionManager` 持有的 4 个 `StreamSession` 读取一次 `StreamPerformance` 快照，并向 QML 暴露 `sourceName`、`fps`、`decodeTimeMs`、`queueSize`、`droppedFrames`、`status`。`PerformancePanel.qml` 当前显示在视频墙页面右侧，刷新频率控制在约 1 次/秒，避免把性能面板做成每帧刷新。

## 手动截图

`VideoTile.qml` 提供手动截图按钮。截图时 `StreamSession` 从 `VideoFrameProvider` 获取当前帧，并通过 `SnapshotManager` 在后台线程保存为图片，避免明显影响视频播放。默认保存目录为 `data/snapshots/YYYY-MM-DD/`，文件名格式包含 `sourceId`、时间和事件类型，例如 `stream1_153012_123_manual.jpg`。手动截图不写入数据库；运动检测告警触发的自动截图由 `AlarmManager` 保存，并把路径写入 `alarm_events.snapshot_path`。

## 手动录像

`VideoTile.qml` 提供手动录像按钮和 `REC` 状态显示。开始录像时 `StreamSession` 使用当前视频帧尺寸和当前源 FPS 创建录像文件，随后把后台解码线程得到的帧写入 `RecordingManager`；停止录像、切换源、停止播放或视频结束时会关闭文件。默认保存目录为 `data/records/YYYY-MM-DD/`，文件名包含 `sourceId`、时间和 `manual`，当前保存为 MJPG 编码的 `.avi` 文件。录像依赖 OpenCV `VideoWriter`，未启用 OpenCV 时会在 UI 返回错误。当前只做手动录像，不做告警前后录像，也不把录像路径写入数据库。

## 基础运动检测

`MotionDetector` 实现 `IVideoAnalyzer` 接口，当前使用 OpenCV 帧差法：转灰度、`absdiff`、阈值二值化、形态学处理、轮廓检测和面积过滤。`StreamSession` 在后台解码线程中运行检测逻辑，并将 `DetectionResult` 中的检测框转换为 QML 可用的归一化坐标。`VideoTile.qml` 提供“运动检测”开关和 `minTargetArea` 参数，启用后在视频画面上叠加矩形框；关闭后会清空检测框。检测到运动时，`StreamSession` 只通知 `AlarmManager`，由 `AlarmManager` 统一生成告警、截图和写库。

## ONNX Runtime AI 检测

`OnnxObjectDetector` 实现目标检测接口，支持加载 ONNX 模型路径、图像预处理、异步推理和基础 YOLO 类输出后处理。`VideoTile.qml` 提供“AI检测”开关、ONNX 模型选择、`confidenceThreshold` 和 `inferenceInterval` 控件；`StreamSession` 在独立 AI 工作线程中按间隔调度推理，避免阻塞 QML UI 和视频解码。检测结果通过 `objectDetectionBoxes` 暴露给 QML，默认由 `VideoRenderItem` 绘制类别、置信度和矩形框，旧 `Image` fallback 也保留基础叠加。

AI 检测到 person 或车辆类目标时，`StreamSession` 会通知 `AlarmManager` 生成 `PersonDetected` 或 `VehicleDetected` 告警，并复用现有自动截图和 `alarm_events` 入库流程。未配置 ONNX Runtime 时，项目仍会正常构建，加载模型会返回“ONNX Runtime 未配置”的错误状态，不影响已有运动检测、ROI 入侵和播放功能。

## 告警事件

`AlarmEventRepository` 封装 `alarm_events` 表的新增、加载、确认和删除操作。`AlarmEventModel` 继承 `QAbstractListModel`，向 QML 暴露 `alarmId`、`sourceId`、`sourceName`、`eventType`、`level`、`message`、`snapshotPath`、`videoClipPath`、`confirmed` 和 `timestamp`。`AlarmCenterPage.qml` 当前显示告警列表，并提供“添加测试告警”“确认”“删除”和“刷新”操作。

`AlarmManager` 当前负责生成 `MotionDetected`、`IntrusionDetected`、`PersonDetected` 和 `VehicleDetected` 告警：运动检测按视频源默认 10 秒冷却，ROI 入侵按 `source + ROI rule` 独立使用每条规则的 `cooldownSeconds`，AI 告警按 `source + eventType` 冷却，避免每帧刷屏；告警发生时会后台保存 jpg 截图，随后把告警事件写入 SQLite，并通过 `AlarmEventModel` 实时显示在告警中心。

## ROI 区域绘制

`RoiRule` 包含 `id`、`sourceId`、`ruleName`、归一化 `rect`、`enabled` 和 `cooldownSeconds`。`StreamSession` 持有当前通道对应的 ROI 列表，并向 QML 暴露 `roiRules`、`addRoiRect`、`removeRoiRule` 和 `clearRoiRules`；这些接口内部通过 `RoiRuleRepository` 持久化到 SQLite，QML 不直接操作数据库。

`RoiEditor.qml` 叠加在 `VideoTile.qml` 的实际视频画面区域上，拖拽时会按 `VideoRenderItem` 或 fallback `Image` 的 `paintedWidth` / `paintedHeight` 反算 0 到 1 的归一化坐标，因此在等比缩放和留黑边时 ROI 也能贴合视频画面。绘制完成会调用 `StreamSession.addRoiRect` 并保存到 `roi_rules`；程序启动或重新打开本通道视频时会按当前 stream/sourceId 加载历史 ROI。

`IntrusionDetector` 只接收 `MotionDetector` 输出的检测框和当前 ROI 快照，第一版以检测框中心点是否落入矩形 ROI 作为入侵判断。命中 ROI 后，`StreamSession` 通知 `AlarmManager` 生成 `IntrusionDetected` 告警并自动截图；`MotionDetector` 不直接操作数据库或告警。

## 测试

当前包含 `FrameQueueTests`、`SnapshotManagerTests`、`RecordingManagerTests`、`AlarmEventRepositoryTests`、`RoiRuleRepositoryTests`、`AlarmManagerTests`、`MotionDetectorTests`、`OnnxObjectDetectorTests`、`IntrusionDetectorTests` 和 `FFmpegVideoDecoderTests`。分别用于验证帧队列、截图保存、手动录像文件生成、告警事件仓库、ROI 规则仓库、运动/入侵/AI 告警生成、基础运动检测行为、ONNX 检测器降级路径、ROI 入侵判断和 FFmpeg 解码器基础失败路径。

```powershell
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
cmake --build build --config Release --target all_qmllint
```

## 下一步建议

下一步建议先在已配置 ONNX Runtime DLL 的机器上用真实 ONNX 模型和 1 路/4 路视频验证类别映射、置信度阈值、推理间隔、AI 框叠加和 `PersonDetected` / `VehicleDetected` 告警冷却，再按 `PROJECT_REQUIREMENTS.md` 推进后续阶段；告警前后录像、硬件解码和 9/16 路视频墙仍不要提前实现。
