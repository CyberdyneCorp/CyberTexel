#!/usr/bin/env python3
"""Check the public C ABI against the committed release baseline."""

from __future__ import annotations

import json
import re
import subprocess
import sys
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
BASELINE = ROOT / "abi" / "cybertexel-abi-v0.json"
HEADER = ROOT / "include" / "ctex" / "capi.h"
LIBRARY = ROOT / "build" / "headless" / "libcybertexel_c.so"


def normalize_declaration(declaration: str) -> str:
    normalized = " ".join(declaration.split())
    normalized = re.sub(r"\s*\*\s*", "*", normalized)
    normalized = re.sub(r"\s+([,;)])", r"\1", normalized)
    normalized = re.sub(r"([(,])\s+", r"\1", normalized)
    return normalized


def extract_symbols(header: str) -> dict[str, str]:
    symbols: dict[str, str] = {}
    for declaration in re.findall(r"^CTEX_API\s+(.+?);", header, re.DOTALL | re.MULTILINE):
        normalized = normalize_declaration(declaration)
        match = re.search(r"\b(ctex_[A-Za-z0-9_]+)\s*\(", normalized)
        if match:
            symbols[match.group(1)] = normalized
    return symbols


def extract_structures(header: str) -> dict[str, list[str]]:
    structures: dict[str, list[str]] = {}
    pattern = re.compile(
        r"typedef\s+struct\s+(ctex_[A-Za-z0-9_]+)\s*\{(.*?)\}\s*\1\s*;",
        re.DOTALL,
    )
    for name, body in pattern.findall(header):
        fields = [normalize_declaration(field) for field in body.split(";") if field.strip()]
        structures[name] = fields
    return structures


def extract_enums(header: str) -> dict[str, list[list[Any]]]:
    enums: dict[str, list[list[Any]]] = {}
    pattern = re.compile(
        r"typedef\s+enum\s+(ctex_[A-Za-z0-9_]+)\s*\{(.*?)\}\s*\1\s*;",
        re.DOTALL,
    )
    for name, body in pattern.findall(header):
        entries: list[list[Any]] = []
        next_value = 0
        for raw_entry in body.split(","):
            entry = normalize_declaration(raw_entry)
            if not entry:
                continue
            if "=" in entry:
                entry_name, raw_value = (part.strip() for part in entry.split("=", 1))
                next_value = int(raw_value, 0)
            else:
                entry_name = entry
            entries.append([entry_name, next_value])
            next_value += 1
        enums[name] = entries
    return enums


def extract_surface(header: str, version: str) -> dict[str, Any]:
    return {
        "abi_version": version,
        "symbols": extract_symbols(header),
        "structures": extract_structures(header),
        "enums": extract_enums(header),
    }


def version_tuple(version: str) -> tuple[int, int, int]:
    parts = version.split(".")
    if len(parts) != 3 or not all(part.isdigit() for part in parts):
        raise ValueError(f"invalid ABI version {version!r}")
    return tuple(int(part) for part in parts)  # type: ignore[return-value]


def compatibility_failures(baseline: dict[str, Any], current: dict[str, Any]) -> list[str]:
    failures: list[str] = []
    baseline_version = version_tuple(baseline["abi_version"])
    current_version = version_tuple(current["abi_version"])
    if current_version < baseline_version:
        failures.append(
            f"ABI version regressed from {baseline['abi_version']} to {current['abi_version']}"
        )
    if current_version[0] != baseline_version[0]:
        return failures

    for name, signature in baseline["symbols"].items():
        current_signature = current["symbols"].get(name)
        if current_signature is None:
            failures.append(f"ABI symbol removed without a major bump: {name}")
        elif current_signature != signature:
            failures.append(
                f"ABI signature changed without a major bump: {name}: "
                f"{signature!r} -> {current_signature!r}"
            )

    for name, fields in baseline["structures"].items():
        current_fields = current["structures"].get(name)
        if current_fields is None:
            failures.append(f"ABI structure removed without a major bump: {name}")
        elif current_fields[: len(fields)] != fields:
            failures.append(f"ABI structure fields changed before the append point: {name}")

    for name, entries in baseline["enums"].items():
        current_entries = current["enums"].get(name)
        if current_entries is None:
            failures.append(f"ABI enum removed without a major bump: {name}")
        elif current_entries[: len(entries)] != entries:
            failures.append(f"ABI enum values changed before the append point: {name}")
    return failures


def descriptor_failures(surface: dict[str, Any]) -> list[str]:
    failures: list[str] = []
    for name, fields in surface["structures"].items():
        if name.endswith("_descriptor") and (not fields or fields[0] != "uint32_t size"):
            failures.append(f"ABI descriptor does not begin with uint32_t size: {name}")
    return failures


def dynamic_exports(library: Path, nm: str) -> set[str]:
    completed = subprocess.run(
        [nm, "-D", "--defined-only", "-g", str(library)],
        check=True,
        capture_output=True,
        text=True,
    )
    return {
        line.split()[-1].split("@")[0]
        for line in completed.stdout.splitlines()
        if line.split()
    }


def export_failures(symbols: dict[str, str], library: Path, nm: str) -> list[str]:
    exports = dynamic_exports(library, nm)
    declared = set(symbols)
    failures: list[str] = []
    for name in sorted(exports - declared):
        failures.append(f"shared library exports undeclared symbol: {name}")
    for name in sorted(declared - exports):
        failures.append(f"public C symbol is not exported: {name}")
    for name in sorted(exports):
        if not name.startswith("ctex_"):
            failures.append(f"shared library exports non-ctex symbol: {name}")
    return failures


def windows_export_failures(root: Path, symbols: dict[str, str]) -> list[str]:
    definition = (root / "cmake" / "exports" / "cybertexel.def").read_text(encoding="utf-8")
    declared = {
        line.strip()
        for line in definition.splitlines()
        if line.strip() and line.strip() != "EXPORTS"
    }
    expected = set(symbols)
    failures = [f"Windows export map is missing: {name}" for name in sorted(expected - declared)]
    failures.extend(
        f"Windows export map contains undeclared symbol: {name}"
        for name in sorted(declared - expected)
    )
    return failures


def check(root: Path, library: Path | None = None, nm: str = "nm") -> list[str]:
    version = (root / "VERSION").read_text(encoding="utf-8").strip()
    header = (root / "include" / "ctex" / "capi.h").read_text(encoding="utf-8")
    baseline = json.loads((root / "abi" / "cybertexel-abi-v0.json").read_text(encoding="utf-8"))
    current = extract_surface(header, version)
    failures = [
        *compatibility_failures(baseline, current),
        *descriptor_failures(current),
        *windows_export_failures(root, current["symbols"]),
    ]
    if library is not None:
        if not library.is_file():
            failures.append(f"shared C ABI library is missing: {library}")
        else:
            failures.extend(export_failures(current["symbols"], library, nm))
    return failures


def main() -> int:
    if len(sys.argv) > 3:
        print("usage: check_abi.py [LIBRARY [NM]]", file=sys.stderr)
        return 2
    library = Path(sys.argv[1]) if len(sys.argv) >= 2 else LIBRARY
    nm = sys.argv[2] if len(sys.argv) == 3 else "nm"
    failures = check(ROOT, library, nm)
    if failures:
        print("ABI compatibility check failed:", file=sys.stderr)
        for failure in failures:
            print(f"  - {failure}", file=sys.stderr)
        return 1
    version = (ROOT / "VERSION").read_text(encoding="utf-8").strip()
    symbols = extract_symbols(HEADER.read_text(encoding="utf-8"))
    print(f"ok: ABI {version}, {len(symbols)} symbols match the v0 release baseline")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
