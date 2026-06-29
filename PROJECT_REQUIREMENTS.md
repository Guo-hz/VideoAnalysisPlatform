# 实时多路视频流智能分析平台需求文档

## 1. 项目名称

基于 C++/Qt/QML 的实时多路视频流智能分析平台

英文名可选：

Real-time Multi-channel Video Stream Intelligent Analysis Platform

---

## 2. 项目定位

本项目是一个面向安防监控、工业视觉、视频分析场景的桌面端软件系统。

项目目标不是做一个普通播放器，而是实现一个具备以下能力的高性能 Qt/QML 客户端：

1. 支持本地视频、摄像头、RTSP 视频流接入。
2. 支持多路视频墙实时展示。
3. 支持视频源管理、播放控制、截图、录像。
4. 支持运动检测、区域入侵检测、事件告警。
5. 支持告警事件存储、查询、截图查看、历史回放。
6. 支持 FPS、解码耗时、分析耗时、队列长度、丢帧数量等性能监控。
7. 使用 C++ 负责底层视频处理、任务调度、数据存储和算法逻辑。
8. 使用 QML 负责界面展示、交互操作和状态可视化。

---

## 3. 项目核心卖点

这个项目需要体现以下技术能力：

1. C++ 工程架构能力。
2. Qt/QML 桌面端开发能力。
3. C++ 与 QML 混合开发能力。
4. 多线程和异步任务调度能力。
5. 实时视频流处理能力。
6. 生产者-消费者模型。
7. 帧队列、限流、主动丢帧策略。
8. OpenCV 图像分析能力。
9. FFmpeg 视频解码能力。
10. SQLite 本地数据存储能力。
11. 自定义 QML 组件和高性能界面渲染能力。
12. 工程化能力：日志、配置、错误处理、模块解耦、测试。

---

## 4. 推荐技术栈

### 4.1 基础技术

* C++17 或 C++20
* Qt 6
* QML / Qt Quick
* CMake
* SQLite
* spdlog 或 Qt 日志系统
* Git

### 4.2 视频处理

第一阶段可以使用：

* Qt Multimedia
* OpenCV VideoCapture

进阶阶段使用：

* FFmpeg
* libavformat
* libavcodec
* libswscale

### 4.3 图像分析

基础阶段：

* OpenCV
* 背景建模
* 帧差法
* 轮廓检测
* ROI 区域判断

高级阶段：

* ONNX Runtime
* YOLO 系列目标检测模型
* 异步 AI 推理
* 检测框绘制
* 置信度过滤
* NMS 后处理

### 4.4 数据存储

* SQLite
* 视频源配置表
* 告警事件表
* 截图文件索引表
* 系统配置表

---

## 5. 总体架构

### 5.1 架构分层

项目采用分层架构：

```text
QML UI 层
  ├── 视频墙界面
  ├── 视频源管理界面
  ├── 告警中心界面
  ├── 性能监控界面
  └── 系统设置界面

C++ ViewModel 层
  ├── VideoSourceModel
  ├── AlarmEventModel
  ├── PerformanceModel
  └── AppController

C++ Core 层
  ├── VideoSourceManager
  ├── StreamSession
  ├── FrameQueue
  ├── VideoDecoder
  ├── VideoAnalyzer
  ├── AlarmManager
  ├── SnapshotManager
  ├── RecordingManager
  └── StorageService

Infrastructure 层
  ├── Logger
  ├── ConfigManager
  ├── DatabaseManager
  ├── ThreadPool
  └── ErrorCode
```

---

## 6. 推荐目录结构

```text
VideoAnalysisPlatform/
├── CMakeLists.txt
├── README.md
├── PROJECT_REQUIREMENTS.md
├── docs/
│   ├── architecture.md
│   ├── roadmap.md
│   ├── database.md
│   └── codex_tasks.md
├── src/
│   ├── main.cpp
│   ├── app/
│   │   ├── AppController.h
│   │   ├── AppController.cpp
│   │   ├── AppContext.h
│   │   └── AppContext.cpp
│   ├── core/
│   │   ├── video/
│   │   │   ├── VideoSource.h
│   │   │   ├── VideoSource.cpp
│   │   │   ├── VideoSourceManager.h
│   │   │   ├── VideoSourceManager.cpp
│   │   │   ├── StreamSession.h
│   │   │   ├── StreamSession.cpp
│   │   │   ├── Frame.h
│   │   │   ├── FrameQueue.h
│   │   │   ├── FrameQueue.cpp
│   │   │   ├── IVideoDecoder.h
│   │   │   ├── OpenCVVideoDecoder.h
│   │   │   ├── OpenCVVideoDecoder.cpp
│   │   │   ├── FFmpegVideoDecoder.h
│   │   │   └── FFmpegVideoDecoder.cpp
│   │   ├── analysis/
│   │   │   ├── IVideoAnalyzer.h
│   │   │   ├── MotionDetector.h
│   │   │   ├── MotionDetector.cpp
│   │   │   ├── RoiRule.h
│   │   │   ├── RoiRule.cpp
│   │   │   ├── IntrusionDetector.h
│   │   │   └── IntrusionDetector.cpp
│   │   ├── alarm/
│   │   │   ├── AlarmEvent.h
│   │   │   ├── AlarmManager.h
│   │   │   └── AlarmManager.cpp
│   │   ├── storage/
│   │   │   ├── DatabaseManager.h
│   │   │   ├── DatabaseManager.cpp
│   │   │   ├── VideoSourceRepository.h
│   │   │   ├── VideoSourceRepository.cpp
│   │   │   ├── AlarmEventRepository.h
│   │   │   └── AlarmEventRepository.cpp
│   │   └── common/
│   │       ├── Logger.h
│   │       ├── Logger.cpp
│   │       ├── Result.h
│   │       ├── ErrorCode.h
│   │       └── TimeUtils.h
│   ├── models/
│   │   ├── VideoSourceModel.h
│   │   ├── VideoSourceModel.cpp
│   │   ├── AlarmEventModel.h
│   │   ├── AlarmEventModel.cpp
│   │   ├── PerformanceModel.h
│   │   └── PerformanceModel.cpp
│   └── render/
│       ├── VideoRenderItem.h
│       ├── VideoRenderItem.cpp
│       ├── OverlayItem.h
│       └── OverlayItem.cpp
├── qml/
│   ├── Main.qml
│   ├── pages/
│   │   ├── DashboardPage.qml
│   │   ├── VideoWallPage.qml
│   │   ├── SourceManagerPage.qml
│   │   ├── AlarmCenterPage.qml
│   │   ├── PlaybackPage.qml
│   │   └── SettingsPage.qml
│   ├── components/
│   │   ├── VideoTile.qml
│   │   ├── VideoGrid.qml
│   │   ├── SourceList.qml
│   │   ├── AlarmList.qml
│   │   ├── PerformancePanel.qml
│   │   ├── RoiEditor.qml
│   │   └── Toolbar.qml
│   └── styles/
│       ├── AppTheme.qml
│       └── Metrics.qml
├── assets/
│   ├── icons/
│   └── sample/
├── tests/
│   ├── test_frame_queue.cpp
│   ├── test_motion_detector.cpp
│   └── test_storage.cpp
└── third_party/
```

---

## 7. 功能需求

## 7.1 视频源管理模块

### 需求描述

用户可以添加、删除、编辑、启用、禁用视频源。

### 视频源类型

第一版支持：

1. 本地视频文件。
2. 本机摄像头。

第二版支持：

1. RTSP 视频流。
2. HTTP 视频流。

### 视频源字段

```text
id: 唯一 ID
name: 视频源名称
type: 视频源类型
url: 视频地址或文件路径
enabled: 是否启用
status: 当前状态
lastError: 最近一次错误
createdAt: 创建时间
updatedAt: 更新时间
```

### 视频源状态

```text
Stopped
Connecting
Playing
Paused
Reconnecting
Error
Disabled
```

### 验收标准

1. 可以在 QML 页面添加本地视频源。
2. 可以保存视频源配置到 SQLite。
3. 重启软件后可以恢复视频源列表。
4. 可以删除视频源。
5. 可以修改视频源名称和地址。
6. 视频源状态变化能实时显示在界面上。

---

## 7.2 多路视频墙模块

### 需求描述

用户可以在主界面同时查看多个视频源。

### 布局要求

支持以下布局：

```text
1 宫格
4 宫格
9 宫格
16 宫格
```

第一版只要求支持 1 宫格和 4 宫格。

### 单个视频窗口功能

每个视频窗口需要支持：

1. 显示视频画面。
2. 显示视频源名称。
3. 显示当前 FPS。
4. 显示当前状态。
5. 播放。
6. 暂停。
7. 停止。
8. 截图。
9. 全屏查看。
10. 告警框叠加显示。

### 验收标准

1. 至少可以同时播放 4 路本地视频。
2. 视频墙切换布局不会导致程序崩溃。
3. 单路视频异常不会影响其他视频。
4. 每路视频状态独立。
5. UI 主线程不能明显卡顿。

---

## 7.3 视频处理流水线模块

### 需求描述

每一路视频流都需要有独立的处理流水线。

### 流水线结构

```text
VideoSource
    ↓
VideoDecoder
    ↓
FrameQueue
    ↓
VideoAnalyzer
    ↓
RenderDispatcher
    ↓
QML VideoTile
```

### 核心类设计

#### StreamSession

负责管理单路视频流的生命周期。

职责：

1. 打开视频源。
2. 启动解码。
3. 停止解码。
4. 暂停和恢复。
5. 管理帧队列。
6. 管理分析器。
7. 发送状态变化。
8. 统计性能数据。

#### IVideoDecoder

视频解码接口。

```cpp
class IVideoDecoder {
public:
    virtual ~IVideoDecoder() = default;
    virtual bool open(const VideoSource& source) = 0;
    virtual bool readFrame(Frame& frame) = 0;
    virtual void close() = 0;
    virtual std::string lastError() const = 0;
};
```

#### FrameQueue

线程安全帧队列。

要求：

1. 支持 push。
2. 支持 pop。
3. 支持 tryPop。
4. 支持 clear。
5. 支持 maxSize。
6. 队列满时支持丢弃旧帧。
7. 统计丢帧数量。

### 验收标准

1. 解码和 UI 不在同一个线程。
2. FrameQueue 单元测试通过。
3. 队列满时不会无限占用内存。
4. 停止视频流后线程能正常退出。
5. 关闭程序时没有明显线程泄漏。

---

## 7.4 视频解码模块

### 第一阶段：OpenCV 解码

先使用 OpenCV VideoCapture 完成本地视频和摄像头读取。

优点：

1. 实现快。
2. 便于验证整体架构。
3. 降低初期复杂度。

### 第二阶段：FFmpeg 解码

使用 FFmpeg 实现更专业的视频解码模块。

FFmpegVideoDecoder 需要支持：

1. 本地视频文件。
2. RTSP URL。
3. 自动识别视频流。
4. 解码 AVPacket。
5. 输出 RGB 或 RGBA 帧。
6. 异常处理。
7. 超时处理。
8. 资源释放。

### 验收标准

第一阶段：

1. 能播放本地 mp4。
2. 能打开摄像头。
3. 能读取帧宽、高、FPS。

第二阶段：

1. 能播放 RTSP 流。
2. RTSP 断开后能进入 Error 或 Reconnecting 状态。
3. 解码失败时不崩溃。
4. 能释放 AVFormatContext、AVCodecContext、AVFrame、AVPacket 等资源。

---

## 7.5 渲染模块

### 第一阶段渲染方案

先采用简单方案：

1. C++ 将视频帧转换为 QImage。
2. 通过信号槽传给 QML。
3. QML 通过 Image 显示。

该方案性能一般，但适合快速完成 MVP。

### 第二阶段渲染方案

实现自定义 VideoRenderItem。

要求：

1. 继承 QQuickItem。
2. C++ 接收视频帧。
3. QML 中使用 VideoRenderItem。
4. 支持检测框、ROI 区域、文字信息叠加显示。
5. 减少 QML 层频繁创建大量对象。
6. 为后续 GPU 纹理渲染预留接口。

### 验收标准

第一阶段：

1. 视频能正常显示。
2. 播放过程中 UI 不明显卡顿。
3. 可以在视频上叠加简单矩形框。

第二阶段：

1. 自定义 QQuickItem 能显示视频帧。
2. 检测框绘制与视频画面位置一致。
3. 4 路视频同时播放时界面仍可操作。

---

## 7.6 运动检测模块

### 需求描述

对视频帧进行基础运动检测，生成运动区域。

### 第一版算法

使用帧差法：

```text
当前帧转灰度
上一帧转灰度
计算 absdiff
阈值二值化
形态学处理
查找轮廓
过滤小面积区域
输出 bounding boxes
```

### 第二版算法

使用 OpenCV BackgroundSubtractorMOG2 或 KNN。

### 输出结果

```text
sourceId
timestamp
boxes
score
eventType
```

### 验收标准

1. 视频中有明显运动时能显示检测框。
2. 静止画面不应持续产生大量误报。
3. 可以设置最小检测面积。
4. 检测结果可以显示在 QML 视频画面上。
5. 检测算法运行在后台线程，不阻塞 UI。

---

## 7.7 ROI 区域入侵检测模块

### 需求描述

用户可以在视频画面上配置感兴趣区域 ROI，当运动目标进入该区域时生成告警。

### ROI 类型

第一版支持矩形 ROI。

高级版支持多边形 ROI。

### 规则字段

```text
ruleId
sourceId
ruleName
enabled
roiType
points
sensitivity
minTargetArea
cooldownSeconds
```

### 判断逻辑

第一版：

1. 运动检测输出 bounding box。
2. 判断 bounding box 中心点是否落入 ROI。
3. 如果落入 ROI，则生成 Intrusion 告警。
4. 同一规则设置冷却时间，避免连续刷屏。

### 验收标准

1. 用户可以在 QML 中绘制矩形 ROI。
2. ROI 可以保存到本地配置。
3. 有目标进入 ROI 时产生告警。
4. 告警不会每帧重复生成。
5. 可以启用或禁用某条 ROI 规则。

---

## 7.8 告警中心模块

### 需求描述

系统检测到事件后，需要记录告警并在界面展示。

### 告警类型

```text
MotionDetected
IntrusionDetected
SourceOffline
DecodeError
RecordingError
```

### 告警字段

```text
id
sourceId
sourceName
eventType
level
message
snapshotPath
videoClipPath
timestamp
confirmed
```

### 告警等级

```text
Info
Warning
Critical
```

### 功能要求

1. 实时展示最新告警。
2. 点击告警显示截图。
3. 支持按视频源过滤。
4. 支持按事件类型过滤。
5. 支持按时间范围过滤。
6. 支持确认告警。
7. 支持删除告警记录。

### 验收标准

1. 运动检测事件可以生成告警。
2. 区域入侵事件可以生成告警。
3. 告警写入 SQLite。
4. 重启软件后仍能看到历史告警。
5. 点击告警可以打开对应截图。

---

## 7.9 截图模块

### 需求描述

用户可以手动截图，系统也可以在告警发生时自动截图。

### 截图保存规则

```text
data/
└── snapshots/
    ├── 2026-06-28/
    │   ├── source_1_153012_motion.jpg
    │   └── source_2_153100_intrusion.jpg
```

### 截图命名格式

```text
source_{sourceId}_{HHMMSS}_{eventType}.jpg
```

### 验收标准

1. 手动点击截图按钮可以保存当前画面。
2. 告警发生时可以自动保存截图。
3. 截图路径写入告警事件表。
4. 文件保存失败时需要有错误提示。

---

## 7.10 录像模块

### 第一阶段

只做手动录像：

1. 用户点击开始录像。
2. 用户点击停止录像。
3. 保存为 mp4 或 avi 文件。

### 第二阶段

做告警前后录像：

1. 告警发生前缓存最近 N 秒视频帧。
2. 告警发生后继续录制 M 秒。
3. 合成一个事件片段。

### 验收标准

第一阶段：

1. 可以对单路视频手动开始录像。
2. 可以停止录像。
3. 录像文件可以正常播放。

第二阶段：

1. 告警发生后能生成事件视频片段。
2. 事件视频路径写入数据库。
3. 告警中心可以打开事件视频。

---

## 7.11 性能监控模块

### 需求描述

系统需要实时统计每路视频流的运行状态。

### 统计指标

```text
sourceId
fps
decodeTimeMs
analyzeTimeMs
renderTimeMs
queueSize
droppedFrames
memoryUsage
lastFrameTime
status
```

### 显示方式

1. 每个视频窗口显示当前 FPS。
2. 性能面板显示所有视频源的性能数据。
3. 异常数据用醒目方式展示。

### 优化策略

1. 帧队列设置最大长度。
2. 队列满时丢弃旧帧。
3. 分析模块可以隔帧运行。
4. UI 渲染可以降帧。
5. 低优先级视频源可以主动降低分析频率。

### 验收标准

1. 可以实时看到每路视频 FPS。
2. 可以看到队列长度。
3. 可以看到丢帧数量。
4. 可以看到分析耗时。
5. 多路视频播放时性能数据持续更新。

---

## 7.12 数据库存储模块

### 数据库文件

```text
data/app.db
```

### 表一：video_sources

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

### 表二：alarm_events

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

### 表三：roi_rules

```sql
CREATE TABLE IF NOT EXISTS roi_rules (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    source_id INTEGER NOT NULL,
    rule_name TEXT NOT NULL,
    enabled INTEGER NOT NULL DEFAULT 1,
    roi_type TEXT NOT NULL,
    points_json TEXT NOT NULL,
    sensitivity REAL NOT NULL DEFAULT 0.5,
    min_target_area INTEGER NOT NULL DEFAULT 500,
    cooldown_seconds INTEGER NOT NULL DEFAULT 10,
    created_at TEXT NOT NULL,
    updated_at TEXT NOT NULL
);
```

### 表四：app_settings

```sql
CREATE TABLE IF NOT EXISTS app_settings (
    key TEXT PRIMARY KEY,
    value TEXT NOT NULL
);
```

### 验收标准

1. 程序启动时自动创建数据库。
2. 视频源可以保存和读取。
3. 告警事件可以保存和读取。
4. ROI 规则可以保存和读取。
5. 数据库操作失败时不能导致程序崩溃。

---

## 8. UI 页面需求

## 8.1 DashboardPage

首页仪表盘。

显示内容：

1. 当前视频源数量。
2. 在线视频源数量。
3. 今日告警数量。
4. 当前运行中的分析任务数量。
5. 最近 10 条告警。
6. 系统整体 FPS 概览。

---

## 8.2 VideoWallPage

核心视频墙页面。

功能：

1. 显示 1 / 4 / 9 / 16 宫格。
2. 每个格子显示一路视频。
3. 支持播放、暂停、停止、截图。
4. 支持全屏查看单路视频。
5. 支持显示检测框。
6. 支持显示 ROI 区域。
7. 支持显示视频状态和 FPS。

---

## 8.3 SourceManagerPage

视频源管理页面。

功能：

1. 视频源列表。
2. 新增视频源。
3. 编辑视频源。
4. 删除视频源。
5. 启用 / 禁用视频源。
6. 测试连接。
7. 查看最近错误。

---

## 8.4 AlarmCenterPage

告警中心页面。

功能：

1. 告警列表。
2. 告警详情。
3. 截图预览。
4. 按视频源过滤。
5. 按事件类型过滤。
6. 按时间范围过滤。
7. 确认告警。
8. 删除告警。

---

## 8.5 PlaybackPage

历史回放页面。

第一版可以简单实现：

1. 打开本地录像文件。
2. 播放、暂停、拖动进度条。

高级版实现：

1. 从告警事件进入回放。
2. 播放告警关联视频片段。
3. 在时间线上展示事件点。

---

## 8.6 SettingsPage

系统设置页面。

功能：

1. 截图保存路径。
2. 录像保存路径。
3. 默认视频墙布局。
4. 是否启用运动检测。
5. 是否启用 ROI 告警。
6. 分析间隔帧数。
7. 最大帧队列长度。
8. 告警冷却时间。
9. 日志级别。

---

## 9. 开发路线图

## 阶段 0：项目初始化

建议耗时：第 1 天

### 目标

创建可运行的 Qt/QML/CMake 项目骨架。

### 任务

1. 创建项目目录结构。
2. 配置 CMake。
3. 创建 main.cpp。
4. 创建 Main.qml。
5. 实现基础窗口。
6. 添加 qml 资源加载。
7. 添加 README.md。
8. 添加 docs/architecture.md。
9. 添加基础日志模块。
10. 确保项目可以编译运行。

### 验收标准

1. 项目可以成功构建。
2. 启动后显示主窗口。
3. QML 页面加载正常。
4. README 包含项目简介和构建方式。
5. 不引入视频、数据库、AI 相关复杂逻辑。

### 给 Codex 的任务提示

```text
请根据 PROJECT_REQUIREMENTS.md 的“阶段 0：项目初始化”完成项目骨架。

要求：
1. 使用 C++17 或 C++20、Qt 6、QML、CMake。
2. 创建推荐目录结构。
3. 实现 main.cpp 和 qml/Main.qml。
4. 主窗口显示应用标题、左侧导航栏占位、右侧内容区占位。
5. 添加 README.md，写明构建方式。
6. 不要实现视频播放、数据库、AI 检测。
7. 修改完成后说明改动文件、构建方式和下一步建议。
```

---

## 阶段 1：基础 UI 框架

建议耗时：第 2 天

### 目标

完成主界面布局和页面导航。

### 任务

1. 创建 DashboardPage.qml。
2. 创建 VideoWallPage.qml。
3. 创建 SourceManagerPage.qml。
4. 创建 AlarmCenterPage.qml。
5. 创建 SettingsPage.qml。
6. 实现左侧导航栏。
7. 实现页面切换。
8. 添加统一主题 AppTheme.qml。
9. 添加基础组件 Toolbar.qml。

### 验收标准

1. 左侧导航可以切换页面。
2. 每个页面有清晰标题和占位内容。
3. 界面风格统一。
4. QML 文件结构清晰。
5. 没有复杂业务逻辑堆在 Main.qml。

### 给 Codex 的任务提示

```text
请完成“阶段 1：基础 UI 框架”。

要求：
1. 创建 qml/pages 和 qml/components 下的基础页面和组件。
2. Main.qml 只负责整体布局和页面切换，不要堆大量业务逻辑。
3. 左侧导航包含：首页、视频墙、视频源、告警中心、回放、设置。
4. 每个页面先做静态 UI 占位。
5. 使用统一的 AppTheme.qml 管理颜色、间距、字体大小。
6. 修改完成后列出新增文件，并说明如何运行验证。
```

---

## 阶段 2：视频源数据模型

建议耗时：第 3 天

### 目标

实现 C++ VideoSource 数据结构和 QML 可绑定的视频源列表。

### 任务

1. 定义 VideoSource 类型。
2. 定义 VideoSourceStatus 枚举。
3. 实现 VideoSourceModel，继承 QAbstractListModel。
4. 暴露给 QML。
5. SourceManagerPage 显示视频源列表。
6. 支持添加本地视频源占位数据。
7. 支持删除视频源。
8. 支持修改视频源名称。

### 验收标准

1. QML 能显示 C++ Model 数据。
2. 可以新增视频源。
3. 可以删除视频源。
4. UI 列表自动刷新。
5. Model 角色命名清晰。

### 给 Codex 的任务提示

```text
请完成“阶段 2：视频源数据模型”。

要求：
1. 在 C++ 中实现 VideoSource、VideoSourceStatus、VideoSourceModel。
2. VideoSourceModel 继承 QAbstractListModel。
3. 至少提供 name、type、url、enabled、status、lastError 这些 role。
4. 将 VideoSourceModel 注册或设置到 QML 上下文。
5. SourceManagerPage.qml 显示视频源列表，并支持添加、删除、编辑名称。
6. 暂时不接数据库，也不真正播放视频。
7. 保持 C++ 和 QML 职责清晰。
```

---

## 阶段 3：SQLite 数据库存储

建议耗时：第 4 天

### 目标

实现视频源持久化。

### 任务

1. 添加 DatabaseManager。
2. 启动时创建 data/app.db。
3. 创建 video_sources 表。
4. 创建 VideoSourceRepository。
5. 程序启动时加载视频源。
6. 添加视频源时写入数据库。
7. 删除视频源时同步删除数据库记录。
8. 修改视频源时同步更新数据库。

### 验收标准

1. 视频源重启后仍然存在。
2. 数据库文件自动创建。
3. 数据库操作失败有错误日志。
4. 不因为数据库错误导致 UI 崩溃。
5. Repository 不直接依赖 QML。

### 给 Codex 的任务提示

```text
请完成“阶段 3：SQLite 数据库存储”。

要求：
1. 使用 Qt SQL 模块实现 SQLite 存储。
2. 新增 DatabaseManager，负责连接数据库和初始化表。
3. 新增 VideoSourceRepository，负责 video_sources 表的增删改查。
4. VideoSourceModel 通过 repository 加载和保存数据。
5. SourceManagerPage 中新增、删除、编辑视频源后需要持久化。
6. 启动程序后自动加载历史视频源。
7. 不要引入告警表、ROI 表，当前只做 video_sources。
```

---

## 阶段 4：单路本地视频播放 MVP

建议耗时：第 5 到第 6 天

### 目标

先打通单路视频播放链路。

### 任务

1. 实现 OpenCVVideoDecoder。
2. 支持打开本地 mp4 文件。
3. 实现 StreamSession 基础版本。
4. 使用后台线程读取视频帧。
5. 将视频帧转换为 QImage。
6. QML VideoTile 显示当前帧。
7. 支持播放、暂停、停止。
8. 显示基础 FPS。

### 验收标准

1. 能选择一个本地 mp4 并播放。
2. 播放时 UI 可以继续操作。
3. 暂停和停止功能正常。
4. 视频结束后状态正确变为 Stopped。
5. 不要求此阶段支持 RTSP。
6. 不要求此阶段支持多路播放。

### 给 Codex 的任务提示

```text
请完成“阶段 4：单路本地视频播放 MVP”。

要求：
1. 使用 OpenCV VideoCapture 实现 OpenCVVideoDecoder。
2. 新增 StreamSession，负责单路视频的启动、暂停、停止。
3. 解码逻辑必须运行在后台线程，不能阻塞 QML UI。
4. 将 cv::Mat 转换为 QImage，并传递给 QML 显示。
5. 在 VideoWallPage 中放置一个 VideoTile，支持播放、暂停、停止。
6. 显示当前 FPS 和视频状态。
7. 先只支持本地 mp4 文件，不要做 RTSP、AI、告警。
8. 注意线程退出和资源释放。
```

---

## 阶段 5：四路视频墙

建议耗时：第 7 到第 8 天

### 目标

实现 4 路视频同时播放。

### 任务

1. VideoWallPage 支持 1 宫格和 4 宫格。
2. 每个 VideoTile 绑定一个 StreamSession。
3. 支持每路独立播放、暂停、停止。
4. 支持每路独立 FPS。
5. 单路异常不影响其他路。
6. 增加 StreamSessionManager。

### 验收标准

1. 可以同时播放 4 个本地视频。
2. 每个视频窗口状态独立。
3. 停止一路不会影响其他路。
4. 切换 1 宫格和 4 宫格不崩溃。
5. UI 仍可响应点击操作。

### 给 Codex 的任务提示

```text
请完成“阶段 5：四路视频墙”。

要求：
1. 新增 StreamSessionManager，统一管理多个 StreamSession。
2. VideoWallPage 支持 1 宫格和 4 宫格布局切换。
3. 每个 VideoTile 绑定不同的视频源。
4. 每路视频可以独立播放、暂停、停止。
5. 显示每路 FPS 和状态。
6. 单个视频源打开失败时，只影响对应 tile，不影响其他 tile。
7. 不要引入运动检测、告警、RTSP。
```

---

## 阶段 6：线程安全 FrameQueue

建议耗时：第 9 天

### 目标

引入帧队列，为后续分析、渲染解耦做准备。

### 任务

1. 定义 Frame 结构。
2. 实现线程安全 FrameQueue。
3. 支持最大容量。
4. 队列满时丢弃旧帧。
5. 统计 droppedFrames。
6. 添加单元测试。
7. StreamSession 使用 FrameQueue。

### 验收标准

1. FrameQueue push/pop 正常。
2. 多线程情况下不崩溃。
3. 队列满时不会无限增长。
4. droppedFrames 统计正确。
5. StreamSession 停止后队列清空。

### 给 Codex 的任务提示

```text
请完成“阶段 6：线程安全 FrameQueue”。

要求：
1. 新增 Frame 结构，包含 image、timestamp、width、height、frameIndex 等字段。
2. 新增 FrameQueue，使用 mutex + condition_variable 或 Qt 同等机制实现线程安全。
3. FrameQueue 支持 maxSize。
4. 队列满时丢弃旧帧，并增加 droppedFrames 计数。
5. 为 FrameQueue 添加基础单元测试。
6. StreamSession 内部改为通过 FrameQueue 解耦解码和消费。
7. 不要改变现有 UI 行为。
```

---

## 阶段 7：性能监控面板

建议耗时：第 10 天

### 目标

实现每路视频的性能指标统计。

### 任务

1. 定义 StreamPerformance 数据结构。
2. 实现 PerformanceModel。
3. 统计 FPS。
4. 统计解码耗时。
5. 统计队列长度。
6. 统计丢帧数量。
7. QML 显示性能面板。
8. VideoTile 上显示 FPS。

### 验收标准

1. 每路视频有独立 FPS。
2. 可以看到 droppedFrames。
3. 可以看到 queueSize。
4. 性能面板每秒刷新。
5. 不频繁刷新导致 UI 卡顿。

### 给 Codex 的任务提示

```text
请完成“阶段 7：性能监控面板”。

要求：
1. 新增 StreamPerformance 结构。
2. 新增 PerformanceModel，继承 QAbstractListModel。
3. 每个 StreamSession 定期更新 fps、decodeTimeMs、queueSize、droppedFrames。
4. VideoTile 显示当前 FPS。
5. 新增 PerformancePanel.qml，在视频墙页面或首页显示所有视频源性能数据。
6. 刷新频率控制在每秒 1 次左右，不要每帧刷新 QML。
```

---

## 阶段 8：截图功能

建议耗时：第 11 天

### 目标

实现手动截图。

### 任务

1. 新增 SnapshotManager。
2. 支持保存当前视频帧。
3. 按日期创建目录。
4. 文件名包含 sourceId 和时间。
5. VideoTile 添加截图按钮。
6. 截图成功后显示提示。

### 验收标准

1. 播放视频时点击截图可以保存 jpg 或 png。
2. 截图路径正确。
3. 文件可以正常打开。
4. 截图失败时显示错误。
5. 不影响视频播放。

### 给 Codex 的任务提示

```text
请完成“阶段 8：截图功能”。

要求：
1. 新增 SnapshotManager，负责保存 QImage 到本地文件。
2. 默认保存到 data/snapshots/YYYY-MM-DD/。
3. 文件名包含 sourceId、时间、事件类型或 manual。
4. VideoTile 添加截图按钮。
5. 截图成功或失败需要给 QML 返回结果。
6. 不要做告警自动截图，当前只做手动截图。
```

---

## 阶段 9：基础运动检测

建议耗时：第 12 到第 14 天

### 目标

实现传统算法运动检测。

### 任务

1. 新增 IVideoAnalyzer。
2. 新增 MotionDetector。
3. 使用帧差法检测运动区域。
4. 输出 bounding boxes。
5. QML 视频画面叠加检测框。
6. 支持开启 / 关闭运动检测。
7. 支持设置最小面积。

### 验收标准

1. 有明显运动时出现检测框。
2. 静止画面误报较少。
3. 检测运行在后台线程。
4. 检测框能显示在对应视频画面上。
5. 关闭检测后不再显示检测框。

### 给 Codex 的任务提示

```text
请完成“阶段 9：基础运动检测”。

要求：
1. 新增 IVideoAnalyzer 接口。
2. 新增 MotionDetector，使用 OpenCV 帧差法实现运动检测。
3. 输出 DetectionResult，包含 sourceId、timestamp、boxes。
4. boxes 需要能传递给 QML，在 VideoTile 上叠加显示矩形框。
5. 增加开关：启用或禁用运动检测。
6. 增加参数：最小检测面积 minTargetArea。
7. 检测逻辑不能阻塞 UI。
8. 不要做 ROI 入侵和数据库告警。
```

---

## 阶段 10：告警事件模型和数据库

建议耗时：第 15 到第 16 天

### 目标

实现告警事件的存储和展示。

### 任务

1. 创建 alarm_events 表。
2. 定义 AlarmEvent。
3. 实现 AlarmEventRepository。
4. 实现 AlarmEventModel。
5. AlarmCenterPage 显示告警列表。
6. 支持确认告警。
7. 支持删除告警。

### 验收标准

1. 告警事件可以写入 SQLite。
2. 重启后可以读取历史告警。
3. 告警中心能显示事件列表。
4. 可以确认告警。
5. 可以删除告警。

### 给 Codex 的任务提示

```text
请完成“阶段 10：告警事件模型和数据库”。

要求：
1. 新增 alarm_events 表。
2. 新增 AlarmEvent 数据结构。
3. 新增 AlarmEventRepository，负责数据库增删改查。
4. 新增 AlarmEventModel，继承 QAbstractListModel。
5. AlarmCenterPage 显示告警列表。
6. 支持确认告警和删除告警。
7. 先手动插入测试告警验证功能，不要立即接入运动检测。
```

---

## 阶段 11：运动检测告警

建议耗时：第 17 天

### 目标

将运动检测结果接入告警中心。

### 任务

1. 新增 AlarmManager。
2. MotionDetector 检测到运动后生成 MotionDetected 事件。
3. 告警发生时自动截图。
4. 告警写入数据库。
5. 告警中心实时刷新。
6. 增加告警冷却时间。

### 验收标准

1. 有运动时生成告警。
2. 告警包含 sourceName、eventType、timestamp。
3. 自动截图路径写入数据库。
4. 告警中心能看到最新告警。
5. 同一视频源不会每帧疯狂生成告警。

### 给 Codex 的任务提示

```text
请完成“阶段 11：运动检测告警”。

要求：
1. 新增 AlarmManager，统一负责生成告警。
2. MotionDetector 检测到运动后，通过 StreamSession 或 Controller 通知 AlarmManager。
3. AlarmManager 写入 alarm_events 表。
4. 告警发生时调用 SnapshotManager 自动截图。
5. 告警中心实时显示新告警。
6. 增加 cooldownSeconds，避免每帧产生告警。
7. 保持模块解耦，MotionDetector 不要直接操作数据库。
```

---

## 阶段 12：ROI 区域绘制

建议耗时：第 18 到第 19 天

### 目标

实现用户在视频画面上绘制矩形 ROI。

### 任务

1. 新增 RoiRule 数据结构。
2. 新增 RoiEditor.qml。
3. 用户可以在视频上拖拽绘制矩形。
4. 显示 ROI 边框。
5. 支持删除 ROI。
6. 暂时只保存在内存。

### 验收标准

1. 可以在视频画面上画矩形 ROI。
2. ROI 位置和视频画面坐标匹配。
3. 可以删除 ROI。
4. 切换视频源时 ROI 不混乱。
5. 暂时不要求数据库持久化。

### 给 Codex 的任务提示

```text
请完成“阶段 12：ROI 区域绘制”。

要求：
1. 新增 RoiRule 结构，至少包含 sourceId、ruleName、rect、enabled。
2. 新增 RoiEditor.qml，支持在 VideoTile 上拖拽绘制矩形 ROI。
3. 绘制完成后将 ROI 数据传给 C++。
4. VideoTile 能显示当前视频源对应的 ROI。
5. 支持删除 ROI。
6. 当前阶段 ROI 只需要保存在内存，不要接数据库。
```

---

## 阶段 13：ROI 持久化和入侵检测

建议耗时：第 20 到第 22 天

### 目标

实现 ROI 规则保存和区域入侵告警。

### 任务

1. 创建 roi_rules 表。
2. 实现 RoiRuleRepository。
3. ROI 保存到 SQLite。
4. 启动时加载 ROI。
5. IntrusionDetector 判断检测框是否进入 ROI。
6. 生成 IntrusionDetected 告警。
7. 支持 ROI 告警冷却时间。

### 验收标准

1. ROI 重启后仍存在。
2. 目标进入 ROI 时生成入侵告警。
3. 目标在 ROI 外时不生成入侵告警。
4. 告警包含规则名称。
5. 同一规则不会频繁刷屏。

### 给 Codex 的任务提示

```text
请完成“阶段 13：ROI 持久化和入侵检测”。

要求：
1. 新增 roi_rules 表。
2. 新增 RoiRuleRepository，负责 ROI 规则增删改查。
3. RoiEditor 绘制完成后保存 ROI 到数据库。
4. 启动时加载 ROI。
5. 新增 IntrusionDetector，判断 MotionDetector 输出的 box 是否进入 ROI。
6. 进入 ROI 时通过 AlarmManager 生成 IntrusionDetected 告警。
7. 增加每条规则的 cooldownSeconds。
8. 保持 MotionDetector、IntrusionDetector、AlarmManager 职责分离。
```

---

## 阶段 14：RTSP 接入和 FFmpeg 解码

建议耗时：第 23 到第 27 天

### 目标

引入 FFmpeg，实现 RTSP 视频流播放。

### 任务

1. 新增 FFmpegVideoDecoder。
2. 支持打开 RTSP URL。
3. 读取视频流信息。
4. 解码视频帧。
5. 转换为 QImage 或内部 Frame。
6. 支持超时。
7. 支持错误状态。
8. StreamSession 支持选择 OpenCVDecoder 或 FFmpegDecoder。
9. 视频源类型新增 RTSP。

### 验收标准

1. 可以添加 RTSP 视频源。
2. 可以播放 RTSP 流。
3. RTSP 地址错误时显示错误。
4. RTSP 断开时不导致程序崩溃。
5. 资源释放正确。
6. 本地视频播放功能不能被破坏。

### 给 Codex 的任务提示

```text
请完成“阶段 14：RTSP 接入和 FFmpeg 解码”。

要求：
1. 新增 FFmpegVideoDecoder，实现 IVideoDecoder 接口。
2. 使用 FFmpeg 打开本地视频或 RTSP 视频流。
3. 解码视频帧并转换为 Frame。
4. VideoSource 新增 RTSP 类型。
5. StreamSession 根据视频源类型选择 OpenCVVideoDecoder 或 FFmpegVideoDecoder。
6. RTSP 打开失败时需要进入 Error 状态并保存 lastError。
7. 断线时不能崩溃。
8. 注意释放 FFmpeg 相关资源。
9. 不要在这个阶段做硬件解码、AI 推理。
```

---

## 阶段 15：自动重连机制

建议耗时：第 28 天

### 目标

提高 RTSP 视频源稳定性。

### 任务

1. StreamSession 检测连续解码失败。
2. 进入 Reconnecting 状态。
3. 延迟几秒后重新打开视频源。
4. 限制最大重连次数。
5. QML 显示重连状态。
6. 失败后进入 Error 状态。

### 验收标准

1. RTSP 断线后进入 Reconnecting。
2. 网络恢复后可以重新播放。
3. 超过最大重连次数后进入 Error。
4. 重连过程 UI 不阻塞。
5. 用户可以手动停止重连。

### 给 Codex 的任务提示

```text
请完成“阶段 15：自动重连机制”。

要求：
1. 在 StreamSession 中增加重连状态机。
2. 连续读取帧失败后进入 Reconnecting。
3. 按固定间隔重连，最大重连次数可配置。
4. QML VideoTile 显示 Reconnecting 状态。
5. 用户点击停止时必须中断重连。
6. 不要影响本地视频文件播放逻辑。
```

---

## 阶段 16：录像功能

建议耗时：第 29 到第 31 天

### 目标

实现手动录像。

### 任务

1. 新增 RecordingManager。
2. 支持开始录像。
3. 支持停止录像。
4. 保存录像文件。
5. VideoTile 添加录像按钮。
6. 录像状态显示在 UI 上。

### 验收标准

1. 点击开始录像后能保存视频。
2. 点击停止录像后文件关闭。
3. 录像文件可播放。
4. 录像失败时显示错误。
5. 录像不明显影响播放流畅度。

### 给 Codex 的任务提示

```text
请完成“阶段 16：录像功能”。

要求：
1. 新增 RecordingManager。
2. 支持对单路视频开始和停止录像。
3. 录像文件保存到 data/records/YYYY-MM-DD/。
4. VideoTile 添加录像按钮和录像状态显示。
5. 录像失败需要返回错误。
6. 先做手动录像，不要做告警前后录像。
```

---

## 阶段 17：自定义 VideoRenderItem

建议耗时：第 32 到第 36 天

### 目标

优化视频渲染方式，提升项目技术含量。

### 任务

1. 新增 VideoRenderItem。
2. 继承 QQuickItem。
3. 将视频帧交给自定义渲染组件显示。
4. 检测框和 ROI 叠加绘制。
5. 替换第一阶段 QImage + Image 的临时方案。
6. 保留旧方案作为 fallback。

### 验收标准

1. QML 可以使用 VideoRenderItem。
2. 视频显示正常。
3. 检测框显示正常。
4. ROI 显示正常。
5. 四路播放时 UI 响应更稳定。
6. 旧渲染方案可以保留但默认使用新方案。

### 给 Codex 的任务提示

```text
请完成“阶段 17：自定义 VideoRenderItem”。

要求：
1. 新增 VideoRenderItem，继承 QQuickItem。
2. 将它注册到 QML。
3. QML VideoTile 使用 VideoRenderItem 显示视频。
4. 支持绘制视频帧、检测框、ROI 区域。
5. 不要一次性做复杂 GPU 零拷贝，先保证功能正确。
6. 保留原 QImage 显示方案作为 fallback，避免大改导致项目不可运行。
```

---

## 阶段 18：ONNX Runtime AI 检测

建议耗时：第 37 到第 45 天

### 目标

集成目标检测模型，实现人车检测。

### 任务

1. 新增 OnnxObjectDetector。
2. 加载 ONNX 模型。
3. 对视频帧进行预处理。
4. 执行推理。
5. 后处理输出检测框。
6. 支持置信度过滤。
7. 支持隔帧推理。
8. 检测结果叠加到视频画面。
9. 可以生成 PersonDetected / VehicleDetected 告警。

### 验收标准

1. 能成功加载 ONNX 模型。
2. 能对单张图片或单帧视频输出检测框。
3. 视频播放时可以显示检测框。
4. 推理不会阻塞 UI。
5. 可以设置推理间隔。
6. 可以设置置信度阈值。
7. 关闭 AI 检测后系统仍可正常运行。

### 给 Codex 的任务提示

```text
请完成“阶段 18：ONNX Runtime AI 检测”。

要求：
1. 新增 OnnxObjectDetector，实现 IVideoAnalyzer 或新的 IObjectDetector 接口。
2. 支持加载 ONNX 模型路径。
3. 实现图像预处理、推理、后处理。
4. 输出 DetectionResult，包含类别、置信度、box。
5. 支持 confidenceThreshold 和 inferenceInterval。
6. 推理必须异步执行，不能阻塞 UI。
7. 检测结果需要能叠加显示在 VideoTile 上。
8. 如果 ONNX Runtime 未配置成功，需要优雅降级，不影响已有运动检测功能。
```

---

## 10. 非功能性需求

## 10.1 性能要求

基础版：

1. 单路 1080p 本地视频播放流畅。
2. 4 路 720p 本地视频可以同时播放。
3. UI 操作无明显卡顿。
4. 截图操作不阻塞播放。
5. 告警写入数据库不阻塞 UI。

进阶版：

1. 支持 4 路 RTSP 同时播放。
2. 支持运动检测开启后仍保持基本流畅。
3. 支持分析模块隔帧运行。
4. 支持帧队列主动丢帧。
5. 支持重连机制。

高级版：

1. 支持 9 路视频墙。
2. 支持 AI 检测异步推理。
3. 支持自定义渲染组件。
4. 支持推理限流。
5. 支持性能面板定位瓶颈。

---

## 10.2 稳定性要求

1. 单路视频打开失败不能导致程序崩溃。
2. 单路视频断开不能影响其他视频。
3. 停止播放后线程必须退出。
4. 退出程序时释放所有视频资源。
5. 数据库写入失败需要记录日志。
6. 截图和录像路径不存在时自动创建。
7. 用户输入非法 RTSP URL 时给出错误提示。
8. AI 模型加载失败时禁用 AI 功能，不影响基础播放。

---

## 10.3 代码质量要求

1. C++ 类职责清晰。
2. QML 不直接写复杂业务逻辑。
3. 视频解码、分析、告警、存储模块解耦。
4. 不要把所有逻辑塞进 Main.qml。
5. 不要把所有逻辑塞进 AppController。
6. 每个阶段完成后更新 README 或 docs。
7. 每个复杂模块至少有一个说明文档。
8. 关键数据结构使用清晰命名。
9. 公共接口加必要注释。
10. 不随意引入无关第三方库。

---

## 11. Codex 工作规则

Codex 每次只能完成一个阶段或一个明确子任务。

### 每次执行前

Codex 必须先做：

1. 阅读 PROJECT_REQUIREMENTS.md。
2. 阅读 README.md。
3. 查看当前目录结构。
4. 查看相关已有文件。
5. 给出简短实现计划。

### 每次执行时

Codex 必须遵守：

1. 只改当前任务相关文件。
2. 不要顺手重构无关模块。
3. 不要一次性实现多个阶段。
4. 不要擅自引入大型依赖。
5. 不要删除已有功能。
6. 保持项目可以编译。
7. 修改完成后说明改动内容。
8. 修改完成后说明如何验证。

### 每次执行后

Codex 必须输出：

```text
完成内容：
- ...

修改文件：
- ...

验证方式：
- ...

风险和 TODO：
- ...

下一阶段建议：
- ...
```

---

## 12. 推荐每日开发安排

```text
第 1 天：项目初始化
第 2 天：基础 UI 框架
第 3 天：视频源数据模型
第 4 天：SQLite 视频源持久化
第 5-6 天：单路本地视频播放
第 7-8 天：四路视频墙
第 9 天：FrameQueue
第 10 天：性能监控
第 11 天：截图功能
第 12-14 天：基础运动检测
第 15-16 天：告警事件模型和数据库
第 17 天：运动检测告警
第 18-19 天：ROI 区域绘制
第 20-22 天：ROI 持久化和入侵检测
第 23-27 天：FFmpeg + RTSP 接入
第 28 天：自动重连机制
第 29-31 天：录像功能
第 32-36 天：自定义 VideoRenderItem
第 37-45 天：ONNX Runtime AI 检测
```

---

## 13. 最终简历写法

项目名称：

基于 C++/Qt/QML 的实时多路视频流智能分析平台

项目描述：

设计并实现一款面向安防监控和工业视觉场景的桌面端实时视频分析系统，支持本地视频、摄像头和 RTSP 视频流接入，多路视频墙展示、运动检测、区域入侵告警、截图录像、历史事件查询和性能监控。项目采用 C++ 负责视频解码、帧队列、图像分析、告警处理和数据存储，QML 负责视频墙、告警中心、ROI 编辑器和可视化交互界面。

简历亮点：

```text
- 基于 C++/Qt/QML 设计实时多路视频流分析平台，支持本地视频、摄像头和 RTSP 流统一接入，实现多路视频墙、播放控制、截图录像和告警中心等功能。
- 设计 StreamSession + FrameQueue 视频处理流水线，将解码、分析、渲染和告警模块解耦，提升多路视频并发处理稳定性。
- 使用 QThread/QtConcurrent 将视频解码、图像分析、截图录像等耗时任务移至后台线程，通过信号槽向 QML 前端同步状态、性能指标和告警事件。
- 实现线程安全帧队列，支持最大容量限制、队列满丢弃旧帧、丢帧统计和主动限流，避免多路视频场景下内存持续增长。
- 基于 OpenCV 实现运动检测和 ROI 区域入侵检测，支持检测框叠加、告警冷却、自动截图和历史事件存储。
- 使用 SQLite 存储视频源配置、ROI 规则和告警事件，支持重启恢复、事件查询、告警确认和截图预览。
- 基于 FFmpeg 实现 RTSP 视频流接入，支持异常处理、断线重连和资源释放，提升实时流播放稳定性。
- 封装自定义 QQuickItem 视频渲染组件，实现视频帧、检测框、ROI 区域和状态信息的统一叠加绘制，降低 QML 层对象数量。
- 实现性能监控模块，统计 FPS、解码耗时、分析耗时、队列长度和丢帧数量，并通过隔帧分析、队列限长和主动丢帧策略优化实时性。
```

---

## 14. 最小可交付版本 MVP

如果时间有限，至少完成以下内容：

```text
1. Qt/QML 主界面
2. 视频源管理
3. SQLite 持久化
4. 单路本地视频播放
5. 四路视频墙
6. FPS 显示
7. 截图
8. 运动检测
9. 告警中心
10. 告警自动截图
```

完成 MVP 后，简历已经可以写。

---

## 15. 高级加分版本

如果想让项目更“牛逼”，继续完成：

```text
1. RTSP 接入
2. FFmpeg 解码
3. 断线重连
4. ROI 区域入侵
5. 手动录像
6. 自定义 QQuickItem 渲染
7. ONNX Runtime AI 检测
8. 9/16 路视频墙
9. 性能监控面板
10. 插件化检测算法
```

---

## 16. 不建议一开始就做的内容

以下内容先不要做，容易拖死项目：

```text
1. 一开始就上 YOLO。
2. 一开始就写复杂 GPU 渲染。
3. 一开始就接多种协议。
4. 一开始就做 16 路视频。
5. 一开始就做完整权限系统。
6. 一开始就做服务端。
7. 一开始就做云端存储。
8. 一开始就做复杂美化。
9. 一开始就做跨平台安装包。
10. 一开始就重构成过度复杂的插件系统。
```

正确顺序是：

```text
先能跑
再能多路
再能检测
再能告警
再能 RTSP
再做性能优化
最后做 AI 和自定义渲染
```
