#!/usr/bin/env python3
"""Verify the three first-release archives before uploading them to GitHub."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import sys
import zipfile


ROOT = Path(__file__).resolve().parents[1]
REQUIRED = {
    "cybertexel/include/ctex/capi.h",
    "cybertexel/include/ctex/version.h",
    "cybertexel/share/cybertexel/LICENSE",
    "cybertexel/share/cybertexel/THIRD_PARTY_NOTICES.md",
    "cybertexel/share/cybertexel/package.json",
}


def verify_archive(path: Path, version: str, platform: dict[str, str]) -> list[str]:
    failures: list[str] = []
    try:
        with zipfile.ZipFile(path) as archive:
            names = archive.namelist()
            contents = set(names)
            if len(names) != len(contents):
                failures.append(f"{path.name}: duplicate ZIP entries")
            missing = REQUIRED - contents
            if platform["kind"] == "desktop" and "cybertexel/bin/cybertexel" not in contents:
                missing.add("cybertexel/bin/cybertexel")
            if missing:
                failures.append(f"{path.name}: missing {', '.join(sorted(missing))}")
            if not any("cybertexel_c" in Path(name).name for name in contents):
                failures.append(f"{path.name}: C ABI library missing")
            if any(not name.startswith("cybertexel/") or ".." in Path(name).parts for name in names):
                failures.append(f"{path.name}: invalid ZIP entry path")
            damaged = archive.testzip()
            if damaged:
                failures.append(f"{path.name}: corrupt ZIP entry {damaged}")
            manifest_name = "cybertexel/share/cybertexel/package.json"
            if manifest_name in contents:
                manifest = json.loads(archive.read(manifest_name))
                if not isinstance(manifest, dict):
                    failures.append(f"{path.name}: package manifest is not an object")
                    return failures
                expected = {"version": version, "platform": platform["preset"],
                            "smoke_test": platform["smoke_test"]}
                for key, value in expected.items():
                    if manifest.get(key) != value:
                        failures.append(f"{path.name}: {key} is {manifest.get(key)!r}, expected {value!r}")
    except (OSError, zipfile.BadZipFile, json.JSONDecodeError) as error:
        failures.append(f"{path.name}: {error}")
    return failures


def verify_assets(directory: Path, root: Path = ROOT) -> tuple[list[str], list[tuple[str, str]]]:
    version = (root / "VERSION").read_text(encoding="utf-8").strip()
    scope = json.loads((root / "release" / "platforms.json").read_text(encoding="utf-8"))
    expected = {f"cybertexel-{version}-{item['preset']}.zip": item
                for item in scope["in_scope"]}
    files = list(directory.rglob("*.zip"))
    by_name: dict[str, Path] = {}
    failures: list[str] = []
    for path in files:
        if path.name in by_name:
            failures.append(f"duplicate release asset {path.name}")
        by_name[path.name] = path
    for name in sorted(expected.keys() - by_name.keys()):
        failures.append(f"missing release asset {name}")
    for name in sorted(by_name.keys() - expected.keys()):
        failures.append(f"unexpected release asset {name}")
    checksums: list[tuple[str, str]] = []
    for name, platform in sorted(expected.items()):
        path = by_name.get(name)
        if path is None:
            continue
        failures.extend(verify_archive(path, version, platform))
        checksums.append((name, hashlib.sha256(path.read_bytes()).hexdigest()))
    return failures, checksums


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("directory", nargs="?", type=Path, default=ROOT / "dist")
    args = parser.parse_args()
    try:
        failures, checksums = verify_assets(args.directory)
    except (OSError, KeyError, TypeError, ValueError, json.JSONDecodeError) as error:
        failures, checksums = [str(error)], []
    if failures:
        for failure in failures:
            print(f"release asset gate failed: {failure}", file=sys.stderr)
        return 1
    for name, digest in checksums:
        print(f"{digest}  {name}")
    print("release asset gate passed: three in-scope archives")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
