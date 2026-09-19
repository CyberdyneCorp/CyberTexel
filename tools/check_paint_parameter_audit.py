#!/usr/bin/env python3
"""Keep documented paint parameters tied to explicit behavioural test evidence."""

from __future__ import annotations

import re
import sys
from collections import Counter
from pathlib import Path

PARAMETER_ROW = re.compile(r"^\| `([^`]+)` \|", re.MULTILINE)
AUDIT_MARKER = re.compile(r"parameter-audit: ([a-z0-9_.]+)")
PARAMETER_DESCRIPTOR = re.compile(
    r'ToolParameterDescriptor\s+[A-Za-z_][A-Za-z0-9_]*\s*\{\s*"([a-z0-9_.]+)"',
    re.DOTALL,
)
PARAMETER_DESCRIPTOR_FACTORY = re.compile(
    r'taper_extent_descriptor\(\s*"([a-z0-9_.]+)"', re.DOTALL
)


def documented_parameters(root: Path) -> list[str]:
    document = root / "docs" / "paint-tool-parameters.md"
    return PARAMETER_ROW.findall(document.read_text(encoding="utf-8"))


def implemented_parameters(root: Path) -> list[str]:
    files = [
        *sorted((root / "include" / "ctex" / "paint").glob("*.hpp")),
        *sorted((root / "src" / "paint").glob("*.cpp")),
    ]
    implementation = "\n".join(path.read_text(encoding="utf-8") for path in files)
    return [
        *PARAMETER_DESCRIPTOR.findall(implementation),
        *PARAMETER_DESCRIPTOR_FACTORY.findall(implementation),
    ]


def audit_markers(root: Path) -> list[str]:
    tests = sorted((root / "tests" / "paint").glob("*_test.cpp"))
    return [
        marker
        for path in tests
        for marker in AUDIT_MARKER.findall(path.read_text(encoding="utf-8"))
    ]


def audit(root: Path) -> list[str]:
    documented = documented_parameters(root)
    documented_counts = Counter(documented)
    implemented_counts = Counter(implemented_parameters(root))
    evidence = audit_markers(root)
    evidence_counts = Counter(evidence)
    failures: list[str] = []

    if not documented:
        failures.append("parameter table contains no documented parameters")

    for name, count in sorted(documented_counts.items()):
        if count != 1:
            failures.append(f"documented parameter {name!r} appears {count} times")
        implementation_count = implemented_counts[name]
        if implementation_count != 1:
            failures.append(
                f"documented parameter {name!r} has {implementation_count} implementation "
                "descriptors"
            )
        evidence_count = evidence_counts[name]
        if evidence_count != 1:
            failures.append(
                f"documented parameter {name!r} has {evidence_count} behavioural audit markers"
            )

    for name in sorted(implemented_counts.keys() - documented_counts.keys()):
        failures.append(f"implementation descriptor {name!r} is not in the parameter table")

    for name in sorted(evidence_counts.keys() - documented_counts.keys()):
        failures.append(f"audit marker {name!r} is not in the parameter table")

    return failures


def main() -> int:
    root = Path.cwd()
    if not (root / "docs" / "paint-tool-parameters.md").is_file():
        print("error: run from the repository root", file=sys.stderr)
        return 2
    failures = audit(root)
    if failures:
        print("paint parameter audit failed:", file=sys.stderr)
        for failure in failures:
            print(f"  - {failure}", file=sys.stderr)
        return 1
    count = len(documented_parameters(root))
    print(f"ok: {count} documented paint parameters have implementation and behaviour evidence")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
