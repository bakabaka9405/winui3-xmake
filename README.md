# winui3_xmake

一个基于 xmake 的 WinUI 3 + C++/WinRT 示例项目。

这个仓库展示了如何不依赖 Visual Studio 的 .sln/.vcxproj，直接通过 xmake 脚本完成 WinUI 3 应用的完整构建链：NuGet 还原、cppwinrt 投影生成、MIDL/WinMD 生成与合并、XAML 编译、PRI 资源打包，以及最终运行。

## 项目功能

- WinUI 3 桌面应用最小可运行示例（Hello World + 点击按钮改文案）。
- 启用 Windows App SDK Bootstrap（手写 `wWinMain`，避免 XAML 自动入口导致初始化缺失）。
- 自动启用高 DPI（`PER_MONITOR_AWARE_V2`）。
- 主窗口启用 Mica 背景（`MicaKind::BaseAlt`）。
- xmake 规则自动完成 WinUI 3 所需代码生成与资源打包。

## 技术栈与依赖

- C++20
- xmake（Windows 平台、x64）
- WinUI 3 / Windows App SDK
- C++/WinRT
- WebView2 元数据（用于投影输入）

NuGet 依赖版本在 `packages.config` 中定义，首次构建会自动还原到 `.xmake/nuget`。

## 目录说明

- `src/`：应用源码（`main.cpp`、`App.xaml*`、`MainWindow.xaml*`、IDL 等）
- `xmake/rules/`：xmake 规则入口
- `xmake/modules/winui3/`：WinUI 3 构建流程脚本（restore/cppwinrt/winmd/xaml/makepri）
- `xmake/scripts/`：Python 规划与输入生成脚本（路径解析、命令参数规划、XAML/makepri 输入生成）
- `.xmake/generated/`：构建时生成的中间代码与资源
- `build/`：编译输出目录

## 环境要求

请确保本机具备以下环境：

1. Windows 10/11 x64
2. Visual Studio 2022+ 或 Build Tools（包含 MSVC C++ 工具链与 Windows SDK）
3. xmake（建议 3.x）
4. `nuget.exe` 可在 PATH 中找到（仅当 `.xmake/nuget` 缺依赖时必需）
5. Python 3.10+（必需，构建流程由 Python 脚本生成规划与输入文件）

## 部署到新电脑

本项目生成的是未打包 WinUI 3 桌面应用，并使用 Windows App SDK Bootstrap 启动。Release 产物使用 MSVC 动态运行库，因此目标电脑需要满足以下条件之一：

1. 推荐：预先安装最新版 Microsoft Visual C++ Redistributable（x64），安装程序可使用 `vc_redist.x64.exe /install /quiet /norestart` 静默安装。
2. 可选：直接分发完整的 Release 输出目录。本项目的 `winui3.app` 规则会复制 `Microsoft.WindowsAppRuntime.Bootstrap.dll` 与 `resources.pri`，`demo.msvc_runtime` 规则会在 Release 构建后复制可再分发的 MSVC CRT DLL 到可执行文件同目录，用于应用本地部署。

不要将 Debug 输出目录部署到新电脑。Debug 模式使用 `/MDd`，依赖不可再分发的调试版 MSVC 运行库。

如果目标电脑启动后无窗口，请先检查事件查看器中的 `Windows Logs/Application`，并确认已经安装 Windows App Runtime 与 Microsoft Visual C++ Redistributable，或确认发布目录内包含 `vcruntime140*.dll`、`msvcp140*.dll` 等运行库文件。

## 自动化构建流程（由规则完成）

`winui3.app` 规则会在构建期自动执行以下步骤：

1. NuGet 还原（按 `packages.config`）。
2. 生成基础投影（platform + WebView2 + AppSDK）。
3. 通过 MIDL 生成组件 WinMD，并用 mdmerge 合并。
4. 基于合并后的 WinMD 生成组件投影代码。
5. 生成 XAML/makepri 输入（Python 脚本）。
6. 调用 XamlCompiler 两阶段编译 XAML。
7. 调用 makepri 生成 `resources.pri`，并复制到输出目录。

## 迁移后架构（Lua + Python）

- Lua 负责接入 xmake 生命周期与命令执行（`on_load` / `before_buildcmd` / `after_buildcmd`）。
- Python 负责路径与参数规划、上下文聚合、以及 XAML/makepri 输入文件生成。
- 上下文由单个聚合脚本 `xmake/scripts/resolve_context_plan.py` 统一产出（`bootstrap`/`target` 两种模式）。
- `before_build` 先执行 restore（context 驱动），随后通过 `resolve_before_build_plan.py` 一次性产出 cppwinrt/winmd/xaml 计划。
- 各步骤计划脚本与 `gen_xaml_inputs.py` 统一读取 `context.*.json`，Lua 侧仅传递 `--context` 和 `--output`。
- `on_load` 阶段直接消费聚合上下文中的 `sysincludedirs`、`linkdirs`、`links`，不再单独调用 on_load 规划脚本。
- XAML 输入生成通过 `gen_xaml_inputs.py` 的 CLI 参数模式完成，不再依赖 Lua 手工 JSON 转义和 PowerShell 中转写文件。

## 命名空间修改指南

项目生成链使用的根命名空间在 `xmake/modules/winui3/context.lua` 顶部常量 `ROOT_NAMESPACE` 控制。

修改命名空间时，建议同步检查以下文件中的手写命名空间和 `x:Class`：

- `src/MainWindow.idl`
- `src/XamlMetaDataProvider.idl`
- `src/App.xaml`
- `src/MainWindow.xaml`
- `src/*.xaml.h`
- `src/*.xaml.cpp`
- `src/main.cpp`
