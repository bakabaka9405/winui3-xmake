#!/usr/bin/env python3
"""NuGet 包配置解析模块。

从 packages.config 读取所有 NuGet 包的精确版本，
提供类型化查询接口与包路径构造功能，供 WinUI3 构建脚本使用。
"""

from __future__ import annotations

import os
import xml.etree.ElementTree as ET
from dataclasses import dataclass
from pathlib import Path


# ── 构建基础设施 ──────────────────────────────────────────────


@dataclass(slots=True, frozen=True)
class _PackageInfo:
    """单个 NuGet 包的元数据。

    Attributes:
        id: NuGet 包 ID（例如 "Microsoft.Windows.CppWinRT"）。
        version: 精确版本号（例如 "2.0.250303.1"）。
        target_framework: 目标框架标识（默认为 "native"）。
    """

    id: str
    version: str
    target_framework: str = "native"


# ── BuildTools 内部子版本常量 ─────────────────────────────────

# BuildTools 包版本为 10.0.28000.1721，其 bin/ 子目录使用不同的版本号格式。
# 该常量直接沿用 NuGet 包缓存目录的实际结构，与 packages.config 中的完整版本号无关。
BUILD_TOOLS_BIN_VERSION: str = "10.0.28000.0"


# ── NuGetConfig ────────────────────────────────────────────────


class NuGetConfig:
    """packages.config 的解析结果，提供包版本的查询与路径构造接口。

    设计约定：
    - 所有包路径基于 NuGet 全局包缓存目录构造。
    - 包目录名遵循 NuGet 小写约定（例如 Microsoft.Windows.CppWinRT → microsoft.windows.cppwinrt）。

    用法示例:
        config = NuGetConfig.from_packages_config()
        ver = config.version("Microsoft.Windows.CppWinRT")       # → "2.0.250303.1"
        pkg = config.package_path("Microsoft.Windows.CppWinRT")   # → Path(".../microsoft.windows.cppwinrt/2.0.250303.1")
    """

    __slots__ = ("_packages", "_nuget_root")

    def __init__(self, packages: dict[str, _PackageInfo], nuget_root: Path) -> None:
        """内部构造方法。请使用 from_packages_config() 类方法创建实例。

        Args:
            packages: 包 ID 到 _PackageInfo 的映射字典。
            nuget_root: NuGet 全局包缓存目录路径。
        """
        self._packages = packages
        self._nuget_root = nuget_root

    # ── 工厂方法 ──────────────────────────────────────────────

    @classmethod
    def from_packages_config(
        cls,
        project_root: Path | None = None,
        *,
        nuget_root: Path | None = None,
    ) -> NuGetConfig:
        """从 packages.config 文件解析所有 NuGet 包声明。

        Args:
            project_root: 项目根目录路径。默认为本模块所在目录的父目录。
            nuget_root: NuGet 全局包缓存目录。默认从 USERPROFILE 环境变量自动检测。

        Returns:
            包含所有包版本信息的 NuGetConfig 实例。

        Raises:
            FileNotFoundError: packages.config 文件不存在时。
            ET.ParseError: XML 解析失败时。
        """
        # 解析项目根目录
        if project_root is None:
            project_root = Path(__file__).resolve().parent.parent

        config_path = project_root / "packages.config"
        if not config_path.is_file():
            raise FileNotFoundError(
                f"packages.config not found at {config_path}"
            )

        # 解析 NuGet 根目录
        if nuget_root is None:
            user_profile = os.environ.get("USERPROFILE")
            if user_profile:
                nuget_root = Path(user_profile) / ".nuget" / "packages"
            else:
                nuget_root = Path(r"C:\Users\Admin\.nuget\packages")

        # 解析 XML
        try:
            tree = ET.parse(config_path)
            root = tree.getroot()
        except ET.ParseError as exc:
            raise ET.ParseError(
                f"failed to parse {config_path}: {exc}"
            ) from exc

        # 提取所有包声明
        packages: dict[str, _PackageInfo] = {}
        for element in root.iter("package"):
            pkg_id = element.attrib.get("id")
            pkg_ver = element.attrib.get("version")
            pkg_tfm = element.attrib.get("targetFramework", "native")

            if not pkg_id or not pkg_ver:
                raise ValueError(
                    f"package element in {config_path} is missing id or version attribute"
                )

            packages[pkg_id] = _PackageInfo(
                id=pkg_id,
                version=pkg_ver,
                target_framework=pkg_tfm,
            )

        return cls(packages=packages, nuget_root=nuget_root)

    # ── 查询接口 ──────────────────────────────────────────────

    def version(self, package_id: str) -> str:
        """获取指定包的精确版本号。

        Args:
            package_id: NuGet 包 ID（例如 "Microsoft.Windows.CppWinRT"）。

        Returns:
            版本号字符串（例如 "2.0.250303.1"）。

        Raises:
            KeyError: 包 ID 在 packages.config 中不存在时。
        """
        if package_id not in self._packages:
            raise KeyError(
                f"package '{package_id}' not found in packages.config"
            )
        return self._packages[package_id].version

    def package_path(self, package_id: str) -> Path:
        """构造包在 NuGet 全局缓存中的绝对路径。

        路径格式: <nuget_root>/<小写包名>/<版本号>

        Args:
            package_id: NuGet 包 ID（例如 "Microsoft.Windows.CppWinRT"）。

        Returns:
            包目录的 Path 对象。

        Raises:
            KeyError: 包 ID 在 packages.config 中不存在时。
        """
        pkg_info = self._packages.get(package_id)
        if pkg_info is None:
            raise KeyError(
                f"package '{package_id}' not found in packages.config"
            )
        return self._nuget_root / pkg_info.id.lower() / pkg_info.version

    # ── 便捷属性 ──────────────────────────────────────────────

    @property
    def nuget_root(self) -> Path:
        """NuGet 全局包缓存目录路径。"""
        return self._nuget_root

    @property
    def package_ids(self) -> tuple[str, ...]:
        """packages.config 中所有已声明的包 ID 列表。"""
        return tuple(self._packages.keys())
