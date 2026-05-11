#!/usr/bin/env python3
"""Standalone shared C++/WinRT projection header generator (Phase 0).

Generates C++/WinRT projection headers for platform contracts, WebView2,
Windows App SDK and Win2D independently from the pre-XAML pipeline.
"""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path

from build_winui3_common import (
    BuildError,
    SharedProjectionInputs,
    absolute_path,
    generate_shared_projection_headers,
    is_shared_projection_current,
    print_phase,
    projection_fingerprint,
    resolve_shared_projection_inputs,
    set_verbose,
    write_shared_projection_stamp,
)

__all__ = ["main"]


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate shared C++/WinRT projection headers (platform, WebView2, Windows App SDK, Win2D)."
    )
    parser.add_argument(
        "--project-dir",
        type=Path,
        default=Path(__file__).resolve().parent.parent,
        help="Project root directory.",
    )
    parser.add_argument(
        "--build-dir",
        type=Path,
        default=None,
        help="Build output directory (unused, accepted for CLI compatibility).",
    )
    parser.add_argument(
        "--shared-projection-dir",
        type=Path,
        required=True,
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

        # ── 解析路径 ──
        project_dir = absolute_path(args.project_dir)
        shared_projection_dir = absolute_path(args.shared_projection_dir)

        # ── 解析共享投影输入 ──
        inputs = resolve_shared_projection_inputs(project_dir)

        # ── 打印项目/共享摘要 ──
        print(f"Project: {str(project_dir)}")
        print(f"Shared:  {str(shared_projection_dir)}")
        print(
            f"Refs:    {len(inputs.platform_winmds)} platform, "
            f"{len(inputs.appsdk_winmds)} WinAppSDK, "
            f"1 WebView2, {len(inputs.win2d_winmds)} Win2D"
        )

        # ── Phase 0: 生成共享投影头 ──
        print_phase("[0] Generate shared C++/WinRT projection headers")
        fingerprint = projection_fingerprint(
            build_script=Path(__file__).resolve(),
            cppwinrt_exe=inputs.cppwinrt_exe,
            platform_winmds=inputs.platform_winmds,
            webview2_winmd=inputs.webview2_winmd,
            appsdk_winmds=inputs.appsdk_winmds,
            win2d_winmds=inputs.win2d_winmds,
        )
        if is_shared_projection_current(shared_projection_dir, fingerprint):
            print("  shared projections are up to date")
        else:
            # 确保输出目录存在
            shared_projection_dir.mkdir(parents=True, exist_ok=True)
            generate_shared_projection_headers(
                cppwinrt_exe=inputs.cppwinrt_exe,
                shared_projection_dir=shared_projection_dir,
                platform_winmds=inputs.platform_winmds,
                webview2_winmd=inputs.webview2_winmd,
                appsdk_winmds=inputs.appsdk_winmds,
                win2d_winmds=inputs.win2d_winmds,
            )
            write_shared_projection_stamp(shared_projection_dir, fingerprint)

        return 0

    except BuildError as e:
        print(f"error: {e}", file=sys.stderr)
        return 1
    except subprocess.CalledProcessError:
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
