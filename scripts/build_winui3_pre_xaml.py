#!/usr/bin/env python3
"""Pre-XAML build entry: executes phases 2-4 (MIDL, mdmerge, cppwinrt)."""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path

from build_winui3_common import (
    BuildError,
    clean_stale_files,
    idl_to_winmd_path,
    print_phase,
    remove_stale_projection_dir,
    resolve_inputs,
    resolve_layout,
    run_command,
    run_midl,
    same_path,
    set_verbose,
    unique_parent_dirs,
)
from plat_info import (
    path_env_with_vc,
    require_file,
    vc_bin_path,
)

__all__ = ["main"]


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Build WinUI 3 pre-XAML artifacts (MIDL, mdmerge, cppwinrt)."
    )
    parser.add_argument(
        "--build-dir",
        type=Path,
        required=True,
        help="Build auto-generated intermediate directory (autogendir root, passed from xmake).",
    )
    parser.add_argument(
        "--project-dir",
        type=Path,
        default=Path(__file__).resolve().parent.parent,
        help="Project root directory.",
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
        help="Source directory containing .idl files.",
    )
    parser.add_argument(
        "--shared-projection-dir",
        type=Path,
        default=None,
        help="Shared output directory for platform, WebView2, and Windows App SDK C++/WinRT projection headers.",
    )
    parser.add_argument(
        "-v",
        "--verbose",
        action="store_true",
        default=False,
        help="Enable verbose output: show unfiltered tool output for debugging.",
    )
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    try:
        # ── 解析参数并设置详细模式 ──
        args = parse_args(sys.argv[1:] if argv is None else argv)
        set_verbose(args.verbose)

        # ── 解析构建布局与外部输入 ──
        layout = resolve_layout(args)
        inputs = resolve_inputs(args, layout)

        # ── 打印项目/构建/共享/WinSDK 摘要 ──
        print(f"Project: {str(layout.project_dir)}")
        print(f"Build:   {str(layout.build_dir)}")
        print(f"Shared:  {str(layout.shared_projection_dir)}")
        print(f"WinSDK:  {str(inputs.winsdk_root)} ({inputs.winsdk_version})")
        print(
            f"Refs:    {len(inputs.platform_winmds)} platform, {len(inputs.appsdk_winmds)} WinAppSDK, 1 WebView2"
        )

        # ── Phase 2: MIDL 编译 ──
        print_phase("MIDL compilation")

        # 清理上次构建的陈旧 WinMD 文件
        clean_stale_files(layout.winmd_unmerged_dir, "**/*.winmd", required=True)

        midl_env = path_env_with_vc(vc_bin_path())

        # 从源码目录动态发现 IDL 文件
        idl_files = sorted(layout.src_dir.rglob("*.idl"))

        # 若存在自动生成的 XamlMetaDataProvider.idl，追加到编译列表
        xmp_idl = layout.generated_dir / "XamlMetaDataProvider.idl"
        if xmp_idl.exists():
            idl_files.append(xmp_idl)

        for idl_path in idl_files:
            # 保留相对于 src_dir 的目录结构生成 winmd，避免子目录同名 IDL 冲突
            out_winmd = idl_to_winmd_path(
                idl_path, layout.src_dir, layout.winmd_unmerged_dir
            )
            out_winmd.parent.mkdir(parents=True, exist_ok=True)
            run_midl(
                midl_exe=inputs.midl_exe,
                idl=idl_path,
                out_winmd=out_winmd,
                foundation_meta=inputs.foundation_meta,
                sdk_include_dir=inputs.sdk_include_dir,
                ref_winmds=inputs.ref_winmds,
                env=midl_env,
            )

        # ── Phase 3: 合并 WinMD ──
        print_phase("Merge WinMDs")
        unmerged_winmds = sorted(layout.winmd_unmerged_dir.rglob("*.winmd"))
        if not unmerged_winmds:
            raise BuildError(
                f"no .winmd files found in {str(layout.winmd_unmerged_dir)} — MIDL compilation failed"
            )
        mdmerge = [str(inputs.mdmerge_exe), "-o", str(layout.winmd_merged_dir)]
        for directory in unique_parent_dirs(inputs.ref_winmds):
            mdmerge.extend(["-metadata_dir", str(directory)])
        for wm in unmerged_winmds:
            mdmerge.extend(["-i", str(wm)])
        mdmerge.extend(["-partial", "-n:1"])
        run_command(mdmerge)
        require_file(layout.merged_winmd, "merged WinMD")

        # ── Phase 4: 生成项目 C++/WinRT 源码 ──
        print_phase("Generate project C++/WinRT sources")

        # 若共享投影目录与生成目录不同，清理本地投影以避免冲突
        if not same_path(layout.shared_projection_dir, layout.generated_dir):
            remove_stale_projection_dir(layout.generated_dir / "winrt")

        cppwinrt_project = [
            str(inputs.cppwinrt_exe),
            "-in",
            str(layout.merged_winmd),
            "-out",
            str(layout.generated_dir),
            "-comp",
            str(layout.generated_sources_dir),
            "-name",
            args.namespace,
            "-pch",
            "pch.h",
            "-prefix",
            "-optimize",
            "-overwrite",
        ]
        for winmd in inputs.ref_winmds:
            cppwinrt_project.extend(["-ref", str(winmd)])
        run_command(cppwinrt_project)

        print("=== WinUI 3 pre-XAML generation complete ===")
        return 0

    except BuildError as e:
        print(f"error: {e}", file=sys.stderr)
        return 1
    except subprocess.CalledProcessError:
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
