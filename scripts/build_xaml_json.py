#!/usr/bin/env python3
"""Generate XamlCompiler.exe input JSON files for WinUI 3 XAML compilation."""

from __future__ import annotations

import argparse
import json
import os.path
import sys
from pathlib import Path
from typing import Any


FEATURE_CONTROL_FLAGS = (
    "EnableXBindDiagnostics;"
    "EnableDefaultValidationContextGeneration;"
    "EnableWin32Codegen"
)


class XamlJsonError(RuntimeError):
    """Raised for user-facing JSON generation failures."""


def native_path(path: str | Path) -> str:
    """Return a normalized Windows-native path string."""

    normalized = os.path.normpath(os.fspath(path))
    return normalized.replace("/", "\\")


def make_msbuild_item(path: str | Path, dependent_upon: str | Path | None = None) -> dict[str, str]:
    native = native_path(path)
    item = {
        "ItemSpec": native,
        "FullPath": native,
    }
    if dependent_upon is not None:
        item["DependentUpon"] = native_path(dependent_upon)

    return item


def xaml_header_path(xaml_path: Path) -> Path:
    return Path(f"{os.fspath(xaml_path)}.h")


def build_common_json(args: argparse.Namespace, output_dir: Path, project_path: Path) -> dict[str, Any]:
    xaml_apps: list[Path] = args.xaml_apps
    xaml_pages: list[Path] = args.xaml_pages
    xaml_files = [*xaml_apps, *xaml_pages]

    return {
        "SavedStateFile": native_path(output_dir / "XamlCompilerState.xml"),
        "IsPass1": "true",
        "Language": "CppWinRT",
        "ProjectPath": native_path(project_path),
        "LanguageSourceExtension": ".cpp",
        "OutputPath": native_path(output_dir),
        "RootNamespace": args.namespace,
        "PrecompiledHeaderFile": args.pch,
        "FeatureControlFlags": FEATURE_CONTROL_FLAGS,
        "ReferenceAssemblies": [make_msbuild_item(path) for path in args.ref_winmd],
        "ReferenceAssemblyPaths": "[]",
        "TargetPlatformMinVersion": args.winsdk_version,
        "XamlPages": [make_msbuild_item(path) for path in xaml_pages],
        "XamlApplications": [make_msbuild_item(path) for path in xaml_apps],
        "ClIncludeFiles": [
            make_msbuild_item(xaml_header_path(path), dependent_upon=path) for path in xaml_files
        ],
    }


def build_pass_json(args: argparse.Namespace, output_dir: Path, project_path: Path) -> tuple[dict[str, Any], dict[str, Any]]:
    pass1 = build_common_json(args, output_dir, project_path)
    pass2 = dict(pass1)
    pass2["IsPass1"] = "false"
    pass2["LocalAssembly"] = [make_msbuild_item(args.merged_winmd)]
    pass2["GenXbfPath"] = native_path(args.genxbf_path)

    return pass1, pass2


def output_json_paths(output_dir: Path, json_out: str) -> tuple[Path, Path]:
    if not json_out.strip():
        raise XamlJsonError("--json-out must not be empty.")

    base_name = Path(json_out).name
    if base_name != json_out:
        raise XamlJsonError("--json-out must be a base file name, not a path.")

    return output_dir / f"{base_name}.pass1.in.json", output_dir / f"{base_name}.pass2.in.json"


def write_json(path: Path, data: dict[str, Any]) -> None:
    try:
        path.write_text(json.dumps(data, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    except OSError as exc:
        raise XamlJsonError(f"failed to write {path}: {exc}") from exc


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate pass 1 and pass 2 XamlCompiler.exe input JSON files.",
    )
    parser.add_argument("--output-dir", type=Path, required=True, help="Output directory for generated files.")
    parser.add_argument("--namespace", required=True, help="XAML root namespace.")
    parser.add_argument("--pch", required=True, help="Precompiled header file name.")
    parser.add_argument("--winsdk-version", required=True, help="Windows SDK version.")
    parser.add_argument("--merged-winmd", type=Path, required=True, help="Path to the merged .winmd file.")
    parser.add_argument(
        "--ref-winmd",
        type=Path,
        action="append",
        default=[],
        help="Reference .winmd path. Can be specified multiple times.",
    )
    parser.add_argument(
        "--xaml-apps",
        type=Path,
        action="append",
        default=[],
        help="Application .xaml file. Can be specified multiple times.",
    )
    parser.add_argument(
        "--xaml-pages",
        type=Path,
        action="append",
        default=[],
        help="Page .xaml file. Can be specified multiple times.",
    )
    parser.add_argument("--xaml-compiler", type=Path, required=True, help="Path to XamlCompiler.exe.")
    parser.add_argument("--genxbf-path", type=Path, required=True, help="Directory containing genxbf.")
    parser.add_argument(
        "--project-path",
        type=Path,
        help="Project file path used by XamlCompiler for relative path computation. Defaults to <output-dir>/dummy.vcxproj.",
    )
    parser.add_argument("--json-out", required=True, help="Base name for generated JSON files.")
    return parser.parse_args(argv)


def validate_args(args: argparse.Namespace) -> None:
    if not args.xaml_apps and not args.xaml_pages:
        raise XamlJsonError("at least one --xaml-apps or --xaml-pages value is required.")

    if args.xaml_compiler.name.lower() != "xamlcompiler.exe":
        raise XamlJsonError(f"--xaml-compiler must point to XamlCompiler.exe: {args.xaml_compiler}")

    for attr in ("namespace", "pch", "winsdk_version"):
        if not getattr(args, attr).strip():
            raise XamlJsonError(f"--{attr.replace('_', '-')} must not be empty.")


def main(argv: list[str] | None = None) -> int:
    args = parse_args(sys.argv[1:] if argv is None else argv)

    try:
        validate_args(args)

        output_dir = args.output_dir
        project_path = args.project_path if args.project_path is not None else output_dir / "dummy.vcxproj"
        pass1_path, pass2_path = output_json_paths(output_dir, args.json_out)

        output_dir.mkdir(parents=True, exist_ok=True)

        pass1_json, pass2_json = build_pass_json(args, output_dir, project_path)
        write_json(pass1_path, pass1_json)
        write_json(pass2_path, pass2_json)

        print(f"Wrote {native_path(pass1_path)}")
        print(f"Wrote {native_path(pass2_path)}")
    except XamlJsonError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1
    except OSError as exc:
        print(f"error: file system operation failed: {exc}", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
