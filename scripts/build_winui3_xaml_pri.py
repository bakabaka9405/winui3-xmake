#!/usr/bin/env python3
"""XAML/PRI build entry: executes phases 6-8 (XAML Pass 1, Pass 2, makepri)."""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path

from build_winui3_common import (
    BuildError,
    BuildLayout,
    ResolvedInputs,
    resolve_layout,
    resolve_inputs,
    set_verbose,
    print_phase,
    build_xaml_json,
    write_json,
    write_text,
    run_command,
    clean_stale_files,
)
from plat_info import require_file

__all__ = ["main"]


def parse_args(argv: list[str]) -> argparse.Namespace:
    """Parse CLI arguments for XAML/PRI build entry."""
    parser = argparse.ArgumentParser(
        description="Build WinUI 3 XAML and PRI artifacts (XAML Pass 1, Pass 2, makepri)."
    )
    parser.add_argument("--build-dir", type=Path, required=True, help="Build output directory.")
    parser.add_argument("--project-dir", type=Path, default=Path("."), help="Project root directory.")
    parser.add_argument("-v", "--verbose", action="store_true", default=False, help="Enable verbose output.")
    parser.add_argument("--namespace", type=str, default="xmake_demo", help="Root namespace.")
    parser.add_argument("--src-dir", type=str, default=None, help="Source directory containing .xaml files.")
    parser.add_argument("--shared-projection-dir", type=Path, default=None, help="Shared output directory for projection headers.")
    parser.add_argument("--xaml-compiler-path", type=Path, default=None, help="Custom directory containing XamlCompiler.exe.")
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    """Execute phases 6-8 (XAML Pass 1, Pass 2, makepri)."""
    args = parse_args(sys.argv[1:] if argv is None else argv)
    set_verbose(args.verbose)

    try:
        # Step 1: Resolve layout and inputs (via common module)
        layout: BuildLayout = resolve_layout(args)
        inputs: ResolvedInputs = resolve_inputs(args, layout)

        # Print build summary
        print(f"Project: {layout.project_dir}")
        print(f"Build: {layout.build_dir}")
        print(f"Shared: {layout.shared_projection_dir}")
        print(f"WinSDK: {inputs.winsdk_version}")
        print(f"Refs: {len(inputs.ref_winmds)}")

        # ---- Phase 6: XAML Compiler Pass 1 ----
        print_phase("[6/8] Run XAML compiler pass 1")
        clean_stale_files(layout.generated_dir, "**/*.xbf")

        pass1_json = layout.generated_dir / "xaml.pass1.in.json"
        pass1_out = layout.generated_dir / "xaml.pass1.out.json"
        write_json(
            pass1_json,
            build_xaml_json(
                generated_dir=layout.generated_dir,
                src_dir=layout.src_dir,
                namespace=args.namespace,
                winsdk_version=inputs.winsdk_version,
                ref_winmds=inputs.ref_winmds,
                merged_winmd=layout.merged_winmd,
                genxbf_path=inputs.genxbf_dir,
                is_pass1=True,
            ),
        )
        run_command([str(inputs.xaml_compiler), str(pass1_json), str(pass1_out)])

        # ---- Phase 7: XAML Compiler Pass 2 ----
        # Validate merged WinMD exists (required by Pass 2)
        if not layout.merged_winmd.is_file():
            raise BuildError(
                f"Phase 7 (xaml-pass2) requires merged WinMD at "
                f"{str(layout.merged_winmd)}, but it does not exist"
            )

        print_phase("[7/8] Run XAML compiler pass 2")
        pass2_json = layout.generated_dir / "xaml.pass2.in.json"
        pass2_out = layout.generated_dir / "xaml.pass2.out.json"
        write_json(
            pass2_json,
            build_xaml_json(
                generated_dir=layout.generated_dir,
                src_dir=layout.src_dir,
                namespace=args.namespace,
                winsdk_version=inputs.winsdk_version,
                ref_winmds=inputs.ref_winmds,
                merged_winmd=layout.merged_winmd,
                genxbf_path=inputs.genxbf_dir,
                is_pass1=False,
            ),
        )
        run_command([str(inputs.xaml_compiler), str(pass2_json), str(pass2_out)])

        # ---- Phase 8: Generate .pri resource index (makepri) ----
        # Validate .xbf files exist (required by makepri)
        xbf_files = sorted(layout.generated_dir.rglob("*.xbf"))
        if len(xbf_files) == 0:
            raise BuildError("No .xbf files found - XAML compilation may have failed")

        print_phase("[8/8] Generate .pri resource index")
        makepri_exe = require_file(inputs.buildtools_bin / "makepri.exe", "makepri.exe")

        # Write layout.resfiles (list of .xbf files)
        resfiles_path = layout.generated_dir / "layout.resfiles"
        resfiles_content = "\n".join(str(f) for f in xbf_files)
        write_text(resfiles_path, resfiles_content)

        # Write priconfig.xml
        priconfig_path = layout.generated_dir / "priconfig.xml"
        priconfig_xml = f"""<?xml version="1.0" encoding="utf-8"?>
<resources targetOsVersion="10.0.0" majorVersion="1">
  <index root="{layout.generated_dir}" startIndexAt="{resfiles_path}">
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
        write_text(priconfig_path, priconfig_xml)

        pri_output = layout.generated_dir / "resources.pri"
        run_command([
            str(makepri_exe), "new",
            "/cf", str(priconfig_path),
            "/pr", str(layout.project_dir),
            "/o",
            "/of", str(pri_output),
        ])

        print("\n=== WinUI 3 XAML/PRI generation complete ===", flush=True)
    except BuildError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1
    except subprocess.CalledProcessError:
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
