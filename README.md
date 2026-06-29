# 实时多路视频流智能分析平台

这是一个基于 C++17、Qt 6、QML 和 CMake 的桌面端项目骨架。当前已完成“阶段 1：基础 UI 框架”，包含主程序入口、QML 主窗口、左侧导航、静态页面占位、基础工具栏和统一主题。

本阶段不包含视频播放、数据库、AI 检测、OpenCV、FFmpeg 或 ONNX Runtime 相关实现。

## 环境要求

- Visual Studio 2022
- Qt 6，建议安装 MSVC 2022 64-bit 套件
- CMake 3.21 或更高版本

## 构建方式

请将下面命令中的 `C:\Qt\6.x.x\msvc2022_64` 替换为本机实际 Qt 6 安装路径。

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="C:\Qt\6.x.x\msvc2022_64"
cmake --build build --config Release
.\build\Release\VideoAnalysisPlatform.exe
```

也可以使用 VS2022 直接打开项目根目录，选择对应的 Qt 6 CMake Kit 后进行配置、构建和运行。

## 当前目录说明

- `src/`：C++ 源码目录，当前包含程序入口。
- `qml/`：QML 界面目录，当前包含主窗口、页面、基础组件和主题。
- `docs/`：项目文档目录。
- `assets/`：图标和示例资源目录。
- `tests/`：后续测试代码目录。
- `third_party/`：后续第三方库或说明目录。

## 下一步建议

下一阶段可以进入视频源数据模型设计，但仍不要提前接入数据库、视频播放或 AI 检测。
