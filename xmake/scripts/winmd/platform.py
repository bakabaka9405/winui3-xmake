#!/usr/bin/env python3
"""Windows SDK Platform WinMD collection module.

提供从 Windows SDK API 契约中收集平台 WinMD 引用文件，
以及为 MIDL 编译定位 FoundationContract 元数据目录。
"""

from __future__ import annotations

import xml.etree.ElementTree as ET
from pathlib import Path

from plat_info import BuildError, parse_version, require_dir, require_file, platform_xml_path


# ── 内部辅助 ──────────────────────────────────────────────────


def _parse_contracts(platform_xml: Path) -> list[tuple[str, str]]:
    """从 Platform.xml 中解析 API 契约列表。"""
    try:
        root = ET.parse(platform_xml).getroot()
    except ET.ParseError as exc:
        raise BuildError(f"failed to parse {str(platform_xml)}: {exc}") from exc
    except OSError as exc:
        raise BuildError(f"failed to read {str(platform_xml)}: {exc}") from exc

    contracts: list[tuple[str, str]] = []
    for element in root.iter():
        if element.tag.rsplit("}", 1)[-1] != "ApiContract":
            continue

        name = element.attrib.get("name")
        version = element.attrib.get("version")
        if not name or not version:
            raise BuildError(
                f"ApiContract in {str(platform_xml)} is missing name or version"
            )
        contracts.append((name, version))

    if not contracts:
        raise BuildError(f"no ApiContract entries found in {str(platform_xml)}")
    return contracts


# ── 统一收集接口 ──────────────────────────────────────────────


def collect(winsdk_root: Path, winsdk_version: str) -> list[Path]:
    """收集平台 API 契约对应的 WinMD 引用文件。

    Args:
        winsdk_root: Windows SDK 根目录路径。
        winsdk_version: Windows SDK UAP 版本号。

    Returns:
        API 契约 WinMD 文件的 Path 列表。
    """
    _, platform_xml = platform_xml_path(winsdk_root, winsdk_version)
    contracts = _parse_contracts(platform_xml)
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


# ── 元数据目录发现 ────────────────────────────────────────────


def find_metadata_dir(winsdk_root: Path, winsdk_version: str) -> Path:
    """定位 Windows.Foundation.FoundationContract 元数据目录，供 MIDL 使用。

    Args:
        winsdk_root: Windows SDK 根目录路径。
        winsdk_version: Windows SDK UAP 版本号。

    Returns:
        FoundationContract 最高版本元数据目录的 Path 对象。
    """
    foundation_root = require_dir(
        winsdk_root
        / "References"
        / winsdk_version
        / "Windows.Foundation.FoundationContract",
        "Windows.Foundation.FoundationContract metadata directory",
    )
    versions = [path for path in foundation_root.iterdir() if path.is_dir()]
    if not versions:
        raise BuildError(
            f"no FoundationContract versions found under: {str(foundation_root)}"
        )
    return max(versions, key=lambda path: parse_version(path.name))
