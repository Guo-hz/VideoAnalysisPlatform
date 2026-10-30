# 实时多路视频流智能分析平台

这是一个基于 C++17、Qt 6、QML 和 CMake 的桌面端项目。当前在“阶段 18：ONNX Runtime AI 检测”的基础上，继续实现了通道绑定、录像索引与回放、设置页、启动失败页和凭据托管。包含主程序入口、QML 主窗口、左侧六页导航（首页/视频墙/视频源/告警中心/回放/设置）、视频源列表模型、SQLite 持久化（含 schema 版本迁移）、最多四路本地 mp4 播放、FFmpeg/RTSP/HTTP 解码入口、RTSP 自动重连、用于解码和画面分发解耦的线程安全帧队列、按秒刷新的视频流性能监控面板、手动截图与手动录像、基于 OpenCV **MOG2 背景建模**的基础运动检测框叠加、告警事件表/仓库/列表模型/告警中心、四类自动告警、视频画面上的矩形 ROI 绘制与数据库存储、基于 ROI 中心点命中的区域入侵告警、自定义 VideoRenderItem、可选 ONNX Runtime 目标检测与 AI 框叠加，以及视频源↔视频墙通道绑定（`channel_bindings`）、录像文件磁盘索引与本地回放页、应用数据目录自动迁移（`AppPaths`）和 RTSP/HTTP 凭据托管（Windows Credential Manager）。

当前不包含 9/16 路视频墙、硬件解码或告警前后录像。注意“录像入库”目前只是占位实现：`recordings` 表由 `RecordingModel::reindex()` 扫描磁盘 `.avi` 建立索引行（`source_id` 固定 0、时长/分辨率/帧率为 0），录像过程本身不写数据库。

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

- `src/`：C++ 源码目录，包含程序入口、应用层（`AppPaths` 数据目录与旧数据迁移、`SettingsController` 设置、`StartupController` 启动失败页）、视频源与告警事件数据结构、ROI 规则结构、告警管理器、凭据存储（`CredentialStore`）、SQLite 数据库管理与 schema 迁移、视频源/告警事件/ROI 规则/通道绑定仓库、视频源/告警事件/性能/录像索引列表模型、视频解码会话管理器、OpenCV/FFmpeg 解码器、线程安全帧队列、截图管理器、录像管理器、基础运动检测器（MOG2）、入侵检测器、ONNX 目标检测器、QML 图像帧提供器和自定义视频渲染项。
- `qml/`：QML 界面目录，包含主窗口、6 个页面（首页总览、1/4 宫格视频墙、视频源管理与通道绑定、告警中心、本地录像回放、设置）、启动失败页 `StartupError.qml`、基础组件与统一主题、ROI 绘制组件、性能监控面板、手动截图/录像按钮、AI 检测开关和 ONNX 模型选择入口，以及默认使用 `VideoRenderItem` 的视频显示和旧 `Image` fallback。
- `docs/`：项目文档目录。
- `assets/`：图标和示例资源目录。
- `tests/`：Qt Test 单元测试目录。
- `third_party/`：后续第三方库或说明目录。

## 数据库

程序启动时会在应用数据目录下创建并初始化 `app.db`（具体路径见下文“应用数据目录与设置”），当前共 7 张业务表：`video_sources`、`alarm_events`、`roi_rules`、`recordings`、`channel_bindings`、`app_settings`、`source_analysis_settings`，另有一张旧数据备份表 `legacy_roi_rules`。schema 使用 `PRAGMA user_version` 管理版本，升级前会自动把数据库备份为 `<db>.bak-v<N>-<时间戳>`。

```sql
CREATE TABLE IF NOT EXISTS video_sources (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    type TEXT NOT NULL,
    url TEXT NOT NULL,
    credential_ref TEXT NOT NULL DEFAULT '',
    enabled INTEGER NOT NULL DEFAULT 1,
    reconnect_max_attempts INTEGER NOT NULL DEFAULT 3,
    reconnect_interval_ms INTEGER NOT NULL DEFAULT 3000,
    connect_timeout_ms INTEGER NOT NULL DEFAULT 5000,
    read_timeout_ms INTEGER NOT NULL DEFAULT 10000,
    created_at TEXT NOT NULL,
    updated_at TEXT NOT NULL
);
```

视频源新增、删除和修改名称会立即保存到 SQLite；程序重启后会自动加载历史视频源。网络源的 `url` 是脱敏地址，真实用户名和密码由 `credential_ref` 指向 Windows Credential Manager（见下文“凭据管理”）。`connect_timeout_ms` 和 `read_timeout_ms` 当前尚未真正下发到解码器，保留为后续接入。

```sql
CREATE TABLE IF NOT EXISTS alarm_events (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    source_id INTEGER NOT NULL,
    video_source_id INTEGER NULL REFERENCES video_sources(id) ON DELETE SET NULL,
    legacy_source_key TEXT NOT NULL DEFAULT '',
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
    source_id INTEGER NOT NULL REFERENCES video_sources(id) ON DELETE CASCADE,
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

`RoiRuleRepository` 负责 `roi_rules` 的增删改查。当前只使用矩形 ROI，坐标以归一化 `x/y/width/height` 写入 `points_json`；`cooldown_seconds` 用于同一 ROI 规则的入侵告警冷却。注意 `source_id` 已改为指向 `video_sources(id)` 的外键（删除视频源会级联删除其 ROI 规则）。

其余表的作用：

- `channel_bindings`：视频墙通道与视频源的绑定关系，`channel_index` 为 0-3 主键（对应 `stream1`-`stream4`），`source_id` 唯一并级联删除，因此一个源只能占用一个通道。
- `recordings`：录像索引，由 `RecordingModel::reindex()` 扫描录像目录下的 `.avi` 写入占位记录。
- `app_settings`、`source_analysis_settings`：已建表但目前没有任何读写方，设置项实际存在 QSettings 中。
- `legacy_roi_rules`：旧版本按“通道号 1-4”存储的 ROI 冷备份。旧库升级时会把 `roi_rules` 的旧数据整表搬到这里（原 `source_id` 存进 `legacy_channel`）然后重建空的 `roi_rules`；目前没有代码把数据回填回应用，旧 ROI 需要人工从这张表恢复或重新绘制。

## 应用数据目录与设置

程序运行时不再使用源码目录下的 `data/`，而是把数据放在 `QStandardPaths::AppLocalDataLocation`（Windows 上约为 `%LOCALAPPDATA%\VideoAnalysisPlatform\VideoAnalysisPlatform\`），其中：

- `app.db`：SQLite 数据库
- `snapshots/YYYY-MM-DD/`：手动截图与告警自动截图
- `records/YYYY-MM-DD/`：手动录像
- `models/`：ONNX 模型
- `logs/`：日志目录

`AppPaths::prepare()` 在启动时创建这些目录；如果目标 `app.db` 还不存在，会自动尝试从旧的 `data/` 布局（例如工作目录下的 `data/`、`build/<Config>/data`、可执行文件旁的 `data/`）迁移数据库，迁移会用 SQLite 只读探针校验候选库，并把旧目录改名保留为 `*.preexisting-<时间戳>`。路径目前是只读属性，暂不可配置。

任一步骤失败（目录不可写或数据库打不开）时，程序不会进入主界面，而是加载 `StartupError.qml` 启动失败页，显示错误信息、数据目录和“打开数据目录 / 退出 / 重试”三个按钮。

设置页 `SettingsPage.qml` 可以查看上述各目录并用系统文件管理器打开，可以修改“磁盘告警阈值”（取值 1-50，经 `SettingsController` 存入 QSettings），并查看 YOLOX 模型文件是否存在。注意该磁盘阈值目前还没有任何运行时消费方。

## 四路视频墙和 RTSP

`VideoWallPage.qml` 当前支持 1 宫格和 4 宫格切换。`StreamSessionManager` 统一持有 4 个 `StreamSession`，每个 `VideoTile` 绑定一个独立会话，可分别选择本地 mp4 或输入 RTSP 地址，并独立播放、暂停、停止。解码逻辑在后台线程中运行，本地文件默认使用 `OpenCVVideoDecoder`，RTSP/HTTP 使用 `FFmpegVideoDecoder`。

**通道绑定**：视频源与视频墙通道的绑定由 `StreamSessionManager` 负责，`channel_bindings` 表中 `channel_index` 0-3 对应 `stream1`-`stream4`，一个视频源只能绑定一个通道（`bind` 是替换语义）。可以在视频源页选择通道后点 Bind，或在视频墙页选择视频源后点 “Bind and play”。绑定后该通道的 `StreamSession` 会打开这个源，ROI 也按该视频源的 id 存取；程序启动时会自动恢复已有绑定。`VideoTile.qml` 仍保留不经绑定的直连入口（选择本地文件 / 打开 RTSP / 打开 HTTP），这些路径下 ROI 仍按 1-4 通道号存取。当前没有解绑按钮。

`FFmpegVideoDecoder` 实现 `IVideoDecoder`，使用 FFmpeg 打开本地视频或 RTSP 视频流，读取视频流信息，解码视频包并通过 `sws_scale` 转为 RGBA `QImage`，再由 `StreamSession` 转成内部 `Frame` 写入队列。RTSP 读取异常会让对应 `StreamSession` 进入 `Reconnecting`，按 `reconnect_interval_ms`（默认 3000 ms）在后台重连；最大重试次数取自 `reconnect_max_attempts`（默认 3 次），超过后进入 `Error` 并保存 `lastError`。用户点击停止会中断重连；本地 mp4 播放仍按原有停止/错误逻辑处理。当前阶段不做硬件解码。

`StreamSession` 内部使用 `FrameQueue` 将解码产帧和画面分发解耦。`FrameQueue` 有固定最大容量，队列满时丢弃旧帧并累计 `droppedFrames`，避免无限占用内存；停止视频流时会关闭并清空队列。画面帧仍由 `VideoFrameProvider` 按 stream id 缓存，`VideoTile.qml` 默认使用 `VideoRenderItem` 从 `StreamSession` 取当前帧并绘制视频、检测框、ROI 和基础状态叠加；旧的 `image://videoFrame` + `Image` 显示路径保留为 fallback。

## 性能监控面板

`PerformanceModel` 每秒从 `StreamSessionManager` 持有的 4 个 `StreamSession` 读取一次 `StreamPerformance` 快照，并向 QML 暴露 `sourceName`、`fps`、`decodeTimeMs`、`queueSize`、`droppedFrames`、`status`。`PerformancePanel.qml` 当前显示在视频墙页面右侧，刷新频率控制在约 1 次/秒，避免把性能面板做成每帧刷新。

## 手动截图

`VideoTile.qml` 提供手动截图按钮。截图时 `StreamSession` 从 `VideoFrameProvider` 获取当前帧，并通过 `SnapshotManager` 在后台线程保存为图片，避免明显影响视频播放。默认保存目录为应用数据目录下的 `snapshots/YYYY-MM-DD/`（主程序由 `AppPaths` 注入），文件名格式包含 `sourceId`、时间和事件类型，例如 `stream1_153012_123_manual.jpg`。手动截图不写入数据库；运动检测告警触发的自动截图由 `AlarmManager` 保存，并把路径写入 `alarm_events.snapshot_path`。

## 手动录像

`VideoTile.qml` 提供手动录像按钮和 `REC` 状态显示。开始录像时 `StreamSession` 使用当前视频帧尺寸和当前源 FPS 创建录像文件，随后把后台解码线程得到的帧写入 `RecordingManager`；停止录像、切换源、停止播放或视频结束时会关闭文件。默认保存目录为应用数据目录下的 `records/YYYY-MM-DD/`（主程序由 `AppPaths` 注入），文件名包含 `sourceId`、时间和 `manual`，保存为 MJPG 编码的 `.avi` 文件。录像依赖 OpenCV `VideoWriter`，未启用 OpenCV 时会在 UI 返回错误。当前只做手动录像，不做告警前后录像。录像过程不写数据库，`recordings` 表中的记录来自下面介绍的磁盘扫描索引。

## 录像索引与回放

`RecordingModel::reindex()` 会扫描录像目录下的 `.avi` 文件并写入 `recordings` 表。当前写入的是占位索引：`source_id` 固定为 0、`source_name` 固定为 `Indexed recording`、`status` 恒为 `Completed`，时长/分辨率/帧率保持默认 0。索引在程序启动时执行一次，也可以在回放页点 Reindex 手动触发。`loadFromDatabase()` 读取索引时会跳过磁盘上已不存在的文件，但目前没有删除数据库行的逻辑，孤儿记录会累积。

`PlaybackPage.qml` 使用 QtMultimedia 的 `MediaPlayer` + `VideoOutput` 播放本地录像，支持列表选择、播放/暂停/停止、跳过 ±10 秒、拖动进度，以及 Reindex / Reload 刷新索引。当前不支持删除录像。

## 基础运动检测

`MotionDetector` 实现 `IVideoAnalyzer` 接口，使用 OpenCV **MOG2 背景建模**：转灰度、MOG2 前景提取、阈值 200 去阴影、形态学开运算与膨胀、轮廓检测和面积过滤；启动预热 30 帧，并要求连续 3 帧检出才输出检测框。`minTargetArea` 是当前唯一可调参数，修改后会重置背景模型。`StreamSession` 在后台解码线程中运行检测逻辑，并将 `DetectionResult` 中的检测框转换为 QML 可用的归一化坐标。`VideoTile.qml` 提供“运动检测”开关和 `minTargetArea` 参数，启用后在视频画面上叠加矩形框；关闭后会清空检测框。检测到运动时，`StreamSession` 只通知 `AlarmManager`，由 `AlarmManager` 统一生成告警、截图和写库。

## ONNX Runtime AI 检测

`OnnxObjectDetector` 实现目标检测接口，支持加载 ONNX 模型路径、图像预处理、异步推理和基础 YOLO 类输出后处理。`VideoTile.qml` 提供“AI检测”开关、ONNX 模型选择、`confidenceThreshold` 和 `inferenceInterval` 控件；`StreamSession` 在独立 AI 工作线程中按间隔调度推理，避免阻塞 QML UI 和视频解码。检测结果通过 `objectDetectionBoxes` 暴露给 QML，默认由 `VideoRenderItem` 绘制类别、置信度和矩形框，旧 `Image` fallback 也保留基础叠加。

AI 检测到 person 或车辆类目标时，`StreamSession` 会通知 `AlarmManager` 生成 `PersonDetected` 或 `VehicleDetected` 告警，并复用现有自动截图和 `alarm_events` 入库流程。类别映射目前硬编码为 person 与车辆类（id 2/3/5/7 或类名含 car/bus/truck/motorcycle），数据库里的 `ai_alert_categories` 默认 6 类尚未被读取，因此 `bicycle` 不会触发告警；NMS 阈值固定为 0.45，未使用 `ai_nms_threshold`。未配置 ONNX Runtime 时，项目仍会正常构建，加载模型会返回“ONNX Runtime 未配置”的错误状态，不影响已有运动检测、ROI 入侵和播放功能。

## 告警事件

`AlarmEventRepository` 封装 `alarm_events` 表的新增、加载、确认和删除操作。`AlarmEventModel` 继承 `QAbstractListModel`，向 QML 暴露 `alarmId`、`sourceId`、`sourceName`、`eventType`、`level`、`message`、`snapshotPath`、`videoClipPath`、`confirmed` 和 `timestamp`。`AlarmCenterPage.qml` 当前显示告警列表，并提供“添加测试告警”“确认”“删除”和“刷新”操作。

`AlarmManager` 当前负责生成 `MotionDetected`、`IntrusionDetected`、`PersonDetected` 和 `VehicleDetected` 告警：运动检测按视频源默认 10 秒冷却，ROI 入侵按 `source + ROI rule` 独立使用每条规则的 `cooldownSeconds`，AI 告警按 `source + eventType` 冷却，避免每帧刷屏；告警发生时会后台保存 jpg 截图，随后把告警事件写入 SQLite，并通过 `AlarmEventModel` 实时显示在告警中心。

## ROI 区域绘制

`RoiRule` 包含 `id`、`sourceId`、`ruleName`、归一化 `rect`、`enabled` 和 `cooldownSeconds`。`StreamSession` 持有当前通道/视频源对应的 ROI 列表，并向 QML 暴露 `roiRules`、`addRoiRect`、`removeRoiRule` 和 `clearRoiRules`；这些接口内部通过 `RoiRuleRepository` 持久化到 SQLite，QML 不直接操作数据库。

`RoiEditor.qml` 叠加在 `VideoTile.qml` 的实际视频画面区域上，拖拽时会按 `VideoRenderItem` 或 fallback `Image` 的 `paintedWidth` / `paintedHeight` 反算 0 到 1 的归一化坐标，因此在等比缩放和留黑边时 ROI 也能贴合视频画面。绘制完成会调用 `StreamSession.addRoiRect` 并保存到 `roi_rules`；打开视频源或重新绑定时会按当前 id 加载历史 ROI。

ROI 的归因方式取决于视频的打开方式：经通道绑定打开的源按 `video_sources.id` 存取 ROI；`VideoTile.qml` 直连路径（选择本地文件 / 打开 RTSP / 打开 HTTP）仍按 1-4 通道号存取，此时若对应 `video_sources.id` 不存在，新增 ROI 会因 `roi_rules.source_id` 的外键约束而失败。

`IntrusionDetector` 只接收 `MotionDetector` 输出的检测框和当前 ROI 快照，第一版以检测框中心点是否落入矩形 ROI 作为入侵判断。命中 ROI 后，`StreamSession` 通知 `AlarmManager` 生成 `IntrusionDetected` 告警并自动截图；`MotionDetector` 不直接操作数据库或告警。

## 凭据管理

RTSP/HTTP 视频源的用户名和密码不会明文写进数据库：`CredentialStore` 把它们存入 Windows Credential Manager（条目名形如 `VideoAnalysisPlatform/source/<uuid>`，`video_sources.credential_ref` 保存这个引用），数据库里的 `url` 是脱敏地址（username/password 被清空，query 被替换为 `[redacted]`）。播放或刷新该源时，`StreamSessionManager` 会先用 `credential_ref` 取回完整地址再交给解码器，界面展示用的仍是脱敏地址。

`CredentialStore` 依赖 Windows Credential Manager，非 Windows 平台不支持，读写都会失败；而保存 RTSP/HTTP 源时是无条件写入凭据的，因此在非 Windows 平台上连不带凭据的网络源也无法保存。

## 测试

当前包含 `FrameQueueTests`、`SnapshotManagerTests`、`RecordingManagerTests`、`AlarmEventRepositoryTests`、`RoiRuleRepositoryTests`、`AlarmManagerTests`、`MotionDetectorTests`、`OnnxObjectDetectorTests`、`IntrusionDetectorTests` 和 `FFmpegVideoDecoderTests`。分别用于验证帧队列、截图保存、手动录像文件生成、告警事件仓库、ROI 规则仓库、运动/入侵/AI 告警生成、基础运动检测行为、ONNX 检测器降级路径、ROI 入侵判断和 FFmpeg 解码器基础失败路径。

```powershell
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
cmake --build build --config Release --target all_qmllint
```

## 下一步建议

下一步建议：

- 先在已配置 ONNX Runtime DLL 的机器上用真实 ONNX 模型和 1 路/4 路视频验证类别映射、置信度阈值、推理间隔、AI 框叠加和 `PersonDetected` / `VehicleDetected` 告警冷却。
- 在具备真实 RTSP 流和 FFmpeg DLL 的环境下验证断线进入 `Reconnecting` 和恢复播放。
- 处理已知遗留：旧库 ROI 迁移后不回填活跃表（数据留在 `legacy_roi_rules`）、`VideoTile.qml` 直连路径与绑定路径的 ROI 归因不一致、缺少解绑 UI、`connect_timeout_ms` / `read_timeout_ms` 未下发到解码器、`app_settings` 与 `source_analysis_settings` 空表未接线、`recordings` 孤儿索引无法清理。
- 之后按 `PROJECT_REQUIREMENTS.md` 推进后续阶段；告警前后录像、硬件解码和 9/16 路视频墙仍不要提前实现。
