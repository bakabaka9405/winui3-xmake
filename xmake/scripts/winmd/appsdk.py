#!/usr/bin/env python3
"""Windows App SDK WinMD collection module.

从 Windows App SDK 2.0.1+ 的三个子包中自动发现并收集 WinMD 元数据文件：
Microsoft.WindowsAppSDK.Foundation、Microsoft.WindowsAppSDK.WinUI、
Microsoft.WindowsAppSDK.InteractiveExperiences。

采用 glob 方式自动发现 metadata/ 目录下的所有 *.winmd 文件，
无需维护硬编码文件名清单，兼容未来 WinMD 增删。
"""

from __future__ import annotations

from pathlib import Path

from plat_info import BuildError, parse_version, require_dir
from nuget_config import NuGetConfig


# ── 统一收集接口 ──────────────────────────────────────────────


def collect(config: NuGetConfig) -> list[Path]:
    """从 WinAppSDK 子包元数据目录中自动收集 WinMD 文件。

    Args:
        config: 已解析的 NuGet 包配置实例。

    Returns:
        所有 WinAppSDK WinMD 文件的 Path 列表（按文件名排序）。
    """
    winmds: list[Path] = []

    # Foundation 子包 — 根级别 metadata/*.winmd
    foundation_pkg = config.package_path("Microsoft.WindowsAppSDK.Foundation")
    foundation_meta = require_dir(
        foundation_pkg / "metadata", "Foundation metadata directory"
    )
    winmds.extend(sorted(foundation_meta.glob("*.winmd")))

    # WinUI 子包 — 根级别 metadata/*.winmd
    winui_pkg = config.package_path("Microsoft.WindowsAppSDK.WinUI")
    winui_meta = require_dir(winui_pkg / "metadata", "WinUI metadata directory")
    winmds.extend(sorted(winui_meta.glob("*.winmd")))

    # InteractiveExperiences 子包 — 版本化 metadata/<max_version>/*.winmd
    ixp_pkg = config.package_path("Microsoft.WindowsAppSDK.InteractiveExperiences")
    ixp_meta_root = require_dir(
        ixp_pkg / "metadata", "InteractiveExperiences metadata directory"
    )
    ixp_versions = sorted(
        (d.name for d in ixp_meta_root.iterdir() if d.is_dir()),
        key=parse_version,
    )
    if not ixp_versions:
        raise BuildError(f"no version sub-directories in: {str(ixp_meta_root)}")
    ixp_meta = require_dir(
        ixp_meta_root / ixp_versions[-1], "InteractiveExperiences version metadata"
    )
    winmds.extend(sorted(ixp_meta.glob("*.winmd")))

    return winmds
