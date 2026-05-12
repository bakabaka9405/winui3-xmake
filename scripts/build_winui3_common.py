#!/usr/bin/env python3
"""Shared build context, utility functions, and tool resolution for WinUI 3 C++/WinRT code generation pipeline. Used by build_winui3_pre_xaml.py and build_winui3_xaml_pri.py."""

from __future__ import annotations

import argparse
import json
import os
import shutil
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any

from plat_info import (
    WINDOWS_SDK_ROOT,
    WINDOWS_SDK_VERSION,
    BuildError,
    platform_xml_path,
    require_dir,
    require_file,
    vc_bin_path,
)
from winmd import platform as winmd_platform
from winmd import appsdk as winmd_appsdk
from winmd import webview2 as winmd_webview2
from winmd import win2d as winmd_win2d

from nuget_config import NuGetConfig, BUILD_TOOLS_BIN_VERSION


# ── 数据模式定义 ────────────────────────────────────────────────


@dataclass(slots=True, frozen=True)
class BuildLayout:
    """构建输出目录布局，描述项目中所有生成产物与中间文件的路径组织。

    Attributes:
        project_dir: 项目根目录绝对路径。
        build_dir: 构建自动生成中间文件根目录（来自 target:autogendir()）。
        generated_dir: 生成产物目录（build_dir/generated）。
        generated_sources_dir: 生成 C++ 源码目录（generated_dir/sources）。
        shared_projection_dir: 共享 C++/WinRT 投影头目录。
        winmd_unmerged_dir: 未合并 WinMD 中间产物目录。
        winmd_merged_dir: 合并后 WinMD 产物目录。
        merged_winmd: 合并后 WinMD 文件路径。
        src_dir: 用户源码目录。
    """

    project_dir: Path
    build_dir: Path
    generated_dir: Path
    generated_sources_dir: Path
    shared_projection_dir: Path
    winmd_unmerged_dir: Path
    winmd_merged_dir: Path
    merged_winmd: Path
    src_dir: Path


@dataclass(slots=True, frozen=True)
class ResolvedInputs:
    """构建流水线所需的全部外部工具与输入路径解析结果。

    Attributes:
        cppwinrt_exe: C++/WinRT 编译器可执行文件路径。
        midl_exe: MIDL 编译器可执行文件路径。
        mdmerge_exe: WinMD 合并工具可执行文件路径。
        buildtools_bin: Windows SDK BuildTools 二进制目录。
        winsdk_root: Windows SDK 安装根目录。
        winsdk_version: Windows SDK UAP 版本号。
        platform_winmds: Windows SDK 平台 API 契约 WinMD 文件列表。
        appsdk_winmds: Windows App SDK 子包 WinMD 文件列表。
        webview2_winmd: WebView2 Core WinMD 文件路径。
        win2d_winmds: Win2D Canvas WinMD 文件列表。
        ref_winmds: 所有引用 WinMD 的聚合列表（platform + webview2 + appsdk + win2d）。
        foundation_meta: Windows.Foundation.FoundationContract 元数据目录。
        sdk_include_dir: Windows SDK 头文件包含目录。
        xaml_compiler: XamlCompiler.exe 可执行文件路径。
        genxbf_dir: GenXbf 原生 DLL 目录。
    """

    cppwinrt_exe: Path
    midl_exe: Path
    mdmerge_exe: Path
    buildtools_bin: Path
    winsdk_root: Path
    winsdk_version: str
    platform_winmds: list[Path]
    appsdk_winmds: list[Path]
    webview2_winmd: Path
    win2d_winmds: list[Path]
    ref_winmds: list[Path]
    foundation_meta: Path
    sdk_include_dir: Path
    xaml_compiler: Path
    genxbf_dir: Path


@dataclass(slots=True, frozen=True)
class SharedProjectionInputs:
    """共享 C++/WinRT 投影头生成所需的输入路径（不含 XAML/MIDL/MSVC 工具链）。

    Attributes:
        cppwinrt_exe: C++/WinRT 编译器可执行文件路径。
        platform_winmds: Windows SDK 平台 API 契约 WinMD 文件列表。
        webview2_winmd: WebView2 Core WinMD 文件路径。
        appsdk_winmds: Windows App SDK 子包 WinMD 文件列表。
        win2d_winmds: Win2D Canvas WinMD 文件列表。
    """

    cppwinrt_exe: Path
    platform_winmds: list[Path]
    webview2_winmd: Path
    appsdk_winmds: list[Path]
    win2d_winmds: list[Path]


# 输出控制
# 模块级详细输出开关：通过 --verbose / -v 参数设置
_verbose: bool = False


def set_verbose(val: bool) -> None:
    """设置全局详细输出模式，供入口脚本在解析参数后调用。"""
    global _verbose
    _verbose = val


# 文件发现基础设施
# 以下函数用于自动发现 src/ 中的源文件，替代硬编码的文件列表。
# 支持任意数量的 .xaml、.idl、.winmd 文件，无需修改构建脚本。


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
            msg = f"failed to remove stale file {str(stale)}: {exc}"
            if required:
                raise BuildError(msg) from exc
            print(f"warning: {msg}")


def remove_stale_projection_dir(directory: Path) -> None:
    """Remove the target-local winrt projection directory before regenerating component projections."""
    if not directory.is_dir():
        return
    try:
        shutil.rmtree(directory)
    except OSError as exc:
        raise BuildError(
            f"failed to remove stale projection directory {str(directory)}: {exc}"
        ) from exc


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


def absolute_path(path: Path) -> Path:
    return path.expanduser().resolve(strict=False)


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
        if stripped.startswith(
            (
                "ReferenceAssembly:",
                "ReferenceAssemblyPath:",
                "XamlPage:",
                "XamlApplication:",
                "ClIncludeFile:",
            )
        ):
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


def unique_parent_dirs(paths: list[Path]) -> list[Path]:
    seen: set[str] = set()
    result: list[Path] = []

    for path in paths:
        parent = path.parent
        key = os.path.normcase(str(parent))
        if key in seen:
            continue
        seen.add(key)
        result.append(parent)

    return result


def msbuild_item(path: Path, dependent_upon: Path | None = None) -> dict[str, str]:
    native = str(absolute_path(path))
    item = {"ItemSpec": native, "FullPath": native}
    if dependent_upon is not None:
        item["DependentUpon"] = str(absolute_path(dependent_upon))
    return item


def build_xaml_json(
    *,
    generated_dir: Path,
    src_dir: Path,
    namespace: str,
    winsdk_version: str,
    ref_winmds: list[Path],
    merged_winmd: Path,
    genxbf_path: Path,
    is_pass1: bool,
) -> dict[str, Any]:
    # 自动发现所有 .xaml 文件，按命名约定分类
    xaml_files = sorted(src_dir.rglob("*.xaml"))
    xaml_pages = xaml_files.copy()
    if not xaml_files:
        raise BuildError(f"no .xaml files found in {str(src_dir)}")
    # xaml_apps, xaml_pages = classify_xaml(xaml_files, src_dir)
    for i, xaml_app in enumerate(xaml_pages):
        if xaml_app == src_dir / "App.xaml":
            xaml_app = xaml_pages.pop(i)
            break
    else:
        raise BuildError(f"no App.xaml found in {str(src_dir)}")

    # 由 .xaml 文件推导对应的 .h 头文件列表
    header_files: list[tuple[Path, Path]] = []
    for xf in xaml_files:
        hf = xf.with_suffix(".xaml.h")
        if hf.is_file():
            header_files.append((hf, xf))
        else:
            print(
                f" warning: no header file found for {xf.name} (expected {hf.name}), skipping ClIncludeFiles entry"
            )

    data: dict[str, Any] = {
        "SavedStateFile": str(generated_dir / "XamlCompilerState.xml"),
        "IsPass1": is_pass1,
        "Language": "CppWinRT",
        "ProjectPath": str(absolute_path(xaml_app)),
        "LanguageSourceExtension": ".cpp",
        "OutputPath": str(generated_dir),
        "RootNamespace": namespace,
        "PrecompiledHeaderFile": "pch.h",
        "FeatureControlFlags": "EnableXBindDiagnostics;EnableDefaultValidationContextGeneration;EnableWin32Codegen",
        "ReferenceAssemblies": [msbuild_item(path) for path in ref_winmds],
        "ReferenceAssemblyPaths": [],
        "TargetPlatformMinVersion": winsdk_version,
        "XamlPages": [msbuild_item(p) for p in xaml_pages],
        "XamlApplications": [msbuild_item(xaml_app)],
        "ClIncludeFiles": [
            msbuild_item(hf, dependent_upon=xf) for hf, xf in header_files
        ],
    }

    if not is_pass1:
        data["LocalAssembly"] = [msbuild_item(merged_winmd)]
        data["GenXbfPath"] = str(genxbf_path)

    return data


def write_text(path: Path, content: str) -> bool:
    """Write content to path only if content differs from existing file.

    Returns True if file was written, False if skipped (content unchanged).
    """
    try:
        if path.is_file() and path.read_text(encoding="utf-8") == content:
            return False
    except (OSError, UnicodeDecodeError):
        pass
    try:
        path.write_text(content, encoding="utf-8", newline="\n")
        return True
    except OSError as exc:
        raise BuildError(f"failed to write {str(path)}: {exc}") from exc


def write_json(path: Path, data: dict[str, Any]) -> None:
    write_text(path, json.dumps(data, indent=2) + "\n")


def projection_fingerprint(
    *,
    build_script: Path,
    cppwinrt_exe: Path,
    platform_winmds: list[Path],
    webview2_winmd: Path,
    appsdk_winmds: list[Path],
    win2d_winmds: list[Path],
) -> dict[str, Any]:
    inputs = [
        build_script,
        cppwinrt_exe,
        *platform_winmds,
        webview2_winmd,
        *appsdk_winmds,
        *win2d_winmds,
    ]
    files: list[dict[str, Any]] = []

    for input_path in inputs:
        stat = input_path.stat()
        files.append(
            {
                "path": str(input_path),
                "size": stat.st_size,
                "mtime_ns": stat.st_mtime_ns,
            }
        )

    return {"version": 1, "files": files}


def is_shared_projection_current(
    shared_projection_dir: Path, fingerprint: dict[str, Any]
) -> bool:
    stamp_path = shared_projection_dir / ".shared_projection_stamp.json"
    required_headers = [
        shared_projection_dir / "winrt" / "Windows.Foundation.h",
        shared_projection_dir / "winrt" / "Microsoft.UI.Xaml.h",
        shared_projection_dir / "winrt" / "Microsoft.Web.WebView2.Core.h",
    ]

    if not all(path.is_file() for path in required_headers):
        return False
    if not stamp_path.is_file():
        return False

    try:
        existing = json.loads(stamp_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError):
        return False

    return existing == fingerprint


def write_shared_projection_stamp(
    shared_projection_dir: Path, fingerprint: dict[str, Any]
) -> None:
    write_json(shared_projection_dir / ".shared_projection_stamp.json", fingerprint)


def same_path(left: Path, right: Path) -> bool:
    return os.path.normcase(str(left)) == os.path.normcase(str(right))


def generate_shared_projection_headers(
    *,
    cppwinrt_exe: Path,
    shared_projection_dir: Path,
    platform_winmds: list[Path],
    webview2_winmd: Path,
    appsdk_winmds: list[Path],
    win2d_winmds: list[Path],
) -> None:
    # 阶段一：平台契约投影。
    phase1a = [str(cppwinrt_exe)]
    for winmd in platform_winmds:
        phase1a.extend(["-in", str(winmd)])
    phase1a.extend(["-out", str(shared_projection_dir)])
    run_command(phase1a)

    # 阶段二：WebView2 投影，引用平台契约。
    phase1b = [str(cppwinrt_exe), "-in", str(webview2_winmd)]
    for winmd in platform_winmds:
        phase1b.extend(["-ref", str(winmd)])
    phase1b.extend(["-out", str(shared_projection_dir)])
    run_command(phase1b)

    # 阶段三：Windows App SDK / WinUI 投影，引用平台契约与 WebView2。
    # 同时将 Win2D WinMD 作为输入一并投影，生成 Canvas 类型头文件。
    phase1c = [str(cppwinrt_exe)]
    for winmd in appsdk_winmds:
        phase1c.extend(["-in", str(winmd)])
    for winmd in win2d_winmds:
        phase1c.extend(["-in", str(winmd)])
    for winmd in platform_winmds:
        phase1c.extend(["-ref", str(winmd)])
    phase1c.extend(
        [
            "-ref",
            str(webview2_winmd),
            "-out",
            str(shared_projection_dir),
        ]
    )
    run_command(phase1c)


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
        str(midl_exe),
        str(idl),
        "/nologo",
        "/winrt",
        "/winmd",
        str(out_winmd),
        "/nomidl",
        "/h",
        "nul",
        "/metadata_dir",
        str(foundation_meta),
        "/I",
        str(sdk_include_dir / "um"),
        "/I",
        str(sdk_include_dir / "shared"),
        "/I",
        str(sdk_include_dir / "winrt"),
        # 支持 IDL 相对 import（例如 import "Common.idl"）
        "/I",
        str(idl.parent),
    ]
    for ref_winmd in ref_winmds:
        command.extend(["/reference", str(ref_winmd)])
    run_command(command, env=env)


# ── 布局与工具解析（由入口脚本在 main() 中调用）─────────────────


def resolve_layout(args: argparse.Namespace) -> BuildLayout:
    """根据解析后的命令行参数计算构建目录布局，并创建所有输出目录。

    Args:
        args: 已解析的 argparse.Namespace；build_dir 为 target:autogendir()
              传入的构建自动生成根目录，需包含 project_dir、namespace、
              src_dir、shared_projection_dir 等字段。

    Returns:
        包含所有标准化路径的 BuildLayout 实例。
    """
    project_dir = absolute_path(args.project_dir)
    build_dir = absolute_path(args.build_dir)
    generated_dir = build_dir / "generated"
    generated_sources_dir = generated_dir / "sources"
    shared_projection_dir = (
        absolute_path(args.shared_projection_dir)
        if args.shared_projection_dir
        else generated_dir
    )
    winmd_unmerged_dir = build_dir / "winmd_unmerged"
    winmd_merged_dir = build_dir / "winmd_merged"
    merged_winmd = winmd_merged_dir / f"{args.namespace}.winmd"

    src_dir = (
        absolute_path(Path(args.src_dir)) if args.src_dir else project_dir / "src"
    )

    # 创建所有输出目录
    for directory in [
        shared_projection_dir,
        generated_dir,
        generated_sources_dir,
        winmd_unmerged_dir,
        winmd_merged_dir,
    ]:
        directory.mkdir(parents=True, exist_ok=True)

    return BuildLayout(
        project_dir=project_dir,
        build_dir=build_dir,
        generated_dir=generated_dir,
        generated_sources_dir=generated_sources_dir,
        shared_projection_dir=shared_projection_dir,
        winmd_unmerged_dir=winmd_unmerged_dir,
        winmd_merged_dir=winmd_merged_dir,
        merged_winmd=merged_winmd,
        src_dir=src_dir,
    )


def resolve_inputs(args: argparse.Namespace, layout: BuildLayout) -> ResolvedInputs:
    """解析外部工具链、WinMD 引用与 Windows SDK 路径。

    包含平台发现（Windows SDK）、NuGet 包解析（packages.config）
    以及所有 WinMD 元数据收集，并对所有关键路径执行存在性校验。

    Args:
        args: 已解析的 argparse.Namespace，需包含 xaml_compiler_path（可选）等字段。
        layout: 由 resolve_layout() 返回的构建布局（预留，供未来扩展使用）。

    Returns:
        包含所有已验证外部输入的 ResolvedInputs 实例。

    Raises:
        BuildError: 所需文件或目录不存在时。
    """
    # layout 参数预留给未来可能需要基于构建布局解析输入的场景
    _ = layout

    # ── NuGet 包与工具路径解析 ──
    config = NuGetConfig.from_packages_config()

    # WinAppSDK 2.0.1 sub-packages (stable GA, April 2026, SemVer)
    foundation_pkg = config.package_path("Microsoft.WindowsAppSDK.Foundation")
    winui_pkg = config.package_path("Microsoft.WindowsAppSDK.WinUI")
    ixp_pkg = config.package_path("Microsoft.WindowsAppSDK.InteractiveExperiences")
    cppwinrt_exe = (
        config.package_path("Microsoft.Windows.CppWinRT") / "bin" / "cppwinrt.exe"
    )
    buildtools_bin = (
        config.package_path("Microsoft.Windows.SDK.BuildTools")
        / "bin"
        / BUILD_TOOLS_BIN_VERSION
        / "x64"
    )
    midl_exe = buildtools_bin / "midl.exe"
    mdmerge_exe = buildtools_bin / "mdmerge.exe"
    xaml_compiler = winui_pkg / "tools" / "net472" / "XamlCompiler.exe"
    genxbf_dir = winui_pkg / "tools"

    # 若用户提供了自定义 XAML 编译器路径指向独立编译器目录
    xaml_compiler_path_override = getattr(args, "xaml_compiler_path", None)
    if xaml_compiler_path_override:
        xaml_compiler = (
            absolute_path(Path(xaml_compiler_path_override)) / "XamlCompiler.exe"
        )

    # ── WinMD 收集 ──
    webview2_winmds = winmd_webview2.collect(config)
    if len(webview2_winmds) != 1:
        raise BuildError(
            f"expected exactly 1 WebView2 WinMD, got {len(webview2_winmds)}"
        )
    webview2_winmd = webview2_winmds[0]

    win2d_winmds = winmd_win2d.collect(config)

    # ── Windows SDK 解析 ──
    winsdk_root = require_dir(WINDOWS_SDK_ROOT, "Windows SDK root")
    winsdk_version, _ = platform_xml_path(winsdk_root, WINDOWS_SDK_VERSION)
    platform_winmds = winmd_platform.collect(winsdk_root, winsdk_version)
    appsdk_winmds = winmd_appsdk.collect(config)
    ref_winmds = [*platform_winmds, webview2_winmd, *appsdk_winmds, *win2d_winmds]
    foundation_meta = winmd_platform.find_metadata_dir(winsdk_root, winsdk_version)
    sdk_include_dir = require_dir(
        winsdk_root / "Include" / winsdk_version,
        "Windows SDK include directory",
    )

    # ── 完整性校验 ──
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

    for include_name in ["um", "shared", "winrt"]:
        require_dir(
            sdk_include_dir / include_name,
            f"Windows SDK {include_name} include directory",
        )

    return ResolvedInputs(
        cppwinrt_exe=cppwinrt_exe,
        midl_exe=midl_exe,
        mdmerge_exe=mdmerge_exe,
        buildtools_bin=buildtools_bin,
        winsdk_root=winsdk_root,
        winsdk_version=winsdk_version,
        platform_winmds=platform_winmds,
        appsdk_winmds=appsdk_winmds,
        webview2_winmd=webview2_winmd,
        win2d_winmds=win2d_winmds,
        ref_winmds=ref_winmds,
        foundation_meta=foundation_meta,
        sdk_include_dir=sdk_include_dir,
        xaml_compiler=xaml_compiler,
        genxbf_dir=genxbf_dir,
    )


def resolve_shared_projection_inputs(project_dir: Path) -> SharedProjectionInputs:
    """解析共享 C++/WinRT 投影头生成所需的输入路径。

    仅解析 WinMD 相关输入（平台、WebView2、AppSDK、Win2D）和
    cppwinrt.exe，不涉及 XAML 编译器、MIDL、mdmerge 或 MSVC 路径校验。

    Args:
        project_dir: 项目根目录绝对路径。

    Returns:
        包含所有已验证输入的 SharedProjectionInputs 实例。

    Raises:
        BuildError: 所需文件或目录不存在时。
    """
    # ── NuGet 包与工具路径解析 ──
    config = NuGetConfig.from_packages_config(project_root=project_dir)

    cppwinrt_exe = (
        config.package_path("Microsoft.Windows.CppWinRT") / "bin" / "cppwinrt.exe"
    )

    # WinAppSDK 2.0.1 sub-packages (用于目录存在性校验)
    foundation_pkg = config.package_path("Microsoft.WindowsAppSDK.Foundation")
    winui_pkg = config.package_path("Microsoft.WindowsAppSDK.WinUI")
    ixp_pkg = config.package_path("Microsoft.WindowsAppSDK.InteractiveExperiences")

    # ── WinMD 收集 ──
    webview2_winmds = winmd_webview2.collect(config)
    if len(webview2_winmds) != 1:
        raise BuildError(
            f"expected exactly 1 WebView2 WinMD, got {len(webview2_winmds)}"
        )
    webview2_winmd = webview2_winmds[0]

    win2d_winmds = winmd_win2d.collect(config)

    # ── Windows SDK 解析 ──
    winsdk_root = require_dir(WINDOWS_SDK_ROOT, "Windows SDK root")
    winsdk_version, _ = platform_xml_path(winsdk_root, WINDOWS_SDK_VERSION)
    platform_winmds = winmd_platform.collect(winsdk_root, winsdk_version)
    appsdk_winmds = winmd_appsdk.collect(config)

    # ── 完整性校验 ──
    require_file(cppwinrt_exe, "cppwinrt.exe")
    require_dir(foundation_pkg, "Foundation sub-package")
    require_dir(winui_pkg, "WinUI sub-package")
    require_dir(ixp_pkg, "InteractiveExperiences sub-package")

    return SharedProjectionInputs(
        cppwinrt_exe=cppwinrt_exe,
        platform_winmds=platform_winmds,
        webview2_winmd=webview2_winmd,
        appsdk_winmds=appsdk_winmds,
        win2d_winmds=win2d_winmds,
    )
