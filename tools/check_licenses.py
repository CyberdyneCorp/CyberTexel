#!/usr/bin/env python3
"""Audit every dependency that can be compiled into a shipped artifact."""

from __future__ import annotations

import json
import re
import sys
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
ALLOWED_LICENSES = {
    "Apache-2.0",
    "BSD-2-Clause",
    "BSD-3-Clause",
    "MIT",
    "MPL-2.0",
    "Zlib",
}
SOURCE_ROOTS = ("thirdparty", "vendor", "external")
IGNORED_SOURCE_DIRECTORIES = {"licenses"}
FLOATING_REVISIONS = {"head", "latest", "main", "master", "trunk"}
# CMake modules that select a platform/toolchain facility rather than code that
# can be compiled into the shipped artifact.
PLATFORM_PACKAGES = {"threads"}


def normalized(name: str) -> str:
    return re.sub(r"[^a-z0-9]", "", name.lower())


def load_manifest(root: Path) -> tuple[list[dict[str, Any]], list[str]]:
    path = root / "thirdparty" / "dependencies.json"
    if not path.is_file():
        return [], ["dependency manifest is missing: thirdparty/dependencies.json"]
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except (json.JSONDecodeError, OSError) as error:
        return [], [f"dependency manifest is invalid: {error}"]
    if data.get("schema") != 1 or not isinstance(data.get("dependencies"), list):
        return [], ["dependency manifest must use schema 1 with a dependencies array"]
    return data["dependencies"], []


def cmake_files(root: Path) -> list[Path]:
    paths = [root / "CMakeLists.txt", *root.rglob("*.cmake")]
    return [
        path
        for path in paths
        if path.is_file()
        and "build" not in path.relative_to(root).parts
        and path.relative_to(root).parts[:2] != ("rust", "target")
    ]


def discovered_dependencies(root: Path) -> dict[str, str]:
    discovered: dict[str, str] = {}
    for directory_name in SOURCE_ROOTS:
        directory = root / directory_name
        if not directory.is_dir():
            continue
        for child in directory.iterdir():
            if child.is_dir() and child.name not in IGNORED_SOURCE_DIRECTORIES:
                discovered[normalized(child.name)] = f"vendored tree {child.relative_to(root)}"

    patterns = (
        (
            re.compile(r"\bFetchContent_Declare\s*\(\s*([A-Za-z0-9_.+-]+)", re.I),
            "FetchContent",
        ),
        (
            re.compile(r"\bfind_package\s*\(\s*([A-Za-z0-9_.+-]+)", re.I),
            "find_package",
        ),
    )
    for path in cmake_files(root):
        text = path.read_text(encoding="utf-8")
        for pattern, kind in patterns:
            for match in pattern.finditer(text):
                name = match.group(1)
                if kind == "find_package" and normalized(name) in PLATFORM_PACKAGES:
                    continue
                discovered[normalized(name)] = f"{kind} declaration {path.relative_to(root)}"
    return discovered


def metadata_failures(entry: dict[str, Any], name: str) -> list[str]:
    failures: list[str] = []
    licence = entry.get("license")
    if licence not in ALLOWED_LICENSES:
        failures.append(f"{name}: licence {licence!r} is not permitted")
    revision = entry.get("revision")
    if not isinstance(revision, str) or not revision.strip():
        failures.append(f"{name}: immutable revision is missing")
    elif revision.lower() in FLOATING_REVISIONS:
        failures.append(f"{name}: revision {revision!r} is floating")
    source = entry.get("source")
    if not isinstance(source, str) or not source.startswith(("https://", "http://")):
        failures.append(f"{name}: source URL is missing or invalid")
    discovery_name = entry.get("discovery_name")
    if not isinstance(discovery_name, str) or not discovery_name.strip():
        failures.append(f"{name}: discovery_name is missing")
    return failures


def license_text_failures(root: Path, entry: dict[str, Any], name: str) -> list[str]:
    license_file = entry.get("license_file")
    if not isinstance(license_file, str) or not license_file.strip():
        return [f"{name}: license_file is missing"]
    license_path = root / license_file
    if not license_path.is_file():
        return [f"{name}: licence text is missing: {license_file}"]
    if len(license_path.read_text(encoding="utf-8").strip()) < 50:
        return [f"{name}: licence text is unexpectedly short: {license_file}"]
    return []


def notice_failures(entry: dict[str, Any], name: str, notices: str) -> list[str]:
    failures: list[str] = []
    fields = (
        ("name", name),
        ("licence", entry.get("license")),
        ("revision", entry.get("revision")),
    )
    for field, value in fields:
        if isinstance(value, str) and value not in notices:
            failures.append(f"{name}: {field} {value!r} is absent from THIRD_PARTY_NOTICES.md")
    return failures


def entry_failures(root: Path, entry: dict[str, Any], notices: str) -> list[str]:
    name = entry.get("name")
    if not isinstance(name, str) or not name.strip():
        return ["dependency entry has no name"]
    return [
        *metadata_failures(entry, name),
        *license_text_failures(root, entry, name),
        *notice_failures(entry, name, notices),
    ]


def tracked_dependencies(
    root: Path,
    entries: list[dict[str, Any]],
    notices: str,
) -> tuple[dict[str, str], list[str]]:
    tracked: dict[str, str] = {}
    failures: list[str] = []
    for entry in entries:
        if not isinstance(entry, dict):
            failures.append("dependency manifest contains a non-object entry")
            continue
        failures.extend(entry_failures(root, entry, notices))
        discovery_name = entry.get("discovery_name")
        if not isinstance(discovery_name, str):
            continue
        key = normalized(discovery_name)
        if key in tracked:
            failures.append(f"duplicate dependency discovery name: {discovery_name}")
        tracked[key] = str(entry.get("name", discovery_name))
    return tracked, failures


def coverage_failures(discovered: dict[str, str], tracked: dict[str, str]) -> list[str]:
    failures = [
        f"untracked dependency: {origin}"
        for key, origin in sorted(discovered.items())
        if key not in tracked
    ]
    failures.extend(
        f"{name}: manifest entry is not present in the build or vendored trees"
        for key, name in sorted(tracked.items())
        if key not in discovered
    )
    return failures


def audit(root: Path) -> list[str]:
    entries, failures = load_manifest(root)
    notices_path = root / "THIRD_PARTY_NOTICES.md"
    notices = notices_path.read_text(encoding="utf-8") if notices_path.is_file() else ""
    if not notices:
        failures.append("THIRD_PARTY_NOTICES.md is missing or empty")

    tracked, tracking_failures = tracked_dependencies(root, entries, notices)
    failures.extend(tracking_failures)
    discovered = discovered_dependencies(root)
    failures.extend(coverage_failures(discovered, tracked))
    return failures


def main() -> int:
    failures = audit(ROOT)
    if failures:
        print("licence audit failed:", file=sys.stderr)
        for failure in failures:
            print(f"  - {failure}", file=sys.stderr)
        return 1
    entries, _ = load_manifest(ROOT)
    print(f"ok: {len(entries)} shipped dependencies; manifest and source discovery agree")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
