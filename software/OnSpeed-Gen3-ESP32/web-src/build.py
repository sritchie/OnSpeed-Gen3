#!/usr/bin/env python3
"""
OnSpeed Web UI Build Tool

This script manages the web UI source files and their conversion to C++ headers.

Commands:
    extract  - Extract content from existing .h files to editable source files
    build    - Convert source files back to .h headers for ESP32 compilation

Usage:
    python build.py extract   # One-time: extract from existing .h files
    python build.py build     # After editing: regenerate .h files
"""

import argparse
import re
import sys
from pathlib import Path


def extract_content_from_header(header_path: Path) -> str:
    """Extract the raw string content from a C++ PROGMEM header file."""
    content = header_path.read_text()

    # Match content between R"=====( and )====="
    match = re.search(r'R"=====\((.*?)\)====="', content, re.DOTALL)
    if match:
        return match.group(1)

    return ""


def wrap_in_header(content: str, var_name: str) -> str:
    """Wrap content in C++ PROGMEM header format."""
    return f'const char {var_name}[] PROGMEM = R"=====({content})=====";\n'


def extract_command(web_dir: Path, src_dir: Path):
    """Extract content from existing .h files to source files."""
    print("Extracting web content from .h files...")

    # Create source directories
    (src_dir / "css").mkdir(parents=True, exist_ok=True)
    (src_dir / "js").mkdir(parents=True, exist_ok=True)
    (src_dir / "pages").mkdir(parents=True, exist_ok=True)

    # Map of header files to extract
    extractions = [
        ("html_header.h", "pages/header.html"),
        ("html_liveview.h", "pages/liveview.html"),
        ("html_calibration.h", "pages/calibration.html"),
        ("css_chartist.h", "css/chartist.css"),
        ("javascript_calibration.h", "js/calibration.js"),
        ("javascript_chartist1.h", "js/chartist1.js"),
        ("javascript_chartist2.h", "js/chartist2.js"),
        ("javascript_regression.h", "js/regression.js"),
        ("sg_filter.h", "js/sg_filter.js"),
    ]

    for header_name, src_name in extractions:
        header_path = web_dir / header_name
        src_path = src_dir / src_name

        if header_path.exists():
            content = extract_content_from_header(header_path)
            if content:
                src_path.write_text(content)
                print(f"  {header_name} -> {src_name}")
            else:
                print(f"  {header_name} -> (empty or parse failed)")
        else:
            print(f"  {header_name} -> (not found)")

    print("\nExtraction complete!")
    print("You can now edit files in web-src/ and run 'python build.py build' to regenerate headers.")


def build_command(web_dir: Path, src_dir: Path):
    """Convert source files to .h headers."""
    print("Building .h headers from source files...")

    # Map of source files to headers with their C variable names
    builds = [
        ("pages/header.html", "html_header.h", "szHtmlHeader"),
        ("pages/liveview.html", "html_liveview.h", "htmlLiveView"),
        ("pages/calibration.html", "html_calibration.h", "htmlCalibration"),
        ("css/chartist.css", "css_chartist.h", "cssChartist"),
        ("js/calibration.js", "javascript_calibration.h", "jsCalibration"),
        ("js/chartist1.js", "javascript_chartist1.h", "jsChartist1"),
        ("js/chartist2.js", "javascript_chartist2.h", "jsChartist2"),
        ("js/regression.js", "javascript_regression.h", "jsRegression"),
        ("js/sg_filter.js", "sg_filter.h", "jsSGFilter"),
    ]

    for src_name, header_name, var_name in builds:
        src_path = src_dir / src_name
        header_path = web_dir / header_name

        if src_path.exists():
            content = src_path.read_text()
            header_content = wrap_in_header(content, var_name)
            header_path.write_text(header_content)
            print(f"  {src_name} -> {header_name}")
        else:
            print(f"  {src_name} -> (source not found, skipping)")

    print("\nBuild complete!")
    print("Header files updated in Web/")


def main():
    parser = argparse.ArgumentParser(
        description="OnSpeed Web UI Build Tool",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
    python build.py extract   # Extract .h files to editable sources
    python build.py build     # Regenerate .h files from sources
        """
    )
    parser.add_argument(
        "command",
        choices=["extract", "build"],
        help="Command to run"
    )
    args = parser.parse_args()

    # Determine paths
    script_dir = Path(__file__).parent
    src_dir = script_dir
    web_dir = script_dir.parent / "Web"

    if not web_dir.exists():
        print(f"Error: Web directory not found at {web_dir}")
        sys.exit(1)

    if args.command == "extract":
        extract_command(web_dir, src_dir)
    elif args.command == "build":
        build_command(web_dir, src_dir)


if __name__ == "__main__":
    main()
