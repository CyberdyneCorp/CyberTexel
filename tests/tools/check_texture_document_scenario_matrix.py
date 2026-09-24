#!/usr/bin/env python3

from __future__ import annotations

import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
MATRIX = ROOT / "docs" / "texture-document-scenarios.md"
SPEC = (
    ROOT
    / "openspec"
    / "specs"
    / "texture-document"
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
            raise ValueError(f"scenario {cells[0]!r} has empty evidence")
        rows.append((cells[0], cells[1]))
    return rows


def registered_ctests() -> set[str]:
    cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    return set(re.findall(r"add_test\(\s*NAME\s+([^\s\)]+)", cmake))


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
    for scenario, evidence in rows:
        commands = re.findall(r"`([^`]+)`", evidence)
        if not commands:
            failures.append(f"{scenario!r} has no named CTest evidence")
        for command in commands:
            if command not in ctests:
                failures.append(f"{scenario!r} names unknown CTest {command!r}")
    if failures:
        for failure in failures:
            print(failure)
        return 1
    print(f"ok: all {len(expected)} texture-document scenarios have mapped evidence")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
