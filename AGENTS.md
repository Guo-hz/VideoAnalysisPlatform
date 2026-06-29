# 项目作战手册

## 项目用途

本项目是“实时多路视频流智能分析平台”的 Qt/QML 桌面端工程。长期目标是面向安防监控、工业视觉和视频分析场景，逐步实现视频源管理、多路视频墙、告警、截图录像、性能监控、RTSP/FFmpeg 和 AI 检测等能力。

当前状态：已完成阶段 1 基础 UI 框架。现有代码实现 Qt/QML/CMake 启动窗口、左侧导航、静态页面占位、基础工具栏和统一主题，不包含视频播放、数据库、AI 检测、OpenCV、FFmpeg 或 ONNX Runtime。

## 技术栈

- C++17
- Qt 6
- QML / Qt Quick / Qt Quick Controls
- CMake
- VS2022 + MSVC 2022 64-bit 作为当前推荐 Windows 构建环境

## 重要目录

- `CMakeLists.txt`：顶层 CMake 配置，定义 `VideoAnalysisPlatform` 可执行目标和 QML 模块。
- `src/main.cpp`：程序入口，创建 `QGuiApplication` 并加载 QML。
- `qml/Main.qml`：当前主窗口，只负责整体布局、左侧导航和页面切换。
- `PROJECT_REQUIREMENTS.md`：完整需求、路线图、推荐目录结构和阶段任务来源。
- `README.md`：项目简介与本地构建命令。
- `docs/`：后续项目文档目录，目前未填充。
- `src/app`、`src/core`、`src/models`、`src/render`：后续 C++ 模块目录，目前是骨架目录。
- `qml/pages`：基础页面目录，包含首页、视频墙、视频源、告警中心、回放和设置的静态占位页面。
- `qml/components`：基础组件目录，当前包含导航项、页面占位容器和工具栏。
- `qml/styles/AppTheme.qml`：统一管理颜色、间距、圆角和字体大小。

## 构建和运行

当前 README 推荐命令：

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="C:\Qt\6.x.x\msvc2022_64"
cmake --build build --config Release
.\build\Release\VideoAnalysisPlatform.exe
```

注意：`C:\Qt\6.x.x\msvc2022_64` 必须替换为本机实际 Qt 6 MSVC 目录。也可以用 VS2022 直接打开项目根目录，通过 CMake 工程配置、构建和运行。

## 当前入口和加载流程

1. `src/main.cpp` 创建 `QGuiApplication`。
2. `QQmlApplicationEngine` 加载 `qrc:/qt/qml/VideoAnalysisPlatform/qml/Main.qml`。
3. QML 创建 `ApplicationWindow`，显示左侧导航并通过 `StackLayout` 切换静态页面。
4. 若 QML 根对象创建失败，程序以错误码退出。

## 代码约定

- 每次只完成一个明确阶段或子任务，避免一次性跨阶段实现。
- QML 只负责界面、布局和交互展示，复杂业务逻辑放到 C++ 层。
- 不要把后续业务逻辑继续堆进 `qml/Main.qml`；页面内容放在 `qml/pages`，复用控件放在 `qml/components`，视觉常量放在 `qml/styles/AppTheme.qml`。
- C++ 模块按职责放置：应用控制在 `src/app`，核心处理在 `src/core`，QML 数据模型在 `src/models`，渲染相关在 `src/render`。
- 不要擅自新增大型依赖或公共 API；OpenCV、FFmpeg、SQLite、ONNX Runtime 等应按路线图阶段引入。
- 修改复杂模块时同步更新 `README.md` 或 `docs/` 中长期有价值的说明。

## 禁止随意修改

- 不要在 UI 框架阶段提前实现视频播放、数据库、AI 检测、RTSP、FFmpeg 或录像功能。
- 不要删除 `PROJECT_REQUIREMENTS.md`，它是当前需求和路线图的主要依据。
- 不要无关重构 CMake、目录结构或 QML 模块 URI。

## 验证方法

- 最低验证：CMake 配置成功、构建成功、启动后能看到主窗口。
- QML 验证：窗口标题正常，左侧导航包含首页、视频墙、视频源、告警中心、回放、设置，点击后能切换静态页面。
- 当前没有单元测试。

## 当前风险和不确定信息

- 当前工作区存在 `.git` 目录，但 `git status` 报告这不是有效 Git 仓库，版本管理状态待确认。
- 当前终端是否能直接找到 `cmake.exe`、Qt 6 安装路径是否存在，取决于本机 VS2022/Qt 环境配置。
- `docs/`、`tests/`、多数推荐 C++ 源码目录目前仍为空，只是为后续阶段预留。
