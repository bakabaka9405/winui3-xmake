#!/usr/bin/env python3
"""Run the WinUI 3 C++/WinRT code generation pipeline used by xmake."""

from __future__ import annotations

import argparse
import json
import os
import shutil
import subprocess
import sys
import xml.etree.ElementTree as ET
from pathlib import Path
from typing import Any


ROOT_NAMESPACE = "xmake_demo"
FEATURE_CONTROL_FLAGS = (
    "EnableXBindDiagnostics;"
    "EnableDefaultValidationContextGeneration;"
    "EnableWin32Codegen"
)

def _discover_winsdk_root() -> Path:
    """Discover Windows SDK root from environment or registry."""
    env = os.environ.get("WindowsSdkDir")
    if env:
        return Path(env)
    try:
        import winreg
        key = winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE,
                            r"SOFTWARE\Microsoft\Windows Kits\Installed Roots")
        value, _ = winreg.QueryValueEx(key, "KitsRoot10")
        winreg.CloseKey(key)
        return Path(value)
    except OSError:
        pass
    return Path(r"C:\Program Files (x86)\Windows Kits\10")

def _discover_winsdk_version(sdk_root: Path) -> str:
    """Discover highest available UAP SDK version."""
    uap_dir = sdk_root / "Platforms" / "UAP"
    if not uap_dir.is_dir():
        return "10.0.26100.0"
    versions = sorted(d.name for d in uap_dir.iterdir() if d.is_dir())
    return versions[-1] if versions else "10.0.26100.0"

def _discover_vc_bin() -> Path:
    """Discover VC toolchain binary directory."""
    vctools = os.environ.get("VCToolsInstallDir")
    if vctools:
        return Path(vctools) / "bin" / "HostX64" / "x64"
    # Try common locations
    candidates = [
        Path(r"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC"),
        Path(r"C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC"),
        Path(r"C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Tools\MSVC"),
        Path(r"C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC"),
    ]
    for base in candidates:
        if base.is_dir():
            versions = sorted(d.name for d in base.iterdir() if d.is_dir())
            if versions:
                return base / versions[-1] / "bin" / "HostX64" / "x64"
    return Path(r"C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\bin\HostX64\x64")

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


class BuildError(RuntimeError):
    """Raised for user-facing build failures."""


def native_path(path: str | Path) -> str:
    return os.path.normpath(os.fspath(path)).replace("/", "\\")


def absolute_path(path: Path) -> Path:
    return path.expanduser().resolve(strict=False)


def parse_version(version: str) -> tuple[int, ...]:
    parts: list[int] = []
    for part in version.split("."):
        try:
            parts.append(int(part))
        except ValueError:
            parts.append(-1)
    return tuple(parts)


def require_file(path: Path, label: str) -> Path:
    if not path.is_file():
        raise BuildError(f"{label} does not exist: {native_path(path)}")
    return path


def require_dir(path: Path, label: str) -> Path:
    if not path.is_dir():
        raise BuildError(f"{label} does not exist: {native_path(path)}")
    return path


def print_phase(title: str) -> None:
    print(f"\n=== {title} ===", flush=True)


def run_command(command: list[str], env: dict[str, str] | None = None) -> None:
    print("  " + subprocess.list2cmdline(command), flush=True)
    try:
        subprocess.run(command, check=True, env=env)
    except subprocess.CalledProcessError as exc:
        print(f"error: command failed with exit code {exc.returncode}:", file=sys.stderr)
        print(subprocess.list2cmdline(command), file=sys.stderr)
        raise


def discover_winsdk_version(winsdk_root: Path) -> str:
    uap_dir = require_dir(winsdk_root / "Platforms" / "UAP", "Windows SDK UAP platform directory")
    versions = [path.name for path in uap_dir.iterdir() if path.is_dir()]
    if not versions:
        raise BuildError(f"no Windows SDK UAP versions found under: {native_path(uap_dir)}")
    return max(versions, key=parse_version)


def platform_xml_path(winsdk_root: Path, requested_version: str) -> tuple[str, Path]:
    platform_xml = winsdk_root / "Platforms" / "UAP" / requested_version / "Platform.xml"
    if platform_xml.is_file():
        return requested_version, platform_xml

    detected_version = discover_winsdk_version(winsdk_root)
    detected_xml = winsdk_root / "Platforms" / "UAP" / detected_version / "Platform.xml"
    return detected_version, require_file(detected_xml, "Windows SDK Platform.xml")


def parse_platform_contracts(platform_xml: Path) -> list[tuple[str, str]]:
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
            raise BuildError(f"ApiContract in {native_path(platform_xml)} is missing name or version")
        contracts.append((name, version))

    if not contracts:
        raise BuildError(f"no ApiContract entries found in {native_path(platform_xml)}")
    return contracts


def collect_platform_winmds(winsdk_root: Path, winsdk_version: str) -> list[Path]:
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
    """Collect WinMD files from WinAppSDK 1.8+ sub-package metadata directories."""
    winmds: list[Path] = []

    # Foundation metadata (root-level)
    foundation_meta = require_dir(foundation_pkg / "metadata", "Foundation metadata directory")
    for name in APP_SDK_WINMDS_FOUNDATION:
        candidate = foundation_meta / f"{name}.winmd"
        winmds.append(require_file(candidate, f"Foundation WinMD {name}"))

    # WinUI metadata (root-level)
    winui_meta = require_dir(winui_pkg / "metadata", "WinUI metadata directory")
    for name in APP_SDK_WINMDS_WINUI:
        candidate = winui_meta / f"{name}.winmd"
        winmds.append(require_file(candidate, f"WinUI WinMD {name}"))

    # InteractiveExperiences metadata (versioned sub-directory)
    ixp_meta_root = require_dir(ixp_pkg / "metadata", "InteractiveExperiences metadata directory")
    ixp_versions = sorted(d.name for d in ixp_meta_root.iterdir() if d.is_dir())
    if not ixp_versions:
        raise BuildError(f"no version sub-directories in: {native_path(ixp_meta_root)}")
    ixp_meta = require_dir(ixp_meta_root / ixp_versions[-1], "InteractiveExperiences version metadata")
    for name in APP_SDK_WINMDS_IXP:
        candidate = ixp_meta / f"{name}.winmd"
        winmds.append(require_file(candidate, f"InteractiveExperiences WinMD {name}"))

    return winmds


def find_foundation_metadata_dir(winsdk_root: Path, winsdk_version: str) -> Path:
    foundation_root = require_dir(
        winsdk_root / "References" / winsdk_version / "Windows.Foundation.FoundationContract",
        "Windows.Foundation.FoundationContract metadata directory",
    )
    versions = [path for path in foundation_root.iterdir() if path.is_dir()]
    if not versions:
        raise BuildError(f"no FoundationContract versions found under: {native_path(foundation_root)}")
    return max(versions, key=lambda path: parse_version(path.name))


def unique_parent_dirs(paths: list[Path]) -> list[Path]:
    seen: set[str] = set()
    result: list[Path] = []

    for path in paths:
        parent = path.parent
        key = os.path.normcase(native_path(parent))
        if key in seen:
            continue
        seen.add(key)
        result.append(parent)

    return result


def nuget_root() -> Path:
    user_profile = os.environ.get("USERPROFILE")
    if user_profile:
        return Path(user_profile) / ".nuget" / "packages"
    return Path(r"C:\Users\WanYa\.nuget\packages")


def vc_bin_path() -> Path:
    tools_dir = os.environ.get("VCToolsInstallDir")
    if tools_dir:
        return Path(tools_dir) / "bin" / "HostX64" / "x64"
    return DEFAULT_VC_BIN


def path_env_with_vc(vc_bin: Path) -> dict[str, str]:
    env = os.environ.copy()
    env["PATH"] = f"{native_path(vc_bin)};{env.get('PATH', '')}"

    if shutil.which("cl.exe", path=env["PATH"]) is None:
        raise BuildError(
            f"cl.exe was not found after adding VC tools directory to PATH: {native_path(vc_bin)}"
        )

    return env


def msbuild_item(path: Path, dependent_upon: Path | None = None) -> dict[str, str]:
    native = native_path(absolute_path(path))
    item = {"ItemSpec": native, "FullPath": native}
    if dependent_upon is not None:
        item["DependentUpon"] = native_path(absolute_path(dependent_upon))
    return item


def build_xaml_json(
    *,
    generated_dir: Path,
    project_dir: Path,
    winsdk_version: str,
    ref_winmds: list[Path],
    merged_winmd: Path,
    genxbf_path: Path,
    is_pass1: bool,
) -> dict[str, Any]:
    src_dir = project_dir / "src"
    app_xaml = src_dir / "App.xaml"
    main_window_xaml = src_dir / "MainWindow.xaml"
    app_header = src_dir / "App.xaml.h"
    main_window_header = src_dir / "MainWindow.xaml.h"

    data: dict[str, Any] = {
        "SavedStateFile": native_path(generated_dir / "XamlCompilerState.xml"),
        "IsPass1": is_pass1,
        "Language": "CppWinRT",
        "ProjectPath": native_path(absolute_path(app_xaml)),
        "LanguageSourceExtension": ".cpp",
        "OutputPath": native_path(generated_dir),
        "RootNamespace": ROOT_NAMESPACE,
        "PrecompiledHeaderFile": "pch.h",
        "FeatureControlFlags": FEATURE_CONTROL_FLAGS,
        "ReferenceAssemblies": [msbuild_item(path) for path in ref_winmds],
        "ReferenceAssemblyPaths": [],
        "TargetPlatformMinVersion": winsdk_version,
        "XamlPages": [msbuild_item(main_window_xaml)],
        "XamlApplications": [msbuild_item(app_xaml)],
        "ClIncludeFiles": [
            msbuild_item(app_header, dependent_upon=app_xaml),
            msbuild_item(main_window_header, dependent_upon=main_window_xaml),
        ],
    }

    if not is_pass1:
        data["LocalAssembly"] = [msbuild_item(merged_winmd)]
        data["GenXbfPath"] = native_path(genxbf_path)

    return data


def write_text(path: Path, content: str) -> None:
    try:
        path.write_text(content, encoding="utf-8", newline="\n")
    except OSError as exc:
        raise BuildError(f"failed to write {native_path(path)}: {exc}") from exc


def write_json(path: Path, data: dict[str, Any]) -> None:
    write_text(path, json.dumps(data, indent=2) + "\n")


def generate_xaml_metadata_provider(generated_dir: Path) -> None:
    write_text(
        generated_dir / "XamlMetaDataProvider.idl",
        "namespace xmake_demo\n"
        "{\n"
        "    runtimeclass XamlMetaDataProvider : [default] Microsoft.UI.Xaml.Markup.IXamlMetadataProvider\n"
        "    {\n"
        "        XamlMetaDataProvider();\n"
        "    }\n"
        "}\n",
    )
    write_text(
        generated_dir / "XamlMetaDataProvider.cpp",
        '#include "pch.h"\n'
        '#include "XamlMetaDataProvider.h"\n'
        '#include "XamlMetaDataProvider.g.cpp"\n',
    )


def run_midl(
    *,
    midl_exe: Path,
    idl: Path,
    out_winmd: Path,
    foundation_meta: Path,
    sdk_include_dir: Path,
    ref_winmds: list[Path],
    env: dict[str, str],
) -> None:
    command = [
        native_path(midl_exe),
        native_path(idl),
        "/nologo",
        "/winrt",
        "/winmd",
        native_path(out_winmd),
        "/nomidl",
        "/h",
        "nul",
        "/metadata_dir",
        native_path(foundation_meta),
        "/I",
        native_path(sdk_include_dir / "um"),
        "/I",
        native_path(sdk_include_dir / "shared"),
        "/I",
        native_path(sdk_include_dir / "winrt"),
    ]
    for ref_winmd in ref_winmds:
        command.extend(["/reference", native_path(ref_winmd)])
    run_command(command, env=env)


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Build WinUI 3 generated C++/WinRT and XAML artifacts.")
    parser.add_argument("--build-dir", type=Path, required=True, help="Build output directory.")
    parser.add_argument(
        "--project-dir",
        type=Path,
        default=Path(__file__).resolve().parent.parent,
        help="Project root directory. Defaults to this script's parent directory.",
    )
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(sys.argv[1:] if argv is None else argv)

    try:
        project_dir = absolute_path(args.project_dir)
        build_dir = absolute_path(args.build_dir)
        generated_dir = build_dir / "generated"
        generated_sources_dir = generated_dir / "sources"
        winmd_unmerged_dir = build_dir / "winmd_unmerged"
        winmd_merged_dir = build_dir / "winmd_merged"
        merged_winmd = winmd_merged_dir / f"{ROOT_NAMESPACE}.winmd"

        root = nuget_root()
        # WinAppSDK 1.8+ sub-packages (latest stable, April 2026)
        foundation_pkg = root / "microsoft.windowsappsdk.foundation" / "1.8.260415000"
        winui_pkg = root / "microsoft.windowsappsdk.winui" / "1.8.260415005"
        ixp_pkg = root / "microsoft.windowsappsdk.interactiveexperiences" / "1.8.260415001"
        cppwinrt_exe = root / "microsoft.windows.cppwinrt" / "2.0.250303.1" / "bin" / "cppwinrt.exe"
        buildtools_bin = (
            root
            / "microsoft.windows.sdk.buildtools"
            / "10.0.28000.1721"
            / "bin"
            / BUILD_TOOLS_BIN_VERSION
            / "x64"
        )
        midl_exe = buildtools_bin / "midl.exe"
        mdmerge_exe = buildtools_bin / "mdmerge.exe"
        xaml_compiler = winui_pkg / "tools" / "net472" / "XamlCompiler.exe"
        genxbf_dir = winui_pkg / "tools"  # XamlCompiler finds GenXbf.dll in arch subdirectories
        webview2_winmd = (
            root / "microsoft.web.webview2" / "1.0.3912.50" / "lib" / "Microsoft.Web.WebView2.Core.winmd"
        )

        require_dir(project_dir / "src", "project src directory")
        for source in [
            project_dir / "src" / "App.xaml",
            project_dir / "src" / "App.xaml.h",
            project_dir / "src" / "MainWindow.xaml",
            project_dir / "src" / "MainWindow.xaml.h",
            project_dir / "src" / "MainWindow.idl",
            project_dir / "src" / "XamlMetaDataProvider.idl",
        ]:
            require_file(source, "project source file")

        require_file(cppwinrt_exe, "cppwinrt.exe")
        require_file(midl_exe, "midl.exe")
        require_file(mdmerge_exe, "mdmerge.exe")
        require_file(xaml_compiler, "XamlCompiler.exe")
        require_file(webview2_winmd, "WebView2 WinMD")
        require_dir(foundation_pkg, "Foundation sub-package")
        require_dir(winui_pkg, "WinUI sub-package")
        require_dir(ixp_pkg, "InteractiveExperiences sub-package")
        require_dir(genxbf_dir, "GenXbf directory")
        require_dir(vc_bin_path(), "VC tools bin directory")

        winsdk_root = require_dir(WINDOWS_SDK_ROOT, "Windows SDK root")
        winsdk_version, _ = platform_xml_path(winsdk_root, WINDOWS_SDK_VERSION)
        platform_winmds = collect_platform_winmds(winsdk_root, winsdk_version)
        appsdk_winmds = collect_appsdk_winmds(foundation_pkg, winui_pkg, ixp_pkg)
        ref_winmds = [*platform_winmds, webview2_winmd, *appsdk_winmds]
        foundation_meta = find_foundation_metadata_dir(winsdk_root, winsdk_version)
        sdk_include_dir = require_dir(
            winsdk_root / "Include" / winsdk_version,
            "Windows SDK include directory",
        )
        for include_name in ["um", "shared", "winrt"]:
            require_dir(sdk_include_dir / include_name, f"Windows SDK {include_name} include directory")

        for directory in [generated_dir, generated_sources_dir, winmd_unmerged_dir, winmd_merged_dir]:
            directory.mkdir(parents=True, exist_ok=True)

        print(f"Project: {native_path(project_dir)}")
        print(f"Build:   {native_path(build_dir)}")
        print(f"WinSDK:  {native_path(winsdk_root)} ({winsdk_version})")
        print(f"Refs:    {len(platform_winmds)} platform, {len(appsdk_winmds)} WinAppSDK, 1 WebView2")

        print_phase("[1/8] Generate platform, WebView2, and WinAppSDK headers")
        phase1a = [native_path(cppwinrt_exe)]
        for winmd in platform_winmds:
            phase1a.extend(["-in", native_path(winmd)])
        phase1a.extend(["-out", native_path(generated_dir)])
        run_command(phase1a)

        phase1b = [native_path(cppwinrt_exe), "-in", native_path(webview2_winmd)]
        for winmd in platform_winmds:
            phase1b.extend(["-ref", native_path(winmd)])
        phase1b.extend(["-out", native_path(generated_dir)])
        run_command(phase1b)

        phase1c = [native_path(cppwinrt_exe)]
        for winmd in appsdk_winmds:
            phase1c.extend(["-in", native_path(winmd)])
        for winmd in platform_winmds:
            phase1c.extend(["-ref", native_path(winmd)])
        phase1c.extend(["-ref", native_path(webview2_winmd), "-out", native_path(generated_dir)])
        run_command(phase1c)

        print_phase("[2/8] Compile IDL to WinMD")
        midl_env = path_env_with_vc(vc_bin_path())
        run_midl(
            midl_exe=midl_exe,
            idl=project_dir / "src" / "XamlMetaDataProvider.idl",
            out_winmd=winmd_unmerged_dir / "XamlMetaDataProvider.winmd",
            foundation_meta=foundation_meta,
            sdk_include_dir=sdk_include_dir,
            ref_winmds=ref_winmds,
            env=midl_env,
        )
        run_midl(
            midl_exe=midl_exe,
            idl=project_dir / "src" / "MainWindow.idl",
            out_winmd=winmd_unmerged_dir / "MainWindow.winmd",
            foundation_meta=foundation_meta,
            sdk_include_dir=sdk_include_dir,
            ref_winmds=ref_winmds,
            env=midl_env,
        )

        print_phase("[3/8] Merge WinMDs")
        mdmerge = [native_path(mdmerge_exe), "-o", native_path(winmd_merged_dir)]
        for directory in unique_parent_dirs(ref_winmds):
            mdmerge.extend(["-metadata_dir", native_path(directory)])
        mdmerge.extend(
            [
                "-i",
                native_path(winmd_unmerged_dir / "XamlMetaDataProvider.winmd"),
                "-i",
                native_path(winmd_unmerged_dir / "MainWindow.winmd"),
                "-partial",
                "-n:1",
            ]
        )
        run_command(mdmerge)
        require_file(merged_winmd, "merged WinMD")

        print_phase("[4/8] Generate project C++/WinRT sources")
        cppwinrt_project = [
            native_path(cppwinrt_exe),
            "-in",
            native_path(merged_winmd),
            "-out",
            native_path(generated_dir),
            "-comp",
            native_path(generated_sources_dir),
            "-name",
            ROOT_NAMESPACE,
            "-pch",
            "pch.h",
            "-prefix",
            "-optimize",
            "-overwrite",
        ]
        for winmd in ref_winmds:
            cppwinrt_project.extend(["-ref", native_path(winmd)])
        run_command(cppwinrt_project)

        print_phase("[5/8] Generate XamlMetaDataProvider source")
        generate_xaml_metadata_provider(generated_dir)

        print_phase("[6/8] Run XAML compiler pass 1")
        pass1_json = generated_dir / "xaml.pass1.in.json"
        pass1_out = generated_dir / "xaml.pass1.out.json"
        write_json(
            pass1_json,
            build_xaml_json(
                generated_dir=generated_dir,
                project_dir=project_dir,
                winsdk_version=winsdk_version,
                ref_winmds=ref_winmds,
                merged_winmd=merged_winmd,
                genxbf_path=genxbf_dir,
                is_pass1=True,
            ),
        )
        run_command([native_path(xaml_compiler), native_path(pass1_json), native_path(pass1_out)])

        print_phase("[7/8] Run XAML compiler pass 2")
        pass2_json = generated_dir / "xaml.pass2.in.json"
        pass2_out = generated_dir / "xaml.pass2.out.json"
        write_json(
            pass2_json,
            build_xaml_json(
                generated_dir=generated_dir,
                project_dir=project_dir,
                winsdk_version=winsdk_version,
                ref_winmds=ref_winmds,
                merged_winmd=merged_winmd,
                genxbf_path=genxbf_dir,
                is_pass1=False,
            ),
        )
        run_command([native_path(xaml_compiler), native_path(pass2_json), native_path(pass2_out)])

        # Phase 8: Generate .pri resource file (makepri)
        print_phase("[8/8] Generate .pri resource index")
        makepri_exe = buildtools_bin / "makepri.exe"
        if not makepri_exe.is_file():
            raise BuildError(f"makepri.exe not found at {makepri_exe}")
        
        # Collect .xbf files generated by XamlCompiler
        xbf_files = sorted(generated_dir.glob("*.xbf"))
        if not xbf_files:
            raise BuildError("No .xbf files found - XAML compilation may have failed")
        
        # Write resfiles list (full paths to .xbf files)
        resfiles_path = generated_dir / "layout.resfiles"
        resfiles_content = "\n".join(str(f) for f in xbf_files)
        resfiles_path.write_text(resfiles_content, encoding="utf-8")
        
        # Write priconfig.xml
        priconfig_path = generated_dir / "priconfig.xml"
        priconfig_xml = f"""<?xml version="1.0" encoding="utf-8"?>
<resources targetOsVersion="10.0.0" majorVersion="1">
  <index root="{generated_dir}" startIndexAt="{resfiles_path}">
    <default>
      <qualifier name="Language" value="en-US"/>
      <qualifier name="Contrast" value="standard"/>
      <qualifier name="Scale" value="200"/>
      <qualifier name="HomeRegion" value="001"/>
      <qualifier name="TargetSize" value="256"/>
      <qualifier name="LayoutDirection" value="LTR"/>
      <qualifier name="DXFeatureLevel" value="DX9"/>
      <qualifier name="Configuration" value=""/>
      <qualifier name="AlternateForm" value=""/>
      <qualifier name="Platform" value="UAP"/>
    </default>
    <indexer-config type="RESFILES" qualifierDelimiter="."/>
    <indexer-config type="EMBEDFILES"/>
  </index>
</resources>"""
        priconfig_path.write_text(priconfig_xml, encoding="utf-8")
        
        pri_output = generated_dir / "resources.pri"
        run_command([
            native_path(makepri_exe), "new",
            "/cf", native_path(priconfig_path),
            "/pr", native_path(project_dir),
            "/o",
            "/of", native_path(pri_output),
        ])

        print("\n=== WinUI 3 code generation complete ===", flush=True)
    except BuildError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1
    except subprocess.CalledProcessError:
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
