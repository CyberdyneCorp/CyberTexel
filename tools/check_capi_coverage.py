#!/usr/bin/env python3
"""Verify that every runtime capability has concrete public C ABI evidence."""

from __future__ import annotations

import json
import re
import sys
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "abi" / "capi-capabilities.json"


def spec_capabilities(root: Path) -> set[str]:
    specs = root / "openspec" / "specs"
    return {path.parent.name for path in specs.glob("*/spec.md")}


def spec_requirements(root: Path, capability: str) -> set[str]:
    path = (
        root
        / "openspec"
        / "specs"
        / capability
        / "spec.md"
    )
    return set(re.findall(r"^### Requirement: (.+)$", path.read_text(encoding="utf-8"), re.MULTILINE))


def public_symbols(root: Path) -> set[str]:
    header = (root / "include" / "ctex" / "capi.h").read_text(encoding="utf-8")
    symbols: set[str] = set()
    for declaration in re.findall(r"^CTEX_API\s+(.+?);", header, re.DOTALL | re.MULTILINE):
        match = re.search(r"\b(ctex_[A-Za-z0-9_]+)\s*\(", declaration)
        if match:
            symbols.add(match.group(1))
    return symbols


def nonempty_strings(value: Any) -> bool:
    return isinstance(value, list) and bool(value) and all(
        isinstance(item, str) and bool(item.strip()) for item in value
    )


def validate_requirement_mapping(
    capability: str, mapping: Any, declared_symbols: set[str], allow_evidence: bool
) -> tuple[str | None, list[str], set[str]]:
    if not isinstance(mapping, dict):
        return None, [f"{capability}: requirement mapping is not an object"], set()
    failures: list[str] = []
    name = mapping.get("requirement")
    valid_name = isinstance(name, str) and bool(name.strip())
    if not valid_name:
        failures.append(f"{capability}: requirement mapping has no name")
        name = None
    symbols = mapping.get("symbols")
    if nonempty_strings(symbols):
        unknown = set(symbols) - declared_symbols
        failures.extend(
            f"{capability}: requirement {name!r} names unknown symbol: {symbol}"
            for symbol in sorted(unknown)
        )
        return name, failures, set(symbols)
    if not (allow_evidence and nonempty_strings(mapping.get("evidence"))):
        failures.append(f"{capability}: requirement {name!r} has no C ABI symbols")
    return name, failures, set()


def requirement_failures(
    root: Path, capability: str, mappings: Any, declared_symbols: set[str], allow_evidence: bool
) -> tuple[list[str], set[str]]:
    if not isinstance(mappings, list):
        return [f"capability requirements are not an array: {capability}"], set()
    failures: list[str] = []
    mapped: set[str] = set()
    names: set[str] = set()
    expected = spec_requirements(root, capability)
    for mapping in mappings:
        name, mapping_failures, mapping_symbols = validate_requirement_mapping(
            capability, mapping, declared_symbols, allow_evidence
        )
        failures.extend(mapping_failures)
        mapped.update(mapping_symbols)
        if name in names:
            failures.append(f"{capability}: duplicate requirement mapping: {name}")
        elif name is not None:
            names.add(name)
    for name in sorted(expected - names):
        failures.append(f"{capability}: requirement lacks C ABI evidence: {name}")
    for name in sorted(names - expected):
        failures.append(f"{capability}: manifest names unknown requirement: {name}")
    return failures, mapped


def non_runtime_failures(entry: dict[str, Any]) -> list[str]:
    name = entry["name"]
    failures: list[str] = []
    if not isinstance(entry.get("rationale"), str) or not entry["rationale"].strip():
        failures.append(f"non-runtime capability has no rationale: {name}")
    if not nonempty_strings(entry.get("evidence")):
        failures.append(f"non-runtime capability has no evidence: {name}")
    if entry.get("requirements"):
        failures.append(f"non-runtime capability declares C ABI requirements: {name}")
    return failures


def capability_entry_failures(
    root: Path, entry: dict[str, Any], declared_symbols: set[str]
) -> tuple[list[str], set[str]]:
    name = entry["name"]
    kind = entry.get("kind")
    if kind in {"runtime", "boundary"}:
        return requirement_failures(
            root,
            name,
            entry.get("requirements"),
            declared_symbols,
            allow_evidence=kind == "boundary",
        )
    if kind == "non-runtime":
        return non_runtime_failures(entry), set()
    return [f"capability has invalid kind: {name}: {kind!r}"], set()


def inventory_failures(
    expected: set[str], seen: set[str], declared_symbols: set[str], mapped_symbols: set[str]
) -> list[str]:
    failures = [
        f"OpenSpec capability is missing from C ABI manifest: {name}"
        for name in sorted(expected - seen)
    ]
    failures.extend(
        f"C ABI manifest names unknown capability: {name}" for name in sorted(seen - expected)
    )
    failures.extend(
        f"public C symbol is not mapped to a capability operation: {symbol}"
        for symbol in sorted(declared_symbols - mapped_symbols)
    )
    return failures


def check(root: Path, manifest_path: Path | None = None) -> list[str]:
    path = manifest_path or root / "abi" / "capi-capabilities.json"
    try:
        manifest = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        return [f"C ABI capability manifest cannot be read: {error}"]
    if manifest.get("schema") != 1 or not isinstance(manifest.get("capabilities"), list):
        return ["C ABI capability manifest must use schema 1 with a capabilities array"]

    expected = spec_capabilities(root)
    declared_symbols = public_symbols(root)
    failures: list[str] = []
    mapped_symbols: set[str] = set()
    seen: set[str] = set()
    for entry in manifest["capabilities"]:
        if not isinstance(entry, dict) or not isinstance(entry.get("name"), str):
            failures.append("C ABI capability entry is not a named object")
            continue
        name = entry["name"]
        if name in seen:
            failures.append(f"duplicate C ABI capability entry: {name}")
            continue
        seen.add(name)
        entry_failures, entry_symbols = capability_entry_failures(root, entry, declared_symbols)
        failures.extend(entry_failures)
        mapped_symbols.update(entry_symbols)

    failures.extend(inventory_failures(expected, seen, declared_symbols, mapped_symbols))
    return failures


def main() -> int:
    failures = check(ROOT, MANIFEST)
    if failures:
        print("C ABI capability coverage failed:", file=sys.stderr)
        for failure in failures:
            print(f"  - {failure}", file=sys.stderr)
        return 1
    print("ok: every runtime capability has public C ABI operation evidence")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
