#!/usr/bin/env python3
"""Run the WinUI 3 C++/WinRT code generation pipeline used by xmake."""

from __future__ import annotations

import argparse
import json
import os
import shutil
import subprocess
import sys
import xml.etree.ElementTree as ET
from pathlib import Path
from typing import Any


ROOT_NAMESPACE = "xmake_demo"
FEATURE_CONTROL_FLAGS = (
    "EnableXBindDiagnostics;"
    "EnableDefaultValidationContextGeneration;"
    "EnableWin32Codegen"
)

# ── 输出控制 ───────────────────────────────────────────────────────
# 模块级详细输出开关：通过 --verbose / -v 参数设置
_verbose: bool = False

def _discover_winsdk_root() -> Path:
    """Discover Windows SDK root from environment or registry."""
    env = os.environ.get("WindowsSdkDir")
    if env:
        return Path(env)
    try:
        import winreg
        key = winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE,
                            r"SOFTWARE\Microsoft\Windows Kits\Installed Roots")
        value, _ = winreg.QueryValueEx(key, "KitsRoot10")
        winreg.CloseKey(key)
        return Path(value)
    except OSError:
        pass
    return Path(r"C:\Program Files (x86)\Windows Kits\10")

def _discover_winsdk_version(sdk_root: Path) -> str:
    """Discover highest available UAP SDK version."""
    uap_dir = sdk_root / "Platforms" / "UAP"
    if not uap_dir.is_dir():
        return "10.0.26100.0"
    versions = sorted(d.name for d in uap_dir.iterdir() if d.is_dir())
    return versions[-1] if versions else "10.0.26100.0"

def _discover_vc_bin() -> Path:
    """Discover VC toolchain binary directory."""
    vctools = os.environ.get("VCToolsInstallDir")
    if vctools:
        return Path(vctools) / "bin" / "HostX64" / "x64"
    # Try common locations
    candidates = [
        Path(r"C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC"),
        Path(r"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC"),
        Path(r"C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC"),
        Path(r"C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Tools\MSVC"),
    ]
    for base in candidates:
        if base.is_dir():
            versions = sorted(d.name for d in base.iterdir() if d.is_dir())
            if versions:
                return base / versions[-1] / "bin" / "HostX64" / "x64"
    return Path(r"C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\bin\HostX64\x64")

WINDOWS_SDK_ROOT = _discover_winsdk_root()
WINDOWS_SDK_VERSION = _discover_winsdk_version(WINDOWS_SDK_ROOT)
BUILD_TOOLS_BIN_VERSION = "10.0.28000.0"
DEFAULT_VC_BIN = _discover_vc_bin()
# WinAppSDK 1.8+ splits WinMDs across Foundation, WinUI, and InteractiveExperiences sub-packages
APP_SDK_WINMDS_FOUNDATION = [
    "Microsoft.Windows.ApplicationModel.DynamicDependency",
    "Microsoft.Windows.ApplicationModel.Resources",
    "Microsoft.Windows.ApplicationModel.WindowsAppRuntime",
    "Microsoft.Windows.AppLifecycle",
    "Microsoft.Windows.AppNotifications",
    "Microsoft.Windows.AppNotifications.Builder",
    "Microsoft.Windows.Foundation",
    "Microsoft.Windows.Globalization",
    "Microsoft.Windows.Management.Deployment",
    "Microsoft.Windows.PushNotifications",
    "Microsoft.Windows.Security.AccessControl",
    "Microsoft.Windows.System",
    "Microsoft.Windows.System.Power",
]
APP_SDK_WINMDS_WINUI = [
    "Microsoft.UI.Text",
    "Microsoft.UI.Xaml",
]
APP_SDK_WINMDS_IXP = [
    "Microsoft.Foundation",
    "Microsoft.Graphics",
    "Microsoft.UI",
]


# ── 文件发现基础设施 ───────────────────────────────────────────────
# 以下函数用于自动发现 src/ 中的源文件，替代硬编码的文件列表。
# 支持任意数量的 .xaml、.idl、.winmd 文件，无需修改构建脚本。

# 按约定：应用程序入口 XAML 文件命名为 App.xaml
_APP_XAML_NAME: str = "App.xaml"


def discover_xaml_files(src_dir: Path) -> list[Path]:
    """递归发现 src_dir 及其子目录中所有 .xaml 文件，按路径排序。"""
    return sorted(src_dir.rglob("*.xaml"), key=lambda p: str(p))


def discover_idl_files(src_dir: Path) -> list[Path]:
    """递归发现 src_dir 及其子目录中所有 .idl 文件，按路径排序。"""
    return sorted(src_dir.rglob("*.idl"), key=lambda p: str(p))


def discover_winmd_files(winmd_dir: Path) -> list[Path]:
    """递归发现 winmd_dir 及其子目录中所有 .winmd 文件。若目录不存在则返回空列表。"""
    if not winmd_dir.is_dir():
        return []
    return sorted(winmd_dir.rglob("*.winmd"), key=lambda p: str(p))


def xaml_to_header(xaml_path: Path) -> Path:
    """由 .xaml 路径推导对应的 .h 头文件路径（例如 App.xaml → App.xaml.h）。"""
    return Path(os.fspath(xaml_path) + ".h")


def classify_xaml(xaml_files: list[Path], src_dir: Path) -> tuple[list[Path], list[Path]]:
    """将 XAML 文件分为 (应用程序入口, 页面) 两组。

    仅 src_dir 根目录下的 App.xaml 被视为应用程序入口；
    子目录中的 App.xaml 按普通页面处理。
    """
    app_candidate = src_dir / _APP_XAML_NAME
    apps = [p for p in xaml_files if p == app_candidate]
    pages = [p for p in xaml_files if p != app_candidate]
    return apps, pages


def clean_stale_files(directory: Path, pattern: str, *, required: bool = False) -> None:
    """清理目录中匹配 pattern 的陈旧生成文件，确保增量构建不会使用旧产物。

    Args:
        directory: 目标目录。
        pattern: glob 匹配模式（例如 "*.winmd"、"**/*.xbf"）。
        required: 若为 True，清理失败时将抛出 BuildError。
    """
    if not directory.is_dir():
        return
    for stale in directory.glob(pattern):
        try:
            stale.unlink()
        except OSError as exc:
            msg = f"failed to remove stale file {native_path(stale)}: {exc}"
            if required:
                raise BuildError(msg) from exc
            print(f"mwarning: {msg}")


def idl_to_winmd_path(idl_path: Path, base_dir: Path, out_dir: Path) -> Path:
    """由 IDL 路径生成 WinMD 输出路径，保留相对于 base_dir 的目录结构以避免命名冲突。

    若 IDL 不在 base_dir 下（例如自动生成的 IDL），则直接放入 out_dir 根目录。

    例如:
        src/MainWindow.idl       -> winmd_unmerged/MainWindow.winmd
        src/Views/Foo.idl        -> winmd_unmerged/Views/Foo.winmd
        generated/XamlMetaDataProvider.idl -> winmd_unmerged/XamlMetaDataProvider.winmd
    """
    try:
        rel = idl_path.relative_to(base_dir)
        return out_dir / rel.parent / f"{idl_path.stem}.winmd"
    except ValueError:
        # IDL 不在源码目录下（例如自动生成的），直接放入 out_dir 根目录
        return out_dir / f"{idl_path.stem}.winmd"


class BuildError(RuntimeError):
    """Raised for user-facing build failures."""


def native_path(path: str | Path) -> str:
    return os.path.normpath(os.fspath(path)).replace("/", "\\")


def absolute_path(path: Path) -> Path:
    return path.expanduser().resolve(strict=False)


def parse_version(version: str) -> tuple[int, ...]:
    parts: list[int] = []
    for part in version.split("."):
        try:
            parts.append(int(part))
        except ValueError:
            parts.append(-1)
    return tuple(parts)


def require_file(path: Path, label: str) -> Path:
    if not path.is_file():
        raise BuildError(f"{label} does not exist: {native_path(path)}")
    return path


def require_dir(path: Path, label: str) -> Path:
    if not path.is_dir():
        raise BuildError(f"{label} does not exist: {native_path(path)}")
    return path


def print_phase(title: str) -> None:
    print(f"\n=== {title} ===", flush=True)


def _filter_subprocess_output(stdout: str, stderr: str) -> str | None:
    """过滤冗长的子进程输出，仅保留具有诊断价值的行。

    保留：
        - 包含 error/warning/fail 关键词的行
        - 包含文件路径和行列号的行（例如 MainWindow.xaml(12,5)）
        - 具有上下文价值的非空行

    剥离：
        - JSON 片段（以 {、} 开头或纯键值对形式的行）
        - 堆栈跟踪行（以 "   at " 开头或以 "---" 分隔）
        - ReferenceAssembly/XamlPage/XamlApplication/ClIncludeFile 等元数据行
        - 纯空白行
    """
    combined = (stderr or "") + "\n" + (stdout or "")
    relevant: list[str] = []

    for raw_line in combined.splitlines():
        stripped = raw_line.strip()
        if not stripped:
            continue

        # 跳过堆栈跟踪行
        if stripped.startswith("at ") or stripped.startswith("---"):
            continue

        # 跳过纯 JSON 结构行（以 {、} 开头 或 纯键值对形式）
        first_char = stripped[0]
        if first_char in ("{", "}"):
            continue
        if first_char == '"' and '":' in stripped[:60]:
            continue

        # 跳过 XamlCompiler 的元数据回显行
        if stripped.startswith(("ReferenceAssembly:", "ReferenceAssemblyPath:",
                                "XamlPage:", "XamlApplication:", "ClIncludeFile:")):
            continue

        # 跳过冗长的路径输出（以特定前缀开头的非诊断行）
        if stripped.startswith("SavedStateFile:") or stripped.startswith("OutputPath:"):
            continue

        relevant.append(raw_line)

    # 限制输出行数，避免淹没终端
    max_lines = 80
    if len(relevant) > max_lines:
        relevant = relevant[:max_lines]
        relevant.append(
            f"... (输出已截断，显示前 {max_lines} 行。使用 --verbose 查看完整输出)"
        )

    return "\n".join(relevant) if relevant else None


def run_command(
    command: list[str],
    env: dict[str, str] | None = None,
) -> subprocess.CompletedProcess[str]:
    """运行子进程命令，捕获并智能过滤输出。

    成功时：显示命令名和简要 [OK] 状态，隐藏详细工具输出。
    失败时：过滤冗余行，仅显示关键诊断信息，随后抛出 CalledProcessError。
    --verbose 模式下恢复完整的未过滤输出。

    Args:
        command: 命令及参数列表。
        env: 可选的环境变量字典。

    Returns:
        CompletedProcess 实例（含 stdout/stderr 属性，供调用方使用）。

    Raises:
        subprocess.CalledProcessError: 命令以非零退出码退出时。
    """
    cmdline = subprocess.list2cmdline(command)

    # 捕获子进程输出阶段
    result = subprocess.run(
        command,
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
        env=env,
    )

    # 判断成功/失败并选择输出策略
    if result.returncode == 0:
        # 成功：压缩为一行简洁提示
        print(f"  [OK] {cmdline[:80]}{'...' if len(cmdline) > 80 else ''}", flush=True)
        return result

    # 失败：根据模式输出诊断信息
    if _verbose:
        # 详细模式：完整输出
        if result.stdout and result.stdout.strip():
            print(result.stdout, flush=True)
        if result.stderr and result.stderr.strip():
            print(result.stderr, file=sys.stderr, flush=True)
    else:
        # 紧凑模式：过滤后输出
        filtered = _filter_subprocess_output(result.stdout, result.stderr)
        if filtered:
            print(filtered, flush=True)

    print(f"error: command failed with exit code {result.returncode}:", file=sys.stderr)
    print(cmdline, file=sys.stderr)
    raise subprocess.CalledProcessError(
        result.returncode, command, output=result.stdout, stderr=result.stderr
    )


def discover_winsdk_version(winsdk_root: Path) -> str:
    uap_dir = require_dir(winsdk_root / "Platforms" / "UAP", "Windows SDK UAP platform directory")
    versions = [path.name for path in uap_dir.iterdir() if path.is_dir()]
    if not versions:
        raise BuildError(f"no Windows SDK UAP versions found under: {native_path(uap_dir)}")
    return max(versions, key=parse_version)


def platform_xml_path(winsdk_root: Path, requested_version: str) -> tuple[str, Path]:
    platform_xml = winsdk_root / "Platforms" / "UAP" / requested_version / "Platform.xml"
    if platform_xml.is_file():
        return requested_version, platform_xml

    detected_version = discover_winsdk_version(winsdk_root)
    detected_xml = winsdk_root / "Platforms" / "UAP" / detected_version / "Platform.xml"
    return detected_version, require_file(detected_xml, "Windows SDK Platform.xml")


def parse_platform_contracts(platform_xml: Path) -> list[tuple[str, str]]:
    try:
        root = ET.parse(platform_xml).getroot()
    except ET.ParseError as exc:
        raise BuildError(f"failed to parse {native_path(platform_xml)}: {exc}") from exc
    except OSError as exc:
        raise BuildError(f"failed to read {native_path(platform_xml)}: {exc}") from exc

    contracts: list[tuple[str, str]] = []
    for element in root.iter():
        if element.tag.rsplit("}", 1)[-1] != "ApiContract":
            continue

        name = element.attrib.get("name")
        version = element.attrib.get("version")
        if not name or not version:
            raise BuildError(f"ApiContract in {native_path(platform_xml)} is missing name or version")
        contracts.append((name, version))

    if not contracts:
        raise BuildError(f"no ApiContract entries found in {native_path(platform_xml)}")
    return contracts


def collect_platform_winmds(winsdk_root: Path, winsdk_version: str) -> list[Path]:
    _, platform_xml = platform_xml_path(winsdk_root, winsdk_version)
    contracts = parse_platform_contracts(platform_xml)
    winmds: list[Path] = []

    for contract_name, contract_version in contracts:
        winmd = (
            winsdk_root
            / "References"
            / winsdk_version
            / contract_name
            / contract_version
            / f"{contract_name}.winmd"
        )
        winmds.append(require_file(winmd, f"platform WinMD {contract_name}"))

    return winmds


def collect_appsdk_winmds(
    foundation_pkg: Path,
    winui_pkg: Path,
    ixp_pkg: Path,
) -> list[Path]:
    """Collect WinMD files from WinAppSDK 1.8+ sub-package metadata directories."""
    winmds: list[Path] = []

    # Foundation metadata (root-level)
    foundation_meta = require_dir(foundation_pkg / "metadata", "Foundation metadata directory")
    for name in APP_SDK_WINMDS_FOUNDATION:
        candidate = foundation_meta / f"{name}.winmd"
        winmds.append(require_file(candidate, f"Foundation WinMD {name}"))

    # WinUI metadata (root-level)
    winui_meta = require_dir(winui_pkg / "metadata", "WinUI metadata directory")
    for name in APP_SDK_WINMDS_WINUI:
        candidate = winui_meta / f"{name}.winmd"
        winmds.append(require_file(candidate, f"WinUI WinMD {name}"))

    # InteractiveExperiences metadata (versioned sub-directory)
    ixp_meta_root = require_dir(ixp_pkg / "metadata", "InteractiveExperiences metadata directory")
    ixp_versions = sorted(d.name for d in ixp_meta_root.iterdir() if d.is_dir())
    if not ixp_versions:
        raise BuildError(f"no version sub-directories in: {native_path(ixp_meta_root)}")
    ixp_meta = require_dir(ixp_meta_root / ixp_versions[-1], "InteractiveExperiences version metadata")
    for name in APP_SDK_WINMDS_IXP:
        candidate = ixp_meta / f"{name}.winmd"
        winmds.append(require_file(candidate, f"InteractiveExperiences WinMD {name}"))

    return winmds


def find_foundation_metadata_dir(winsdk_root: Path, winsdk_version: str) -> Path:
    foundation_root = require_dir(
        winsdk_root / "References" / winsdk_version / "Windows.Foundation.FoundationContract",
        "Windows.Foundation.FoundationContract metadata directory",
    )
    versions = [path for path in foundation_root.iterdir() if path.is_dir()]
    if not versions:
        raise BuildError(f"no FoundationContract versions found under: {native_path(foundation_root)}")
    return max(versions, key=lambda path: parse_version(path.name))


def unique_parent_dirs(paths: list[Path]) -> list[Path]:
    seen: set[str] = set()
    result: list[Path] = []

    for path in paths:
        parent = path.parent
        key = os.path.normcase(native_path(parent))
        if key in seen:
            continue
        seen.add(key)
        result.append(parent)

    return result


def nuget_root() -> Path:
    user_profile = os.environ.get("USERPROFILE")
    if user_profile:
        return Path(user_profile) / ".nuget" / "packages"
    return Path(r"C:\Users\WanYa\.nuget\packages")


def vc_bin_path() -> Path:
    tools_dir = os.environ.get("VCToolsInstallDir")
    if tools_dir:
        return Path(tools_dir) / "bin" / "HostX64" / "x64"
    return DEFAULT_VC_BIN


def path_env_with_vc(vc_bin: Path) -> dict[str, str]:
    env = os.environ.copy()
    env["PATH"] = f"{native_path(vc_bin)};{env.get('PATH', '')}"

    if shutil.which("cl.exe", path=env["PATH"]) is None:
        raise BuildError(
            f"cl.exe was not found after adding VC tools directory to PATH: {native_path(vc_bin)}"
        )

    return env


def msbuild_item(path: Path, dependent_upon: Path | None = None) -> dict[str, str]:
    native = native_path(absolute_path(path))
    item = {"ItemSpec": native, "FullPath": native}
    if dependent_upon is not None:
        item["DependentUpon"] = native_path(absolute_path(dependent_upon))
    return item


def build_xaml_json(
    *,
    generated_dir: Path,
    project_dir: Path,
    winsdk_version: str,
    ref_winmds: list[Path],
    merged_winmd: Path,
    genxbf_path: Path,
    is_pass1: bool,
) -> dict[str, Any]:
    src_dir = project_dir / "src"

    # 自动发现所有 .xaml 文件，按命名约定分类
    xaml_files = discover_xaml_files(src_dir)
    if not xaml_files:
        raise BuildError(f"no .xaml files found in {native_path(src_dir)}")
    xaml_apps, xaml_pages = classify_xaml(xaml_files, src_dir)

    # 由 .xaml 文件推导对应的 .h 头文件列表
    header_files: list[tuple[Path, Path]] = []
    for xf in xaml_files:
        hf = xaml_to_header(xf)
        if hf.is_file():
            header_files.append((hf, xf))
        else:
            print(f" mwarning: no header file found for {xf.name} (expected {hf.name}), skipping ClIncludeFiles entry")

    data: dict[str, Any] = {
        "SavedStateFile": native_path(generated_dir / "XamlCompilerState.xml"),
        "IsPass1": is_pass1,
        "Language": "CppWinRT",
        "ProjectPath": native_path(absolute_path(xaml_apps[0] if xaml_apps else xaml_files[0])),
        "LanguageSourceExtension": ".cpp",
        "OutputPath": native_path(generated_dir),
        "RootNamespace": ROOT_NAMESPACE,
        "PrecompiledHeaderFile": "pch.h",
        "FeatureControlFlags": FEATURE_CONTROL_FLAGS,
        "ReferenceAssemblies": [msbuild_item(path) for path in ref_winmds],
        "ReferenceAssemblyPaths": [],
        "TargetPlatformMinVersion": winsdk_version,
        "XamlPages": [msbuild_item(p) for p in xaml_pages],
        "XamlApplications": [msbuild_item(p) for p in xaml_apps],
        "ClIncludeFiles": [
            msbuild_item(hf, dependent_upon=xf) for hf, xf in header_files
        ],
    }

    if not is_pass1:
        data["LocalAssembly"] = [msbuild_item(merged_winmd)]
        data["GenXbfPath"] = native_path(genxbf_path)

    return data


def write_text(path: Path, content: str) -> None:
    try:
        path.write_text(content, encoding="utf-8", newline="\n")
    except OSError as exc:
        raise BuildError(f"failed to write {native_path(path)}: {exc}") from exc


def write_json(path: Path, data: dict[str, Any]) -> None:
    write_text(path, json.dumps(data, indent=2) + "\n")


def generate_xaml_metadata_provider(generated_dir: Path) -> None:
    write_text(
        generated_dir / "XamlMetaDataProvider.idl",
        "namespace xmake_demo\n"
        "{\n"
        "    runtimeclass XamlMetaDataProvider : [default] Microsoft.UI.Xaml.Markup.IXamlMetadataProvider\n"
        "    {\n"
        "        XamlMetaDataProvider();\n"
        "    }\n"
        "}\n",
    )
    write_text(
        generated_dir / "XamlMetaDataProvider.cpp",
        '#include "pch.h"\n'
        '#include "XamlMetaDataProvider.h"\n'
        '#include "XamlMetaDataProvider.g.cpp"\n',
    )


def run_midl(
    *,
    midl_exe: Path,
    idl: Path,
    out_winmd: Path,
    foundation_meta: Path,
    sdk_include_dir: Path,
    ref_winmds: list[Path],
    env: dict[str, str],
) -> None:
    command = [
        native_path(midl_exe),
        native_path(idl),
        "/nologo",
        "/winrt",
        "/winmd",
        native_path(out_winmd),
        "/nomidl",
        "/h",
        "nul",
        "/metadata_dir",
        native_path(foundation_meta),
        "/I",
        native_path(sdk_include_dir / "um"),
        "/I",
        native_path(sdk_include_dir / "shared"),
        "/I",
        native_path(sdk_include_dir / "winrt"),
        # 支持 IDL 相对 import（例如 import "Common.idl"）
        "/I",
        native_path(idl.parent),
    ]
    for ref_winmd in ref_winmds:
        command.extend(["/reference", native_path(ref_winmd)])
    run_command(command, env=env)


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Build WinUI 3 generated C++/WinRT and XAML artifacts.")
    parser.add_argument("--build-dir", type=Path, required=True, help="Build output directory.")
    parser.add_argument(
        "--project-dir",
        type=Path,
        default=Path(__file__).resolve().parent.parent,
        help="Project root directory. Defaults to this script's parent directory.",
    )
    parser.add_argument(
        "-v", "--verbose",
        action="store_true",
        default=False,
        help="Enable verbose output: show unfiltered tool output for debugging.",
    )
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    global _verbose
    args = parse_args(sys.argv[1:] if argv is None else argv)
    _verbose = args.verbose

    try:
        project_dir = absolute_path(args.project_dir)
        build_dir = absolute_path(args.build_dir)
        generated_dir = build_dir / "generated"
        generated_sources_dir = generated_dir / "sources"
        winmd_unmerged_dir = build_dir / "winmd_unmerged"
        winmd_merged_dir = build_dir / "winmd_merged"
        merged_winmd = winmd_merged_dir / f"{ROOT_NAMESPACE}.winmd"

        root = nuget_root()
        # WinAppSDK 1.8+ sub-packages (latest stable, April 2026)
        foundation_pkg = root / "microsoft.windowsappsdk.foundation" / "1.8.260415000"
        winui_pkg = root / "microsoft.windowsappsdk.winui" / "1.8.260415005"
        ixp_pkg = root / "microsoft.windowsappsdk.interactiveexperiences" / "1.8.260415001"
        cppwinrt_exe = root / "microsoft.windows.cppwinrt" / "2.0.250303.1" / "bin" / "cppwinrt.exe"
        buildtools_bin = (
            root
            / "microsoft.windows.sdk.buildtools"
            / "10.0.28000.1721"
            / "bin"
            / BUILD_TOOLS_BIN_VERSION
            / "x64"
        )
        midl_exe = buildtools_bin / "midl.exe"
        mdmerge_exe = buildtools_bin / "mdmerge.exe"
        xaml_compiler = winui_pkg / "tools" / "net472" / "XamlCompiler.exe"
        genxbf_dir = winui_pkg / "tools"  # XamlCompiler finds GenXbf.dll in arch subdirectories
        webview2_winmd = (
            root / "microsoft.web.webview2" / "1.0.3912.50" / "lib" / "Microsoft.Web.WebView2.Core.winmd"
        )

        src_dir = require_dir(project_dir / "src", "project src directory")

        # 动态发现用户编写的源文件，替代硬编码文件列表
        xaml_files = discover_xaml_files(src_dir)
        if not xaml_files:
            raise BuildError(f"no .xaml files found in {native_path(src_dir)}")

        idl_files = discover_idl_files(src_dir)

        require_file(cppwinrt_exe, "cppwinrt.exe")
        require_file(midl_exe, "midl.exe")
        require_file(mdmerge_exe, "mdmerge.exe")
        require_file(xaml_compiler, "XamlCompiler.exe")
        require_file(webview2_winmd, "WebView2 WinMD")
        require_dir(foundation_pkg, "Foundation sub-package")
        require_dir(winui_pkg, "WinUI sub-package")
        require_dir(ixp_pkg, "InteractiveExperiences sub-package")
        require_dir(genxbf_dir, "GenXbf directory")
        require_dir(vc_bin_path(), "VC tools bin directory")

        winsdk_root = require_dir(WINDOWS_SDK_ROOT, "Windows SDK root")
        winsdk_version, _ = platform_xml_path(winsdk_root, WINDOWS_SDK_VERSION)
        platform_winmds = collect_platform_winmds(winsdk_root, winsdk_version)
        appsdk_winmds = collect_appsdk_winmds(foundation_pkg, winui_pkg, ixp_pkg)
        ref_winmds = [*platform_winmds, webview2_winmd, *appsdk_winmds]
        foundation_meta = find_foundation_metadata_dir(winsdk_root, winsdk_version)
        sdk_include_dir = require_dir(
            winsdk_root / "Include" / winsdk_version,
            "Windows SDK include directory",
        )
        for include_name in ["um", "shared", "winrt"]:
            require_dir(sdk_include_dir / include_name, f"Windows SDK {include_name} include directory")

        for directory in [generated_dir, generated_sources_dir, winmd_unmerged_dir, winmd_merged_dir]:
            directory.mkdir(parents=True, exist_ok=True)

        print(f"Project: {native_path(project_dir)}")
        print(f"Build:   {native_path(build_dir)}")
        print(f"WinSDK:  {native_path(winsdk_root)} ({winsdk_version})")
        print(f"Refs:    {len(platform_winmds)} platform, {len(appsdk_winmds)} WinAppSDK, 1 WebView2")

        print_phase("[1/8] Generate platform, WebView2, and WinAppSDK headers")
        phase1a = [native_path(cppwinrt_exe)]
        for winmd in platform_winmds:
            phase1a.extend(["-in", native_path(winmd)])
        phase1a.extend(["-out", native_path(generated_dir)])
        run_command(phase1a)

        phase1b = [native_path(cppwinrt_exe), "-in", native_path(webview2_winmd)]
        for winmd in platform_winmds:
            phase1b.extend(["-ref", native_path(winmd)])
        phase1b.extend(["-out", native_path(generated_dir)])
        run_command(phase1b)

        phase1c = [native_path(cppwinrt_exe)]
        for winmd in appsdk_winmds:
            phase1c.extend(["-in", native_path(winmd)])
        for winmd in platform_winmds:
            phase1c.extend(["-ref", native_path(winmd)])
        phase1c.extend(["-ref", native_path(webview2_winmd), "-out", native_path(generated_dir)])
        run_command(phase1c)

        print_phase("[2/8] Compile IDL to WinMD")

        # 清理上次构建的陈旧 WinMD 文件（递归，因 IDL 可能位于子目录）
        clean_stale_files(winmd_unmerged_dir, "**/*.winmd", required=True)

        midl_env = path_env_with_vc(vc_bin_path())

        if not idl_files:
            # 零用户 IDL：自动生成最小 XamlMetaDataProvider 以供构建
            print("  (no user IDL files found, auto-generating XamlMetaDataProvider.idl)")
            generate_xaml_metadata_provider(generated_dir)
            idl_files = [generated_dir / "XamlMetaDataProvider.idl"]

        for idl_path in idl_files:
            # 保留相对于 src_dir 的目录结构生成 winmd，避免子目录同名 IDL 冲突
            out_winmd = idl_to_winmd_path(idl_path, src_dir, winmd_unmerged_dir)
            # 确保输出子目录存在
            out_winmd.parent.mkdir(parents=True, exist_ok=True)
            run_midl(
                midl_exe=midl_exe,
                idl=idl_path,
                out_winmd=out_winmd,
                foundation_meta=foundation_meta,
                sdk_include_dir=sdk_include_dir,
                ref_winmds=ref_winmds,
                env=midl_env,
            )

        print_phase("[3/8] Merge WinMDs")
        unmerged_winmds = discover_winmd_files(winmd_unmerged_dir)
        if not unmerged_winmds:
            raise BuildError(
                f"no .winmd files found in {native_path(winmd_unmerged_dir)} — MIDL compilation failed"
            )
        mdmerge = [native_path(mdmerge_exe), "-o", native_path(winmd_merged_dir)]
        for directory in unique_parent_dirs(ref_winmds):
            mdmerge.extend(["-metadata_dir", native_path(directory)])
        for wm in unmerged_winmds:
            mdmerge.extend(["-i", native_path(wm)])
        mdmerge.extend(["-partial", "-n:1"])
        run_command(mdmerge)
        require_file(merged_winmd, "merged WinMD")

        print_phase("[4/8] Generate project C++/WinRT sources")
        cppwinrt_project = [
            native_path(cppwinrt_exe),
            "-in",
            native_path(merged_winmd),
            "-out",
            native_path(generated_dir),
            "-comp",
            native_path(generated_sources_dir),
            "-name",
            ROOT_NAMESPACE,
            "-pch",
            "pch.h",
            "-prefix",
            "-optimize",
            "-overwrite",
        ]
        for winmd in ref_winmds:
            cppwinrt_project.extend(["-ref", native_path(winmd)])
        run_command(cppwinrt_project)

        print_phase("[5/8] Generate XamlMetaDataProvider source")
        generate_xaml_metadata_provider(generated_dir)

        print_phase("[6/8] Run XAML compiler pass 1")

        # 清理上次构建的陈旧 .xbf 文件，防止已删除 XAML 的残留 XBF 被 makepri 打包
        clean_stale_files(generated_dir, "**/*.xbf")

        pass1_json = generated_dir / "xaml.pass1.in.json"
        pass1_out = generated_dir / "xaml.pass1.out.json"
        write_json(
            pass1_json,
            build_xaml_json(
                generated_dir=generated_dir,
                project_dir=project_dir,
                winsdk_version=winsdk_version,
                ref_winmds=ref_winmds,
                merged_winmd=merged_winmd,
                genxbf_path=genxbf_dir,
                is_pass1=True,
            ),
        )
        run_command([native_path(xaml_compiler), native_path(pass1_json), native_path(pass1_out)])

        print_phase("[7/8] Run XAML compiler pass 2")
        pass2_json = generated_dir / "xaml.pass2.in.json"
        pass2_out = generated_dir / "xaml.pass2.out.json"
        write_json(
            pass2_json,
            build_xaml_json(
                generated_dir=generated_dir,
                project_dir=project_dir,
                winsdk_version=winsdk_version,
                ref_winmds=ref_winmds,
                merged_winmd=merged_winmd,
                genxbf_path=genxbf_dir,
                is_pass1=False,
            ),
        )
        run_command([native_path(xaml_compiler), native_path(pass2_json), native_path(pass2_out)])

        # Phase 8: Generate .pri resource file (makepri)
        print_phase("[8/8] Generate .pri resource index")
        makepri_exe = buildtools_bin / "makepri.exe"
        if not makepri_exe.is_file():
            raise BuildError(f"makepri.exe not found at {makepri_exe}")
        
        # Collect .xbf files generated by XamlCompiler (recursive to cover subdirectories)
        xbf_files = sorted(generated_dir.rglob("*.xbf"))
        if not xbf_files:
            raise BuildError("No .xbf files found - XAML compilation may have failed")
        
        # Write resfiles list (full paths to .xbf files)
        resfiles_path = generated_dir / "layout.resfiles"
        resfiles_content = "\n".join(str(f) for f in xbf_files)
        resfiles_path.write_text(resfiles_content, encoding="utf-8")
        
        # Write priconfig.xml
        priconfig_path = generated_dir / "priconfig.xml"
        priconfig_xml = f"""<?xml version="1.0" encoding="utf-8"?>
<resources targetOsVersion="10.0.0" majorVersion="1">
  <index root="{generated_dir}" startIndexAt="{resfiles_path}">
    <default>
      <qualifier name="Language" value="en-US"/>
      <qualifier name="Contrast" value="standard"/>
      <qualifier name="Scale" value="200"/>
      <qualifier name="HomeRegion" value="001"/>
      <qualifier name="TargetSize" value="256"/>
      <qualifier name="LayoutDirection" value="LTR"/>
      <qualifier name="DXFeatureLevel" value="DX9"/>
      <qualifier name="Configuration" value=""/>
      <qualifier name="AlternateForm" value=""/>
      <qualifier name="Platform" value="UAP"/>
    </default>
    <indexer-config type="RESFILES" qualifierDelimiter="."/>
    <indexer-config type="EMBEDFILES"/>
  </index>
</resources>"""
        priconfig_path.write_text(priconfig_xml, encoding="utf-8")
        
        pri_output = generated_dir / "resources.pri"
        run_command([
            native_path(makepri_exe), "new",
            "/cf", native_path(priconfig_path),
            "/pr", native_path(project_dir),
            "/o",
            "/of", native_path(pri_output),
        ])

        print("\n=== WinUI 3 code generation complete ===", flush=True)
    except BuildError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1
    except subprocess.CalledProcessError:
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
