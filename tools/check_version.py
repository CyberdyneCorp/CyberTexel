#!/usr/bin/env python3
"""Verify every version consumer agrees with the repository VERSION file."""

from __future__ import annotations

import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SEMVER = re.compile(r"^[0-9]+\.[0-9]+\.[0-9]+$")


def extract(path: Path, pattern: str, label: str) -> tuple[str | None, str | None]:
    match = re.search(pattern, path.read_text(encoding="utf-8"), re.MULTILINE)
    if not match:
        return None, f"{label} does not declare a version"
    return match.group(1), None


def package_failures(root: Path, expected: str) -> list[str]:
    declarations = (
        (
            "Python package",
            root / "python" / "pyproject.toml",
            r'^version\s*=\s*"([^"]+)"',
        ),
        (
            "Rust package",
            root / "rust" / "Cargo.toml",
            r'^version\s*=\s*"([^"]+)"',
        ),
        (
            "Swift package",
            root / "swift" / "Package.swift",
            r'^let cyberTexelVersion\s*=\s*"([^"]+)"',
        ),
    )
    failures: list[str] = []
    for label, path, pattern in declarations:
        if not path.is_file():
            failures.append(f"{label} manifest is missing: {path.relative_to(root)}")
            continue
        declared, failure = extract(path, pattern, label)
        if failure:
            failures.append(failure)
        elif declared != expected:
            failures.append(f"{label} version {declared} differs from VERSION {expected}")
    return failures


def native_failures(root: Path, expected: str) -> list[str]:
    failures: list[str] = []
    cmake = (root / "CMakeLists.txt").read_text(encoding="utf-8")
    required_cmake_fragments = (
        'file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" CTEX_VERSION)',
        'string(STRIP "${CTEX_VERSION}" CTEX_VERSION)',
        'project(CyberTexel VERSION "${CTEX_VERSION}"',
    )
    for fragment in required_cmake_fragments:
        if fragment not in cmake:
            failures.append(f"CMake does not consume VERSION through: {fragment}")

    header_template = (root / "cmake" / "version.h.in").read_text(encoding="utf-8")
    for token in (
        "@CyberTexel_VERSION_MAJOR@",
        "@CyberTexel_VERSION_MINOR@",
        "@CyberTexel_VERSION_PATCH@",
        "@CyberTexel_VERSION@",
    ):
        if token not in header_template:
            failures.append(f"generated native version header is missing {token}")

    generated_header = (
        root / "build" / "headless" / "generated" / "include" / "ctex" / "version.h"
    )
    if not generated_header.is_file():
        failures.append("configured native version header is missing; run just build")
    else:
        configured, failure = extract(
            generated_header,
            r'^#define CTEX_VERSION_STRING "([^"]+)"$',
            "CMake project",
        )
        if failure:
            failures.append(failure)
        elif configured != expected:
            failures.append(
                f"CMake project version {configured} differs from VERSION {expected}"
            )
    return failures


def check(root: Path) -> list[str]:
    version_file = root / "VERSION"
    if not version_file.is_file():
        return ["VERSION is missing"]
    expected = version_file.read_text(encoding="utf-8").strip()
    if not SEMVER.fullmatch(expected):
        return [f"VERSION must be major.minor.patch; found {expected!r}"]
    return [*native_failures(root, expected), *package_failures(root, expected)]


def main() -> int:
    failures = check(ROOT)
    if failures:
        print("version consistency check failed:", file=sys.stderr)
        for failure in failures:
            print(f"  - {failure}", file=sys.stderr)
        return 1
    version = (ROOT / "VERSION").read_text(encoding="utf-8").strip()
    print(f"ok: build, ABI, container writer and three binding manifests use {version}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
