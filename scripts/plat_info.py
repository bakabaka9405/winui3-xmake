#!/usr/bin/env python3
"""Windows 平台信息发现与路径解析工具。

提供 Windows SDK 发现、WinMD 收集、MSVC 工具链定位及 NuGet 路径解析功能，
供 build_winui3.py 等构建脚本引用。
"""

from __future__ import annotations

import os
import shutil
import xml.etree.ElementTree as ET
from pathlib import Path


# ── 构建基础设施 ──────────────────────────────────────────────


class BuildError(RuntimeError):
    """面向用户的构建失败异常。"""


def native_path(path: str | Path) -> str:
    """将路径转换为 Windows 原生格式（反斜杠分隔）。"""
    return str(path)


def parse_version(version: str) -> tuple[int, ...]:
    """将语义化版本字符串解析为整数元组，用于版本比较。"""
    return tuple(int(i) if i.isdigit() else -1 for i in version.split("."))


def require_file(path: Path, label: str) -> Path:
    """断言 path 为已存在的文件，否则抛出 BuildError。"""
    if not path.is_file():
        raise BuildError(f"{label} does not exist: {native_path(path)}")
    return path


def require_dir(path: Path, label: str) -> Path:
    """断言 path 为已存在的目录，否则抛出 BuildError。"""
    if not path.is_dir():
        raise BuildError(f"{label} does not exist: {native_path(path)}")
    return path


# ── Windows SDK 发现与版本解析 ────────────────────────────────


def _discover_winsdk_root() -> Path:
    """从环境变量或注册表发现 Windows SDK 根目录。"""
    env = os.environ.get("WindowsSdkDir")
    if env:
        return Path(env)
    try:
        import winreg

        key = winreg.OpenKey(
            winreg.HKEY_LOCAL_MACHINE,
            r"SOFTWARE\Microsoft\Windows Kits\Installed Roots",
        )
        value, _ = winreg.QueryValueEx(key, "KitsRoot10")
        winreg.CloseKey(key)
        return Path(value)
    except OSError:
        pass
    return Path(r"C:\Program Files (x86)\Windows Kits\10")


def _discover_winsdk_version(sdk_root: Path) -> str:
    """发现可用的最高 UAP SDK 版本。"""
    uap_dir = sdk_root / "Platforms" / "UAP"
    if not uap_dir.is_dir():
        return "10.0.26100.0"
    versions = sorted(d.name for d in uap_dir.iterdir() if d.is_dir())
    return versions[-1] if versions else "10.0.26100.0"


def _discover_vc_bin() -> Path:
    """发现 MSVC 工具链二进制目录。"""
    vctools = os.environ.get("VCToolsInstallDir")
    if vctools:
        return Path(vctools) / "bin" / "HostX64" / "x64"
    candidates = [
        Path(r"C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC"),
        Path(r"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC"),
        Path(
            r"C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC"
        ),
        Path(r"C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Tools\MSVC"),
    ]
    for base in candidates:
        if base.is_dir():
            versions = sorted(d.name for d in base.iterdir() if d.is_dir())
            if versions:
                return base / versions[-1] / "bin" / "HostX64" / "x64"
    return Path(
        r"C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\bin\HostX64\x64"
    )


# ── 模块级预计算常量 ──────────────────────────────────────────

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


# ── SDK 版本发现、Platform.xml 解析与 WinMD 收集 ──────────────


def discover_winsdk_version(winsdk_root: Path) -> str:
    """公开的 Windows SDK 版本发现函数（带目录校验）。"""
    uap_dir = require_dir(
        winsdk_root / "Platforms" / "UAP", "Windows SDK UAP platform directory"
    )
    versions = [path.name for path in uap_dir.iterdir() if path.is_dir()]
    if not versions:
        raise BuildError(
            f"no Windows SDK UAP versions found under: {native_path(uap_dir)}"
        )
    return max(versions, key=parse_version)


def platform_xml_path(winsdk_root: Path, requested_version: str) -> tuple[str, Path]:
    """定位 Platform.xml 路径，若请求版本不存在则回退至已安装的最高版本。"""
    platform_xml = (
        winsdk_root / "Platforms" / "UAP" / requested_version / "Platform.xml"
    )
    if platform_xml.is_file():
        return requested_version, platform_xml

    detected_version = discover_winsdk_version(winsdk_root)
    detected_xml = winsdk_root / "Platforms" / "UAP" / detected_version / "Platform.xml"
    return detected_version, require_file(detected_xml, "Windows SDK Platform.xml")


def parse_platform_contracts(platform_xml: Path) -> list[tuple[str, str]]:
    """从 Platform.xml 中解析 API 契约列表。"""
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
            raise BuildError(
                f"ApiContract in {native_path(platform_xml)} is missing name or version"
            )
        contracts.append((name, version))

    if not contracts:
        raise BuildError(f"no ApiContract entries found in {native_path(platform_xml)}")
    return contracts


def collect_platform_winmds(winsdk_root: Path, winsdk_version: str) -> list[Path]:
    """收集平台 API 契约对应的 WinMD 引用文件。"""
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
    """从 WinAppSDK 1.8+ 子包元数据目录中收集 WinMD 文件。"""
    winmds: list[Path] = []

    # Foundation metadata (root-level)
    foundation_meta = require_dir(
        foundation_pkg / "metadata", "Foundation metadata directory"
    )
    for name in APP_SDK_WINMDS_FOUNDATION:
        candidate = foundation_meta / f"{name}.winmd"
        winmds.append(require_file(candidate, f"Foundation WinMD {name}"))

    # WinUI metadata (root-level)
    winui_meta = require_dir(winui_pkg / "metadata", "WinUI metadata directory")
    for name in APP_SDK_WINMDS_WINUI:
        candidate = winui_meta / f"{name}.winmd"
        winmds.append(require_file(candidate, f"WinUI WinMD {name}"))

    # InteractiveExperiences metadata (versioned sub-directory)
    ixp_meta_root = require_dir(
        ixp_pkg / "metadata", "InteractiveExperiences metadata directory"
    )
    ixp_versions = sorted(d.name for d in ixp_meta_root.iterdir() if d.is_dir())
    if not ixp_versions:
        raise BuildError(f"no version sub-directories in: {native_path(ixp_meta_root)}")
    ixp_meta = require_dir(
        ixp_meta_root / ixp_versions[-1], "InteractiveExperiences version metadata"
    )
    for name in APP_SDK_WINMDS_IXP:
        candidate = ixp_meta / f"{name}.winmd"
        winmds.append(require_file(candidate, f"InteractiveExperiences WinMD {name}"))

    return winmds


def find_foundation_metadata_dir(winsdk_root: Path, winsdk_version: str) -> Path:
    """定位 Windows.Foundation.FoundationContract 元数据目录。"""
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
            f"no FoundationContract versions found under: {native_path(foundation_root)}"
        )
    return max(versions, key=lambda path: parse_version(path.name))


# ── NuGet / VC 工具链路径 ────────────────────────────────────


def nuget_root() -> Path:
    """返回 NuGet 全局包缓存目录。"""
    user_profile = os.environ.get("USERPROFILE")
    if user_profile:
        return Path(user_profile) / ".nuget" / "packages"
    return Path(r"C:\Users\Admin\.nuget\packages")


def vc_bin_path() -> Path:
    """返回 MSVC 工具链二进制目录（优先环境变量，回退至默认值）。"""
    tools_dir = os.environ.get("VCToolsInstallDir")
    if tools_dir:
        return Path(tools_dir) / "bin" / "HostX64" / "x64"
    return DEFAULT_VC_BIN


def path_env_with_vc(vc_bin: Path) -> dict[str, str]:
    """构建包含 MSVC 工具链路径的 PATH 环境变量字典，并校验 cl.exe 可达。"""
    env = os.environ.copy()
    env["PATH"] = f"{native_path(vc_bin)};{env.get('PATH', '')}"

    if shutil.which("cl.exe", path=env["PATH"]) is None:
        raise BuildError(
            f"cl.exe was not found after adding VC tools directory to PATH: {native_path(vc_bin)}"
        )

    return env
