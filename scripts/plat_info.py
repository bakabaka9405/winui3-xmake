#!/usr/bin/env python3
"""Windows 平台信息发现与路径解析工具。

提供 Windows SDK 发现、MSVC 工具链定位及 NuGet 路径解析功能，
供 build_winui3.py 等构建脚本引用。

WinMD 收集逻辑已提取至 scripts/winmd/ 包，各子模块提供统一接口。
"""

from __future__ import annotations

import os
import shutil
from pathlib import Path


# ── 构建基础设施 ──────────────────────────────────────────────


class BuildError(RuntimeError):
    """面向用户的构建失败异常。"""

    pass


def parse_version(version: str) -> tuple[int, ...]:
    """将语义化版本字符串解析为整数元组，用于版本比较。"""
    return tuple(int(i) if i.isdigit() else -1 for i in version.split("."))


def require_file(path: Path, label: str) -> Path:
    """断言 path 为已存在的文件，否则抛出 BuildError。"""
    if not path.is_file():
        raise BuildError(f"{label} does not exist: {str(path)}")
    return path


def require_dir(path: Path, label: str) -> Path:
    """断言 path 为已存在的目录，否则抛出 BuildError。"""
    if not path.is_dir():
        raise BuildError(f"{label} does not exist: {str(path)}")
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


def discover_winsdk_version(winsdk_root: Path) -> str:
    """发现可用的最高 Windows SDK UAP 版本（带目录校验）。"""
    uap_dir = require_dir(
        winsdk_root / "Platforms" / "UAP", "Windows SDK UAP platform directory"
    )
    versions = [path.name for path in uap_dir.iterdir() if path.is_dir()]
    if not versions:
        raise BuildError(f"no Windows SDK UAP versions found under: {str(uap_dir)}")
    return max(versions, key=parse_version)


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
try:
    WINDOWS_SDK_VERSION = discover_winsdk_version(WINDOWS_SDK_ROOT)
except BuildError:
    WINDOWS_SDK_VERSION = "10.0.26100.0"
DEFAULT_VC_BIN = _discover_vc_bin()


# ── Platform.xml 解析 ────────────────────────────────────────


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
    env["PATH"] = f"{str(vc_bin)};{env.get('PATH', '')}"

    if shutil.which("cl.exe", path=env["PATH"]) is None:
        raise BuildError(
            f"cl.exe was not found after adding VC tools directory to PATH: {str(vc_bin)}"
        )

    return env
