# winui3-xmake

`winui3-xmake` 是一个面向 Windows x64 的 WinUI 3 / C++/WinRT 示例工程。项目使用 xmake 与 Python 脚本编排 XAML、IDL、WinMD、C++/WinRT 投影和资源索引生成流程，不依赖 Visual Studio 解决方案文件（`.sln` / `.vcxproj`）。

## 示例目标

| 目标 | 说明 | 源码目录 |
| --- | --- | --- |
| `demo.hello` | 最小 WinUI 3 应用，包含按钮交互与 `DesktopAcrylicBackdrop` | `demo/hello/src/` |
| `demo.notepad` | 简易记事本，包含 `x:Bind` 双向绑定与 ViewModel | `demo/notepad/src/` |
| `demo.gallery` | 控件展示应用，覆盖按钮、选择、文本、媒体、导航和菜单等场景 | `demo/gallery/src/` |
| `demo.camera` | 摄像头预览示例，枚举视频采集设备并通过 `MediaCapture` 与 `MediaPlayerElement` 显示预览 | `demo/camera/src/` |

## 技术栈

- C++20
- xmake（Windows x64）
- WinUI 3 / Windows App SDK 2.0.1
- C++/WinRT 2.0
- WIL
- WebView2 WinMD（用于 C++/WinRT 投影输入）

NuGet 依赖及精确版本见 `packages.config`。构建脚本会从 `%USERPROFILE%\.nuget\packages` 解析包路径；首次构建前请确认依赖已按 NuGet 全局包目录布局安装（小写包名 / 版本号）。

## 环境要求

1. Windows 10/11 x64
2. Visual Studio 2022 或 Build Tools，包含 MSVC C++ 工具链与 Windows SDK
3. xmake
4. Python 3.10+
5. `packages.config` 中的 NuGet 包已存在于 NuGet 全局包目录

> `xmake.lua` 可通过 `winui3.xaml_compiler_path` 指定自定义 XAML Compiler 路径。若在其他机器上使用，请确认该路径有效，或移除该值以改用脚本的包路径探测逻辑。

## 准备、构建与运行

```powershell
# 可选：查看 NuGet 全局包目录；构建脚本默认读取 %USERPROFILE%\.nuget\packages
nuget locals global-packages -list

# 构建指定示例
xmake build demo.hello
xmake build demo.notepad
xmake build demo.gallery
xmake build demo.camera

# 运行示例
xmake run demo.hello

# 切换构建模式
xmake f -m debug
xmake f -m release
xmake f -m dist
```

`dist` 模式用于生成分发产物：该模式启用最快优化、隐藏符号、剥离符号，并使用静态 MSVC 运行库（`MT`）。

## 目录结构

```text
winui3-xmake/
├── common/             共享入口点、预编译头和应用清单
├── demo/               示例应用；每个子目录定义一个 xmake 目标
├── rules/              xmake 自定义规则
├── scripts/            WinUI 3 代码生成与平台探测脚本
├── packages.config     NuGet 依赖清单
├── xmake.lua           根项目配置
└── build/              构建输出目录（未纳入版本控制）
```

关键文件：

- `common/main.cpp`：提供手写 `wWinMain`，初始化 Windows App SDK Bootstrap，并集中注册应用级 `UnhandledException` 处理器。
- `rules/winui3.lua`：定义 `winui3.app` 规则，配置编译、链接、代码生成和运行时文件复制。
- `rules/demo.lua`：定义 `demo.common` 规则，为示例目标挂载共享入口点、预编译头和清单。
- `rules/dist.lua`：定义 `mode.dist` 分发构建模式，配置优化、符号剥离和静态运行库。
- `scripts/build_winui3_common.py`：承载共享上下文、路径/工具解析与公共辅助函数，供各构建入口导入。
- `scripts/build_winui3_shared_projection.py`：共享投影生成入口，从平台 WinMD、WebView2、Windows App SDK 与 Win2D 生成共享 C++/WinRT 投影头（阶段 0）。
- `scripts/build_winui3_pre_xaml.py`：前置生成入口，执行 MIDL 编译、WinMD 合并与 C++/WinRT 源码投影（阶段 2-4）。
- `scripts/build_winui3_xaml_pri.py`：XAML/PRI 生成入口，执行 XAML Pass 1 / Pass 2 编译与资源索引生成（阶段 6-8）。
- `scripts/plat_info.py`：解析 Windows SDK、MSVC 工具链和 NuGet 包路径。

## 构建流水线

`winui3.app` 通过 Python 脚本执行代码生成，Lua/xmake 仅负责任务编排与摘要文件（stamp）驱动的增量控制。

`build_winui3_shared_projection.py` 负责共享投影生成（阶段 0）：从平台 WinMD、WebView2、Windows App SDK 与 Win2D 生成共享 C++/WinRT 投影头。该阶段独立运行，不依赖项目 IDL 或 MIDL 工具。

1. `build_winui3_pre_xaml.py` 负责前置生成（阶段 2-4）：使用 MIDL 编译项目 IDL 并合并 WinMD，然后基于合并后的 WinMD 生成项目 C++/WinRT 源码。
2. `build_winui3_xaml_pri.py` 负责 XAML/PRI 生成（阶段 6-8）：执行 XAML Pass 1 生成 `.xbf`，执行 XAML Pass 2 生成 `.g.hpp` / `.g.cpp`，最后使用 `makepri` 生成 `resources.pri` 并复制到目标输出目录。

各入口共享 `build_winui3_common.py` 中的路径解析、工具函数与数据模型。

> 阶段 5 在当前流水线中不参与执行。

构建完成后，输出目录包含应用程序、生成代码、WinMD 中间产物、`resources.pri` 和 `Microsoft.WindowsAppRuntime.Bootstrap.dll`。

## 部署说明

本项目生成未打包 WinUI 3 桌面应用，并在启动时通过 Windows App SDK Bootstrap 加载匹配的 Windows App Runtime。

部署时请确认：

1. 输出目录包含 `.exe`、`resources.pri` 和 `Microsoft.WindowsAppRuntime.Bootstrap.dll`。
2. 目标机器已安装匹配的 Windows App Runtime；若未安装，Bootstrap 会根据 `MddBootstrapInitialize2` 选项显示安装界面。
3. 目标机器具备匹配的 MSVC 运行库，或使用 `dist` 模式生成静态运行库产物。

```powershell
# 使用静态 MSVC 运行库构建分发产物
xmake f -m dist
xmake build demo.hello
```

若不使用静态 MSVC 运行库，请在目标机器安装 Microsoft Visual C++ Redistributable（x64）。不要分发 Debug 构建产物；Debug 模式依赖不可再分发的调试版 MSVC 运行库。

## 添加新示例

1. 在 `demo/` 下创建目录，并放入 XAML、IDL 与 C++ 源码。
2. 创建 `demo/<name>/xmake.lua`：

```lua
target("demo.<name>")
    add_rules("winui3.app", {namespace = "<namespace>", src_dir = path.join(os.scriptdir(), "src")})
    add_rules("demo.common")
    add_files(path.join(os.scriptdir(), "src", "**.cpp"))
```

> **注意：** `namespace` 与 `src_dir` 是 `winui3.app` 的必需参数，不存在默认值。`add_rules` 必须使用**独立调用**形式（不可写为 `add_rules("winui3.app", {opts}, "demo.common")` 的复合形式）。

若示例包含 WebView 内容，需额外添加 `webview` 规则并指定 `dist_dir`：

```lua
target("demo.<name>")
    add_rules("winui3.app", {namespace = "<namespace>", src_dir = path.join(os.scriptdir(), "src")})
    add_rules("demo.common")
    add_rules("webview", {dist_dir = path.join(os.scriptdir(), "src", "web")})
    add_files(path.join(os.scriptdir(), "src", "**.cpp"))
```

> **注意：** `dist_dir` 是 `webview` 规则的必需参数，指向包含 Web 前端资源（HTML/CSS/JS）的目录。该参数不存在向 `winui3.src_dir` 的回退逻辑，必须显式指定。

3. 在源码中通过 `#include "pch.h"` 引用共享预编译头。

随后可使用 `xmake build demo.<name>` 构建新目标。

## 命名空间约定

每个示例的根命名空间通过 `winui3.app` 规则的 `namespace` 参数指定：

```lua
add_rules("winui3.app", {namespace = "hello", src_dir = path.join(os.scriptdir(), "src")})
```

修改命名空间时，请同步检查 IDL、XAML 的 `x:Class` 和手写 C++ 命名空间；构建规则会将该命名空间传给共享 `wWinMain`，用于实例化对应示例的 `App`。
