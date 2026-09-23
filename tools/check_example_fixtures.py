#!/usr/bin/env python3
"""Validate example fixture completeness, provenance, and byte identity."""

from __future__ import annotations

import hashlib
import json
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
FIXTURES = ROOT / "examples" / "fixtures"
REQUIRED_CATEGORIES = {"mesh-uv", "mesh-udim", "mesh-map", "brush-alpha", "image", "font"}


def check(fixtures: Path) -> list[str]:
    failures: list[str] = []
    manifest_path = fixtures / "manifest.json"
    attribution_path = fixtures / "ATTRIBUTION.md"
    if not manifest_path.is_file() or not attribution_path.is_file():
        return ["fixture manifest and ATTRIBUTION.md are required"]
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    attribution = attribution_path.read_text(encoding="utf-8")
    assets = manifest.get("assets", [])
    categories = {asset.get("category") for asset in assets}
    for category in sorted(REQUIRED_CATEGORIES - categories):
        failures.append(f"fixture category is missing: {category}")
    seen: set[str] = set()
    for asset in assets:
        relative = asset.get("path", "")
        if not relative or relative in seen:
            failures.append(f"fixture path is empty or duplicated: {relative!r}")
            continue
        seen.add(relative)
        path = fixtures / relative
        if not path.is_file():
            failures.append(f"fixture file is missing: {relative}")
            continue
        digest = hashlib.sha256(path.read_bytes()).hexdigest()
        if digest != asset.get("sha256"):
            failures.append(f"fixture digest changed: {relative}")
        if not asset.get("origin") or not asset.get("license"):
            failures.append(f"fixture provenance is incomplete: {relative}")
        if f"`{relative}`" not in attribution:
            failures.append(f"fixture is absent from ATTRIBUTION.md: {relative}")
    expected = {
        path.relative_to(fixtures).as_posix()
        for path in fixtures.rglob("*")
        if path.is_file()
        and path.name not in {"manifest.json", "ATTRIBUTION.md", "generate_images.py"}
    }
    for relative in sorted(expected - seen):
        failures.append(f"untracked fixture asset: {relative}")
    return failures


def main() -> int:
    failures = check(FIXTURES)
    if failures:
        print("example fixture check failed:", file=sys.stderr)
        for failure in failures:
            print(f"  - {failure}", file=sys.stderr)
        return 1
    count = len(json.loads((FIXTURES / "manifest.json").read_text())["assets"])
    print(f"ok: {count} fixture assets have complete provenance and matching digests")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
