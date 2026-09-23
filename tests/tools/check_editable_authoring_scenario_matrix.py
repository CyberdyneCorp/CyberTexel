#!/usr/bin/env python3

from __future__ import annotations

import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
MATRIX = ROOT / "docs" / "editable-authoring-scenarios.md"
SPEC = (
    ROOT
    / "openspec"
    / "changes"
    / "bootstrap-v1-cybertexel"
    / "specs"
    / "editable-authoring"
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
    labeled: set[str] = set()
    for match in re.finditer(
        r"(?:set_tests_properties|set_property)\((?P<body>.*?)\)", cmake, re.DOTALL
    ):
        body = match.group("body")
        if "LABELS" not in body or "editable-authoring-scenario" not in body:
            continue
        tests = re.split(r"\b(?:PROPERTIES|APPEND PROPERTY)\b", body, maxsplit=1)[0]
        labeled.update(name for name in tests.split() if name != "TEST")
    return labeled


def main() -> int:
    expected = spec_scenarios()
    rows = matrix_rows()
    documented = [scenario for scenario, _ in rows]
    failures: list[str] = []
    for label, values in (
        ("missing", sorted(set(expected) - set(documented))),
        ("unknown", sorted(set(documented) - set(expected))),
        ("duplicate", sorted({name for name in documented if documented.count(name) > 1})),
    ):
        if values:
            failures.append(f"{label} scenarios: {', '.join(values)}")

    ctests = registered_ctests()
    labeled = labeled_ctests()
    for scenario, evidence in rows:
        for command in re.findall(r"`([^`]+)`", evidence):
            if command not in ctests:
                failures.append(f"{scenario!r} names unknown CTest {command!r}")
            elif command not in labeled:
                failures.append(f"{scenario!r} names unlabeled CTest {command!r}")
    if "editable-authoring-scenario-matrix" not in labeled:
        failures.append("scenario matrix CTest is missing the editable-authoring label")
    if failures:
        for failure in failures:
            print(failure)
        return 1
    print(f"ok: all {len(expected)} editable-authoring scenarios have mapped evidence")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
