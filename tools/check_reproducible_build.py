#!/usr/bin/env python3
"""Build the shipped library twice and compare the produced binaries.

`build-packaging` requires that building the same commit twice with the same
toolchain produces identical libraries where the toolchain supports it, and that
any unavoidable non-determinism is named in the documentation. This gate builds
into two separate trees, compares every shipped artifact byte for byte, and
compares a differing artifact against the documented variances rather than
accepting it silently.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import platform
import re
import shutil
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
VARIANCES = ROOT / "docs" / "reproducible-builds.md"
NATIVE_SHIPPED_PRESET = {
    "Darwin": "macos-universal",
    "Linux": "linux-x64",
    "Windows": "windows-x64",
}


class BuildError(RuntimeError):
    pass


def documented_variances(path: Path = VARIANCES) -> dict[str, str]:
    """Artifact name -> the named differing input, read from the documentation."""
    variances: dict[str, str] = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        if not line.startswith("|"):
            continue
        cells = [cell.strip() for cell in line.strip().strip("|").split("|")]
        if len(cells) != 3 or cells[0] in {"Artifact", "---"}:
            continue
        if not cells[1] or not cells[2]:
            raise BuildError(f"documented variance for {cells[0]!r} names no differing input")
        variances[cells[0]] = cells[1]
    return variances


def build(preset: str, work: Path, destination: Path) -> None:
    """Configure and build at one fixed path, then copy the artifacts aside.

    Both builds use the same directory, so the build path cannot be the input
    that makes two builds of one commit differ.
    """
    environment = os.environ.copy()
    if work.is_dir():
        shutil.rmtree(work)
    subprocess.run(
        ["cmake", "--preset", preset, "-B", str(work)], cwd=ROOT, env=environment, check=True
    )
    subprocess.run(
        ["cmake", "--build", str(work), "--target", "cybertexel_c"],
        cwd=ROOT,
        env=environment,
        check=True,
    )
    produced = shipped_artifacts(work)
    if not produced:
        raise BuildError(f"no shipped library was produced by preset {preset}")
    destination.mkdir(parents=True, exist_ok=True)
    for name, path in produced.items():
        # Resolve symlinked version aliases so the comparison is over content.
        shutil.copyfile(path, destination / name, follow_symlinks=True)
    shutil.rmtree(work)


def shipped_artifacts(directory: Path) -> dict[str, Path]:
    pattern = re.compile(r"^(lib)?cybertexel_c(\.\d+)*\.(so|dylib|dll|a|lib)(\.\d+)*$")
    return {
        path.name: path
        for path in sorted(directory.rglob("*"))
        if path.is_file() and pattern.match(path.name)
    }


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def compare(first: Path, second: Path) -> list[str]:
    left = shipped_artifacts(first)
    right = shipped_artifacts(second)
    if left.keys() != right.keys():
        raise BuildError(
            f"the two builds produced different artifacts: {sorted(left)} != {sorted(right)}"
        )
    variances = documented_variances()
    failures = []
    for name in sorted(left):
        if digest(left[name]) == digest(right[name]):
            continue
        if name in variances:
            print(f"documented variance: {name} — {variances[name]}")
            continue
        failures.append(f"{name} differs between two builds of the same commit and is not documented")
    return failures


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--preset", default=NATIVE_SHIPPED_PRESET.get(platform.system(), "linux-x64")
    )
    parser.add_argument("--keep", action="store_true", help="keep the two build trees")
    arguments = parser.parse_args()

    work = ROOT / "build" / "reproducible"
    first, second = work / "a", work / "b"
    try:
        if work.is_dir():
            shutil.rmtree(work)
        for directory in (first, second):
            build(arguments.preset, work / "tree", directory)
        failures = compare(first, second)
    except (BuildError, OSError, subprocess.CalledProcessError) as error:
        print(f"reproducible build gate failed: {error}", file=sys.stderr)
        return 1
    finally:
        if not arguments.keep and work.is_dir():
            shutil.rmtree(work, ignore_errors=True)
    if failures:
        print("reproducible build gate failed:", file=sys.stderr)
        for failure in failures:
            print(f"- {failure}", file=sys.stderr)
        return 1
    print(f"reproducible build gate passed for preset {arguments.preset}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
