#!/usr/bin/env python3
"""Restore NuGet packages and write a package-id to install-path map."""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
import xml.etree.ElementTree as ET
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True, slots=True)
class PackageSpec:
    package_id: str
    version: str


class RestoreError(RuntimeError):
    """Raised for user-facing restore failures."""


def default_project_root() -> Path:
    return Path(__file__).resolve().parent.parent


def parse_args() -> argparse.Namespace:
    project_root = default_project_root()

    parser = argparse.ArgumentParser(
        description="Restore packages.config with nuget.exe and emit a JSON package path map.",
    )
    parser.add_argument(
        "--packages-config",
        type=Path,
        default=project_root / "packages.config",
        help="Path to packages.config (default: project-root/packages.config).",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=project_root / "packages",
        help="Directory where NuGet packages are installed (default: project-root/packages).",
    )
    parser.add_argument(
        "--map-file",
        type=Path,
        default=project_root / "packages" / "packages_map.json",
        help="JSON output file for package path map (default: project-root/packages/packages_map.json).",
    )
    return parser.parse_args()


def normalize_path(path: Path) -> str:
    return path.resolve().as_posix()


def local_name(tag: str) -> str:
    return tag.rsplit("}", 1)[-1]


def parse_packages_config(packages_config: Path) -> list[PackageSpec]:
    if not packages_config.is_file():
        raise RestoreError(f"packages.config not found: {packages_config}")

    try:
        tree = ET.parse(packages_config)
    except ET.ParseError as exc:
        raise RestoreError(f"Failed to parse XML in {packages_config}: {exc}") from exc

    packages: list[PackageSpec] = []
    for element in tree.getroot().iter():
        if local_name(element.tag) != "package":
            continue

        package_id = element.get("id")
        version = element.get("version")
        if not package_id or not version:
            raise RestoreError(
                f"Invalid package entry in {packages_config}: each package needs id and version."
            )

        packages.append(PackageSpec(package_id=package_id, version=version))

    if not packages:
        raise RestoreError(f"No packages found in {packages_config}")

    return packages


def run_nuget_install(packages_config: Path, output_dir: Path) -> None:
    command = [
        "nuget",
        "install",
        str(packages_config),
        "-OutputDirectory",
        str(output_dir),
        "-NonInteractive",
    ]

    print("Running:", " ".join(command))
    try:
        completed = subprocess.run(command, check=False)
    except FileNotFoundError as exc:
        raise RestoreError("nuget executable was not found on PATH.") from exc

    if completed.returncode != 0:
        raise RestoreError(f"nuget install failed with exit code {completed.returncode}.")


def find_package_dir(output_dir: Path, package: PackageSpec) -> Path:
    exact_candidates = [
        output_dir / f"{package.package_id}.{package.version}",
        output_dir / package.package_id / package.version,
    ]
    for candidate in exact_candidates:
        if candidate.is_dir():
            return candidate

    lower_exact = f"{package.package_id}.{package.version}".lower()
    lower_package_id = package.package_id.lower()
    lower_version = package.version.lower()

    if not output_dir.is_dir():
        raise RestoreError(f"NuGet output directory does not exist: {output_dir}")

    for candidate in output_dir.iterdir():
        if not candidate.is_dir():
            continue

        candidate_name = candidate.name.lower()
        if candidate_name == lower_exact:
            return candidate

        version_subdir = candidate / package.version
        if candidate_name == lower_package_id and version_subdir.is_dir():
            return version_subdir

        if candidate_name.startswith(f"{lower_package_id}.") and candidate_name.endswith(
            f".{lower_version}"
        ):
            return candidate

    raise RestoreError(
        f"Installed directory for {package.package_id} {package.version} was not found in {output_dir}."
    )


def write_package_map(map_file: Path, package_map: dict[str, str]) -> None:
    map_file.parent.mkdir(parents=True, exist_ok=True)
    with map_file.open("w", encoding="utf-8") as file:
        json.dump(package_map, file, indent=2, sort_keys=True)
        file.write("\n")


def main() -> int:
    args = parse_args()
    packages_config = args.packages_config.resolve()
    output_dir = args.output_dir.resolve()
    map_file = args.map_file.resolve()

    try:
        print(f"Reading packages from {packages_config}")
        packages = parse_packages_config(packages_config)
        print(f"Found {len(packages)} package(s).")

        print(f"Ensuring output directory exists: {output_dir}")
        output_dir.mkdir(parents=True, exist_ok=True)

        run_nuget_install(packages_config, output_dir)

        print("Locating installed package directories...")
        package_map: dict[str, str] = {}
        for package in packages:
            package_dir = find_package_dir(output_dir, package)
            package_map[package.package_id] = normalize_path(package_dir)
            print(f"  {package.package_id} -> {package_map[package.package_id]}")

        write_package_map(map_file, package_map)
        print(f"Wrote package map: {normalize_path(map_file)}")
        return 0
    except RestoreError as exc:
        print(f"Error: {exc}", file=sys.stderr)
        return 1
    except OSError as exc:
        print(f"Error: file system operation failed: {exc}", file=sys.stderr)
        return 1
    except KeyboardInterrupt:
        print("Error: interrupted.", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
