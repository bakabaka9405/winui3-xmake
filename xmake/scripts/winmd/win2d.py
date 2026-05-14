#!/usr/bin/env python3
"""Win2D WinMD collection module.

从 Microsoft.Graphics.Win2D NuGet 包中收集 Win2D Canvas WinMD 文件。
"""

from __future__ import annotations

from pathlib import Path

from plat_info import require_dir, require_file
from nuget_config import NuGetConfig


# ── 统一收集接口 ──────────────────────────────────────────────


def collect(config: NuGetConfig) -> list[Path]:
    """从 Win2D NuGet 包中收集 WinMD 文件。

    Win2D v1.2.0+ 在 lib/uap10.0/ 子目录下提供单个 WinMD：
    Microsoft.Graphics.Canvas.winmd

    Args:
        config: 已解析的 NuGet 包配置实例。

    Returns:
        包含 Microsoft.Graphics.Canvas.winmd 路径的单元素列表。
    """
    win2d_pkg = config.package_path("Microsoft.Graphics.Win2D")
    lib_dir = require_dir(win2d_pkg / "lib" / "uap10.0", "Win2D lib/uap10.0 directory")
    winmd = lib_dir / "Microsoft.Graphics.Canvas.winmd"
    return [require_file(winmd, "Win2D WinMD: Microsoft.Graphics.Canvas.winmd")]
