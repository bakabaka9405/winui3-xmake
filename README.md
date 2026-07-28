# winui3-xmake

使用 xmake 构建 WinUI 3 / C++/WinRT 应用，不依赖 Visual Studio 解决方案文件（`.sln` / `.vcxproj`）。

## 环境要求

1. Windows 10/11 x64
2. Visual Studio 2022 或 Build Tools，包含 MSVC C++ 工具链与 Windows SDK
3. xmake 3.0.9+

## 构建与运行

```powershell
# 配置（首次使用时 xmake 自动安装 NuGet 依赖，无需手动 nuget restore）
xmake f -m debug -y

# 构建
xmake -y demo.hello

# 运行
xmake run demo.hello
```

### 构建模式

| 模式 | 说明 |
| --- | --- |
| `debug` | 调试构建，保留符号信息 |
| `release` | 优化构建 |
| `dist` | 分发构建：最快优化、剥离符号、静态 MSVC 运行库（`MT`） |

```powershell
xmake f -m dist
xmake -y demo.hello
```

## 创建新目标

1. 在 `demo/` 下创建目录，放入 XAML、IDL 与 C++ 源码。

2. 创建 `demo/<name>/xmake.lua`：

```lua
target("demo.<name>")
    add_rules("winui3.app")
    set_values("winui3.namespace", "<namespace>")
    add_rules("demo.common")
    add_includedirs("src")
    add_files("src/**.cpp", "src/**.idl", "src/**.xaml")
```

3. 普通目标在源码中通过 `#include "pch.h"` 引用共享预编译头。

4. 使用 `xmake -y demo.<name>` 构建。

> **注意：**
> - `namespace` 是 `winui3.app` 的必需参数，通过 `set_values("winui3.namespace", ...)` 声明。
> - `add_files` 需同时包含 `.cpp`、`.idl` 和 `.xaml` 三类文件。

### 模块化 C++/WinRT 投影目标

启用 C++ modules 的目标不直接包含文本投影头，而是由规则自动生成聚合模块，并由 `pch.h` 通过宏导入它。`demo.paint` 是当前示例：

```lua
target("demo.<name>")
    add_rules("winui3.app", "winui3.modules")
    set_values("winui3.namespace", "<namespace>")
    add_rules("demo.common")
    add_includedirs("src")
    add_files("src/**.cpp", "src/**.idl", "src/**.xaml")
```

`winui3.modules` 规则从 `winui3.namespace` 自动派生聚合模块名 `<namespace>.winrt`，启用 C++ modules，依赖 `winui3.shared_projection.modules`，并设置 `WINRT_ENABLE_LEGACY_COM` 和 `WINUI3_IMPORT_MODULE` 宏。普通目标与模块目标的手写 `.cpp` 均以 `#include "pch.h"` 开头；模块目标由 `WINUI3_IMPORT_MODULE` 宏使 `pch.h` 进入模块前导分支并导入 `<namespace>.winrt`。

### 可选规则

**WebView2**——在 WinUI 3 窗口中嵌入 Web 前端内容：

```lua
add_rules("webview", {dist_dir = path.join(os.scriptdir(), "src", "web")})
```

`dist_dir` 为必需参数，指向包含 Web 前端资源（HTML/CSS/JS）的目录。

**Win2D**——使用 `CanvasControl` 进行画布绘图：

```lua
add_rules("win2d")
```

### 命名空间约定

修改命名空间时，请同步检查 IDL、XAML 的 `x:Class` 和手写 C++ 命名空间；构建规则会将该命名空间传给共享 `wWinMain`，用于实例化对应的 `App`。

## 部署

本项目生成未打包 WinUI 3 桌面应用，启动时通过 Windows App SDK Bootstrap 加载 Windows App Runtime。

部署清单：

1. 输出目录包含 `.exe`、`resources.pri` 和 `Microsoft.WindowsAppRuntime.Bootstrap.dll`。
2. 目标机器已安装匹配的 Windows App Runtime；若未安装，Bootstrap 会显示安装界面。
3. 目标机器具备匹配的 MSVC 运行库，或使用 `dist` 模式生成静态链接产物。

> 不要分发 Debug 构建产物——Debug 模式依赖不可再分发的调试版 MSVC 运行库。

## 示例

| 目标 | 说明 |
| --- | --- |
| `demo.hello` | 最小 WinUI 3 应用，包含按钮交互与 `DesktopAcrylicBackdrop` |
| `demo.notepad` | 简易记事本，包含 `x:Bind` 双向绑定与 ViewModel |
| `demo.gallery` | 控件展示，覆盖按钮、选择、文本、媒体、导航和菜单等场景 |
| `demo.camera` | 摄像头预览，枚举视频采集设备并通过 `MediaCapture` 显示预览 |
| `demo.explorer` | 文件资源管理器 |
| `demo.paint` | 画布绘图，基于 Win2D `CanvasControl` 实现自由绘制 |
| `demo.webview` | WebView2 集成，在 WinUI 3 窗口中嵌入 Web 前端内容 |
| `demo.xaml-studio` | XAML 编辑器与实时预览，集成 Monaco 与 `XamlReader.Load` |

## 技术栈

- C++20、xmake（Windows x64）
- WinUI 3 / Windows App SDK 2.0.1
- C++/WinRT 2.0、WIL
- WebView2 1.0（`demo.webview`、`demo.xaml-studio`）
- Win2D 1.4（`demo.paint`）
