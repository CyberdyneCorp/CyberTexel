#!/usr/bin/env python3

from __future__ import annotations

import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
MATRIX = ROOT / "docs" / "resource-residency-scenarios.md"
SPEC = (
    ROOT
    / "openspec"
    / "changes"
    / "bootstrap-v1-cybertexel"
    / "specs"
    / "resource-residency"
    / "spec.md"
)


def spec_scenarios() -> list[str]:
    pattern = re.compile(r"^#### Scenario: (.+)$")
    return [
        match.group(1)
        for line in SPEC.read_text(encoding="utf-8").splitlines()
        if (match := pattern.match(line))
    ]


def matrix_rows() -> list[tuple[str, str]]:
    rows: list[tuple[str, str]] = []
    for line in MATRIX.read_text(encoding="utf-8").splitlines():
        if not line.startswith("|"):
            continue
        cells = [cell.strip() for cell in line.strip().strip("|").split("|")]
        if len(cells) != 3 or cells[0] in {"OpenSpec scenario", "---"}:
            continue
        if not cells[1] or not cells[2]:
            raise ValueError(f"scenario {cells[0]!r} has empty executable evidence")
        rows.append((cells[0], cells[1]))
    return rows


def registered_ctests() -> set[str]:
    cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    return set(re.findall(r"add_test\(\s*NAME\s+([^\s\)]+)", cmake))


def labeled_ctests() -> set[str]:
    cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    match = re.search(
        r"set_property\(\s*TEST(?P<tests>.*?)"
        r"APPEND PROPERTY LABELS resource-residency-scenario\s*\)",
        cmake,
        re.DOTALL,
    )
    if match is None:
        return set()
    return set(match.group("tests").split())


def main() -> int:
    expected = spec_scenarios()
    rows = matrix_rows()
    documented = [scenario for scenario, _ in rows]
    failures: list[str] = []
    missing = sorted(set(expected) - set(documented))
    unexpected = sorted(set(documented) - set(expected))
    duplicates = sorted({name for name in documented if documented.count(name) > 1})
    if missing:
        failures.append(f"missing scenarios: {', '.join(missing)}")
    if unexpected:
        failures.append(f"unknown scenarios: {', '.join(unexpected)}")
    if duplicates:
        failures.append(f"duplicate scenarios: {', '.join(duplicates)}")

    ctests = registered_ctests()
    labeled = labeled_ctests()
    for scenario, evidence in rows:
        for command in re.findall(r"`([^`]+)`", evidence):
            if command not in ctests:
                failures.append(f"{scenario!r} names unknown CTest {command!r}")
            elif command not in labeled:
                failures.append(f"{scenario!r} names unlabeled CTest {command!r}")
    if "resource-residency-scenario-matrix" not in labeled:
        failures.append("scenario matrix CTest is missing the resource-residency label")
    if failures:
        for failure in failures:
            print(failure)
        return 1
    print(f"ok: all {len(expected)} resource-residency scenarios have mapped evidence")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
