#!/usr/bin/env python3

from __future__ import annotations

import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
MATRIX = ROOT / "docs" / "device-gate-scenarios.md"
SPEC = (
    ROOT
    / "openspec"
    / "specs"
    / "device-gate"
    / "spec.md"
)
LABEL = "device-gate-scenario"


def headings() -> list[str]:
    pattern = re.compile(r"^#### Scenario: (.+)$")
    return [
        match.group(1)
        for line in SPEC.read_text(encoding="utf-8").splitlines()
        if (match := pattern.match(line))
    ]


def rows() -> list[tuple[str, str]]:
    result = []
    for line in MATRIX.read_text(encoding="utf-8").splitlines():
        if not line.startswith("|"):
            continue
        cells = [cell.strip() for cell in line.strip().strip("|").split("|")]
        if len(cells) == 3 and cells[0] not in {"OpenSpec scenario", "---"}:
            result.append((cells[0], cells[1]))
    return result


def labeled_tests(cmake: str) -> set[str]:
    result: set[str] = set()
    for match in re.finditer(r"set_tests_properties\((.*?)\)", cmake, re.DOTALL):
        body = match.group(1)
        if LABEL not in body:
            continue
        names = re.split(r"\bPROPERTIES\b", body, maxsplit=1)[0]
        result.update(names.split())
    return result


def main() -> int:
    expected = headings()
    mapped = rows()
    names = [name for name, _ in mapped]
    failures = []
    for description, values in (
        ("missing", sorted(set(expected) - set(names))),
        ("unknown", sorted(set(names) - set(expected))),
        ("duplicate", sorted({name for name in names if names.count(name) > 1})),
    ):
        if values:
            failures.append(f"{description} scenarios: {', '.join(values)}")
    cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    registered = set(re.findall(r"add_test\(\s*NAME\s+([^\s\)]+)", cmake))
    labeled = labeled_tests(cmake)
    for scenario, evidence in mapped:
        for test in re.findall(r"`([^`]+)`", evidence):
            if test not in registered:
                failures.append(f"{scenario!r} names unknown CTest {test!r}")
            elif test not in labeled:
                failures.append(f"{scenario!r} names unlabeled CTest {test!r}")
    justfile = (ROOT / "justfile").read_text(encoding="utf-8")
    if "\ngate-budgets:" not in justfile or "tools/device_gate.py gate" not in justfile:
        failures.append("device-dependent gate-budgets recipe is not wired")
    if "device-gate-scenario-matrix" not in labeled:
        failures.append("scenario matrix CTest is missing its capability label")
    if failures:
        print("\n".join(failures))
        return 1
    print(f"ok: all {len(expected)} device-gate scenarios have mapped evidence")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
