#!/usr/bin/env python3
"""Check that every capability has a numbered Python example."""

from __future__ import annotations

import ast
import json
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def declared_capabilities(script: Path) -> list[str]:
    tree = ast.parse(script.read_text(encoding="utf-8"), filename=str(script))
    assignments = [
        node.value
        for node in tree.body
        if isinstance(node, ast.Assign)
        and any(isinstance(target, ast.Name) and target.id == "CAPABILITIES" for target in node.targets)
    ]
    if len(assignments) != 1:
        raise ValueError("must define CAPABILITIES exactly once")
    value = ast.literal_eval(assignments[0])
    if not isinstance(value, (list, tuple)) or not value:
        raise ValueError("CAPABILITIES must be a non-empty literal list or tuple")
    if not all(isinstance(name, str) and name for name in value):
        raise ValueError("CAPABILITIES entries must be non-empty strings")
    if len(value) != len(set(value)):
        raise ValueError("CAPABILITIES contains duplicates")
    return list(value)


def coverage_failures(root: Path = ROOT) -> list[str]:
    manifest = json.loads((root / "abi" / "capi-capabilities.json").read_text(encoding="utf-8"))
    expected = {entry["name"] for entry in manifest["capabilities"]}
    covered: dict[str, list[str]] = {}
    failures: list[str] = []
    scripts = sorted((root / "examples").glob("[0-9][0-9]_*.py"))
    for script in scripts:
        try:
            names = declared_capabilities(script)
        except (SyntaxError, ValueError) as error:
            failures.append(f"{script.name}: {error}")
            continue
        unknown = sorted(set(names) - expected)
        if unknown:
            failures.append(f"{script.name}: unknown capabilities: {', '.join(unknown)}")
        for name in set(names) & expected:
            covered.setdefault(name, []).append(script.stem)
    missing = sorted(expected - covered.keys())
    if missing:
        failures.append(f"capabilities without numbered examples: {', '.join(missing)}")
    return failures


def main() -> int:
    failures = coverage_failures()
    if failures:
        print("example coverage gate failed:", file=sys.stderr)
        for failure in failures:
            print(f"- {failure}", file=sys.stderr)
        return 1
    print("example coverage gate passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
