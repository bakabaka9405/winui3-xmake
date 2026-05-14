#!/usr/bin/env python3
"""WinMD 收集统一入口包。

将平台 WinMD、Windows App SDK WinMD、WebView2 WinMD 和 Win2D WinMD
的收集逻辑抽象为统一的子模块接口。每个子模块暴露 `collect()` 函数，
返回 `list[Path]`。
"""

from __future__ import annotations

from pathlib import Path

from . import platform as platform
from . import appsdk as appsdk
from . import webview2 as webview2
from . import win2d as win2d
from nuget_config import NuGetConfig

# ── 聚合接口 ──────────────────────────────────────────────────


def collect_all(
    config: NuGetConfig,
    winsdk_root: Path,
    winsdk_version: str,
) -> list[Path]:
    """聚合所有来源的 WinMD 文件。

    Args:
        config: 已解析的 NuGet 包配置实例。
        winsdk_root: Windows SDK 根目录路径。
        winsdk_version: Windows SDK UAP 版本号。

    Returns:
        所有 WinMD 文件路径的扁平列表。
    """
    winmds: list[Path] = []
    winmds.extend(platform.collect(winsdk_root, winsdk_version))
    winmds.extend(webview2.collect(config))
    winmds.extend(appsdk.collect(config))
    winmds.extend(win2d.collect(config))
    return winmds
