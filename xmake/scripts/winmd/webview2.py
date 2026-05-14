#!/usr/bin/env python3
"""WebView2 WinMD collection module.

从 Microsoft.Web.WebView2 NuGet 包中收集 WebView2 Core WinMD 文件。
"""

from __future__ import annotations

from pathlib import Path

from plat_info import require_file
from nuget_config import NuGetConfig


# ── 统一收集接口 ──────────────────────────────────────────────


def collect(config: NuGetConfig) -> list[Path]:
    """从 WebView2 NuGet 包中收集 WinMD 文件。

    Args:
        config: 已解析的 NuGet 包配置实例。

    Returns:
        包含 Microsoft.Web.WebView2.Core.winmd 路径的单元素列表。
    """
    webview2_pkg = config.package_path("Microsoft.Web.WebView2")
    winmd = webview2_pkg / "lib" / "Microsoft.Web.WebView2.Core.winmd"
    return [require_file(winmd, "WebView2 WinMD")]
