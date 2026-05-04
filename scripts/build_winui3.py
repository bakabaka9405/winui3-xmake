#!/usr/bin/env python3
"""Run the WinUI 3 C++/WinRT code generation pipeline used by xmake."""

from __future__ import annotations

import argparse
import json
import os
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Any

from plat_info import (
    BUILD_TOOLS_BIN_VERSION,
    WINDOWS_SDK_ROOT,
    WINDOWS_SDK_VERSION,
    BuildError,
    collect_appsdk_winmds,
    collect_platform_winmds,
    find_foundation_metadata_dir,
    nuget_root,
    path_env_with_vc,
    platform_xml_path,
    require_dir,
    require_file,
    vc_bin_path,
)

# 输出控制
# 模块级详细输出开关：通过 --verbose / -v 参数设置
_verbose: bool = False


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


def write_text(path: Path, content: str) -> None:
    try:
        path.write_text(content, encoding="utf-8", newline="\n")
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
) -> dict[str, Any]:
    inputs = [
        build_script,
        cppwinrt_exe,
        *platform_winmds,
        webview2_winmd,
        *appsdk_winmds,
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


def generate_xaml_metadata_provider(generated_dir: Path, namespace: str) -> None:
    write_text(
        generated_dir / "XamlMetaDataProvider.idl",
        f"namespace {namespace}\n"
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


def generate_shared_projection_headers(
    *,
    cppwinrt_exe: Path,
    shared_projection_dir: Path,
    platform_winmds: list[Path],
    webview2_winmd: Path,
    appsdk_winmds: list[Path],
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
    phase1c = [str(cppwinrt_exe)]
    for winmd in appsdk_winmds:
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


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Build WinUI 3 generated C++/WinRT and XAML artifacts."
    )
    parser.add_argument(
        "--build-dir", type=Path, required=True, help="Build output directory."
    )
    parser.add_argument(
        "--project-dir",
        type=Path,
        default=Path(__file__).resolve().parent.parent,
        help="Project root directory. Defaults to this script's parent directory.",
    )
    parser.add_argument(
        "-v",
        "--verbose",
        action="store_true",
        default=False,
        help="Enable verbose output: show unfiltered tool output for debugging.",
    )
    parser.add_argument(
        "--namespace",
        type=str,
        default="xmake_demo",
        help="Root namespace for the project.",
    )
    parser.add_argument(
        "--src-dir",
        type=str,
        default=None,
        help="Source directory containing .xaml/.idl files. Defaults to <project-dir>/src.",
    )
    parser.add_argument(
        "--shared-projection-dir",
        type=Path,
        default=None,
        help="Shared output directory for platform, WebView2, and Windows App SDK C++/WinRT projection headers.",
    )
    parser.add_argument(
        "--xaml-compiler-path",
        type=Path,
        default=None,
        help="Custom directory containing XamlCompiler.exe. Only the compiler executable is overridden; GenXbf native DLLs still come from the NuGet package.",
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
        shared_projection_dir = (
            absolute_path(args.shared_projection_dir)
            if args.shared_projection_dir
            else generated_dir
        )
        winmd_unmerged_dir = build_dir / "winmd_unmerged"
        winmd_merged_dir = build_dir / "winmd_merged"
        merged_winmd = winmd_merged_dir / f"{args.namespace}.winmd"

        root = nuget_root()
        # WinAppSDK 1.8+ sub-packages (latest stable, April 2026)
        foundation_pkg = root / "microsoft.windowsappsdk.foundation" / "1.8.260415000"
        winui_pkg = root / "microsoft.windowsappsdk.winui" / "1.8.260415005"
        ixp_pkg = (
            root / "microsoft.windowsappsdk.interactiveexperiences" / "1.8.260415001"
        )
        cppwinrt_exe = (
            root
            / "microsoft.windows.cppwinrt"
            / "2.0.250303.1"
            / "bin"
            / "cppwinrt.exe"
        )
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
        genxbf_dir = winui_pkg / "tools"

        # 若用户提供了自定义 XAML 编译器路径
        if args.xaml_compiler_path:
            xaml_compiler = absolute_path(args.xaml_compiler_path) / "XamlCompiler.exe"

        webview2_winmd = (
            root
            / "microsoft.web.webview2"
            / "1.0.3912.50"
            / "lib"
            / "Microsoft.Web.WebView2.Core.winmd"
        )

        src_dir = require_dir(
            absolute_path(Path(args.src_dir)) if args.src_dir else project_dir / "src",
            "project src directory",
        )

        # 动态发现用户编写的源文件，替代硬编码文件列表
        xaml_files = sorted(src_dir.rglob("*.xaml"))
        if len(xaml_files) == 0:
            raise BuildError(f"no .xaml files found in {str(src_dir)}")

        idl_files = sorted(src_dir.rglob("*.idl"))

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
            require_dir(
                sdk_include_dir / include_name,
                f"Windows SDK {include_name} include directory",
            )

        for directory in [
            shared_projection_dir,
            generated_dir,
            generated_sources_dir,
            winmd_unmerged_dir,
            winmd_merged_dir,
        ]:
            directory.mkdir(parents=True, exist_ok=True)

        print(f"Project: {str(project_dir)}")
        print(f"Build:   {str(build_dir)}")
        print(f"Shared:  {str(shared_projection_dir)}")
        print(f"WinSDK:  {str(winsdk_root)} ({winsdk_version})")
        print(
            f"Refs:    {len(platform_winmds)} platform, {len(appsdk_winmds)} WinAppSDK, 1 WebView2"
        )

        print_phase("[1/8] Generate shared platform, WebView2, and WinAppSDK headers")
        shared_projection_fingerprint = projection_fingerprint(
            build_script=Path(__file__).resolve(),
            cppwinrt_exe=cppwinrt_exe,
            platform_winmds=platform_winmds,
            webview2_winmd=webview2_winmd,
            appsdk_winmds=appsdk_winmds,
        )
        if is_shared_projection_current(
            shared_projection_dir, shared_projection_fingerprint
        ):
            print("  shared projections are up to date")
        else:
            generate_shared_projection_headers(
                cppwinrt_exe=cppwinrt_exe,
                shared_projection_dir=shared_projection_dir,
                platform_winmds=platform_winmds,
                webview2_winmd=webview2_winmd,
                appsdk_winmds=appsdk_winmds,
            )
            write_shared_projection_stamp(
                shared_projection_dir, shared_projection_fingerprint
            )

        print_phase("[2/8] Compile IDL to WinMD")

        # 清理上次构建的陈旧 WinMD 文件（递归，因 IDL 可能位于子目录）
        clean_stale_files(winmd_unmerged_dir, "**/*.winmd", required=True)

        midl_env = path_env_with_vc(vc_bin_path())

        if not idl_files:
            # 零用户 IDL：自动生成最小 XamlMetaDataProvider 以供构建
            print(
                "  (no user IDL files found, auto-generating XamlMetaDataProvider.idl)"
            )
            generate_xaml_metadata_provider(generated_dir, args.namespace)
            idl_files = [generated_dir / "XamlMetaDataProvider.idl"]

        for idl_path in idl_files:
            # 保留相对于 src_dir 的目录结构生成 winmd，避免子目录同名 IDL 冲突
            out_winmd = idl_to_winmd_path(idl_path, src_dir, winmd_unmerged_dir)
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
        unmerged_winmds = sorted(winmd_unmerged_dir.rglob("*.winmd"))
        if not unmerged_winmds:
            raise BuildError(
                f"no .winmd files found in {str(winmd_unmerged_dir)} — MIDL compilation failed"
            )
        mdmerge = [str(mdmerge_exe), "-o", str(winmd_merged_dir)]
        for directory in unique_parent_dirs(ref_winmds):
            mdmerge.extend(["-metadata_dir", str(directory)])
        for wm in unmerged_winmds:
            mdmerge.extend(["-i", str(wm)])
        mdmerge.extend(["-partial", "-n:1"])
        run_command(mdmerge)
        require_file(merged_winmd, "merged WinMD")

        print_phase("[4/8] Generate project C++/WinRT sources")
        if not same_path(shared_projection_dir, generated_dir):
            remove_stale_projection_dir(generated_dir / "winrt")
        cppwinrt_project = [
            str(cppwinrt_exe),
            "-in",
            str(merged_winmd),
            "-out",
            str(generated_dir),
            "-comp",
            str(generated_sources_dir),
            "-name",
            args.namespace,
            "-pch",
            "pch.h",
            "-prefix",
            "-optimize",
            "-overwrite",
        ]
        for winmd in ref_winmds:
            cppwinrt_project.extend(["-ref", str(winmd)])
        run_command(cppwinrt_project)

        print_phase("[5/8] Generate XamlMetaDataProvider source")
        generate_xaml_metadata_provider(generated_dir, args.namespace)

        print_phase("[6/8] Run XAML compiler pass 1")

        # 清理上次构建的陈旧 .xbf 文件，防止已删除 XAML 的残留 XBF 被 makepri 打包
        clean_stale_files(generated_dir, "**/*.xbf")

        pass1_json = generated_dir / "xaml.pass1.in.json"
        pass1_out = generated_dir / "xaml.pass1.out.json"
        write_json(
            pass1_json,
            build_xaml_json(
                generated_dir=generated_dir,
                src_dir=src_dir,
                namespace=args.namespace,
                winsdk_version=winsdk_version,
                ref_winmds=ref_winmds,
                merged_winmd=merged_winmd,
                genxbf_path=genxbf_dir,
                is_pass1=True,
            ),
        )
        run_command(
            [
                str(xaml_compiler),
                str(pass1_json),
                str(pass1_out),
            ]
        )

        print_phase("[7/8] Run XAML compiler pass 2")
        pass2_json = generated_dir / "xaml.pass2.in.json"
        pass2_out = generated_dir / "xaml.pass2.out.json"
        write_json(
            pass2_json,
            build_xaml_json(
                generated_dir=generated_dir,
                src_dir=src_dir,
                namespace=args.namespace,
                winsdk_version=winsdk_version,
                ref_winmds=ref_winmds,
                merged_winmd=merged_winmd,
                genxbf_path=genxbf_dir,
                is_pass1=False,
            ),
        )
        run_command(
            [
                str(xaml_compiler),
                str(pass2_json),
                str(pass2_out),
            ]
        )

        # Phase 8: Generate .pri resource file (makepri)
        print_phase("[8/8] Generate .pri resource index")
        makepri_exe = buildtools_bin / "makepri.exe"
        if not makepri_exe.is_file():
            raise BuildError(f"makepri.exe not found at {makepri_exe}")

        # Collect .xbf files generated by XamlCompiler (recursive to cover subdirectories)
        xbf_files = sorted(generated_dir.rglob("*.xbf"))
        if len(xbf_files) == 0:
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
        run_command(
            [
                str(makepri_exe),
                "new",
                "/cf",
                str(priconfig_path),
                "/pr",
                str(project_dir),
                "/o",
                "/of",
                str(pri_output),
            ]
        )

        print("\n=== WinUI 3 code generation complete ===", flush=True)
    except BuildError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1
    except subprocess.CalledProcessError:
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
