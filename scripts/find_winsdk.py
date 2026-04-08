#!/usr/bin/env python3
"""Discover Windows SDK platform WinMD references."""

from __future__ import annotations

import argparse
import json
import sys
import xml.etree.ElementTree as ET
from pathlib import Path
from typing import Any


REGISTRY_SUBKEY = r"SOFTWARE\Microsoft\Windows Kits\Installed Roots"
REGISTRY_VALUE = "KitsRoot10"


def normalize_path(path: Path) -> str:
    return path.as_posix()


def parse_version(version: str) -> tuple[int, ...]:
    parts: list[int] = []
    for part in version.split("."):
        try:
            parts.append(int(part))
        except ValueError:
            parts.append(-1)
    return tuple(parts)


def read_winsdk_root_from_registry() -> Path:
    try:
        import winreg
    except ImportError as exc:
        raise RuntimeError(
            "winreg is not available; pass --winsdk-root explicitly on non-Windows Python."
        ) from exc

    try:
        with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, REGISTRY_SUBKEY) as key:
            value, _ = winreg.QueryValueEx(key, REGISTRY_VALUE)
    except OSError as exc:
        raise RuntimeError(
            f"Failed to read Windows SDK root from registry value "
            f"HKLM\\{REGISTRY_SUBKEY}\\{REGISTRY_VALUE}: {exc}"
        ) from exc

    if not isinstance(value, str) or not value.strip():
        raise RuntimeError(
            f"Registry value HKLM\\{REGISTRY_SUBKEY}\\{REGISTRY_VALUE} is empty or invalid."
        )

    return Path(value)


def discover_highest_winsdk_version(winsdk_root: Path) -> str:
    uap_dir = winsdk_root / "Platforms" / "UAP"
    if not uap_dir.is_dir():
        raise RuntimeError(f"Windows SDK UAP platform directory does not exist: {uap_dir}")

    versions = [path.name for path in uap_dir.iterdir() if path.is_dir()]
    if not versions:
        raise RuntimeError(f"No SDK versions found under: {uap_dir}")

    return max(versions, key=parse_version)


def parse_platform_contracts(platform_xml: Path) -> list[tuple[str, str]]:
    if not platform_xml.is_file():
        raise RuntimeError(f"Platform.xml does not exist: {platform_xml}")

    try:
        root = ET.parse(platform_xml).getroot()
    except ET.ParseError as exc:
        raise RuntimeError(f"Failed to parse Platform.xml at {platform_xml}: {exc}") from exc
    except OSError as exc:
        raise RuntimeError(f"Failed to read Platform.xml at {platform_xml}: {exc}") from exc

    contracts: list[tuple[str, str]] = []
    for element in root.iter():
        if element.tag.rsplit("}", 1)[-1] != "ApiContract":
            continue

        name = element.attrib.get("name")
        version = element.attrib.get("version")
        if not name or not version:
            raise RuntimeError(
                f"ApiContract in {platform_xml} is missing required name or version attribute."
            )

        contracts.append((name, version))

    if not contracts:
        raise RuntimeError(f"No ApiContract entries found in: {platform_xml}")

    return contracts


def resolve_platform_winmds(
    winsdk_root: Path, winsdk_version: str, contracts: list[tuple[str, str]]
) -> list[Path]:
    winmds: list[Path] = []
    for name, contract_version in contracts:
        winmds.append(
            winsdk_root
            / "References"
            / winsdk_version
            / name
            / contract_version
            / f"{name}.winmd"
        )

    return winmds


def build_result(winsdk_root: Path, winsdk_version: str) -> dict[str, Any]:
    platform_xml = winsdk_root / "Platforms" / "UAP" / winsdk_version / "Platform.xml"
    contracts = parse_platform_contracts(platform_xml)
    winmds = resolve_platform_winmds(winsdk_root, winsdk_version, contracts)

    return {
        "winsdk_root": normalize_path(winsdk_root),
        "winsdk_version": winsdk_version,
        "platform_winmds": [normalize_path(path) for path in winmds],
    }


def write_json(result: dict[str, Any], output: Path | None) -> None:
    text = json.dumps(result, indent=2) + "\n"
    if output is None:
        sys.stdout.write(text)
        return

    try:
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(text, encoding="utf-8")
    except OSError as exc:
        raise RuntimeError(f"Failed to write output JSON to {output}: {exc}") from exc


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Discover Windows SDK platform .winmd references."
    )
    parser.add_argument(
        "--winsdk-root",
        type=Path,
        help="Windows 10/11 SDK root. Skips registry lookup when provided.",
    )
    parser.add_argument(
        "--winsdk-version",
        help="Windows SDK version, such as 10.0.22621.0. Defaults to highest UAP version.",
    )
    parser.add_argument(
        "--output",
        type=Path,
        help="Output JSON file path. Defaults to stdout.",
    )
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(sys.argv[1:] if argv is None else argv)

    try:
        winsdk_root = args.winsdk_root if args.winsdk_root else read_winsdk_root_from_registry()
        winsdk_version = args.winsdk_version or discover_highest_winsdk_version(winsdk_root)
        result = build_result(winsdk_root, winsdk_version)
        write_json(result, args.output)
    except RuntimeError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
