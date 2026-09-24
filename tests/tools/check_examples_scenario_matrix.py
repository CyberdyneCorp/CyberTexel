#!/usr/bin/env python3

from __future__ import annotations

import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
MATRIX = ROOT / "docs" / "examples-scenarios.md"
SPEC = (
    ROOT
    / "openspec"
    / "specs"
    / "examples"
    / "spec.md"
)
LABEL = "examples-scenario"


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


def registered_ctests(cmake: str) -> set[str]:
    return set(re.findall(r"add_test\(\s*NAME\s+([^\s\)]+)", cmake))


def labeled_ctests(cmake: str) -> set[str]:
    result: set[str] = set()
    for match in re.finditer(r"set_tests_properties\((.*?)\)", cmake, re.DOTALL):
        body = match.group(1)
        if f'LABELS "{LABEL}"' not in body:
            continue
        result.update(re.split(r"\bPROPERTIES\b", body, maxsplit=1)[0].split())
    return result


def main() -> int:
    expected = spec_scenarios()
    rows = matrix_rows()
    documented = [scenario for scenario, _ in rows]
    failures: list[str] = []
    for description, values in (
        ("missing", sorted(set(expected) - set(documented))),
        ("unknown", sorted(set(documented) - set(expected))),
        ("duplicate", sorted({name for name in documented if documented.count(name) > 1})),
    ):
        if values:
            failures.append(f"{description} scenarios: {', '.join(values)}")

    cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    registered = registered_ctests(cmake)
    labeled = labeled_ctests(cmake)
    for scenario, evidence in rows:
        for test in re.findall(r"`([^`]+)`", evidence):
            if test not in registered:
                failures.append(f"{scenario!r} names unknown CTest {test!r}")
            elif test not in labeled:
                failures.append(f"{scenario!r} names unlabeled CTest {test!r}")
    if "examples-scenario-matrix" not in labeled:
        failures.append("scenario matrix CTest is missing its capability label")

    # The recorded-trace feature gate is what makes "exercises the library" a
    # decision rather than a claim; it must stay wired into the examples recipe.
    justfile = (ROOT / "justfile").read_text(encoding="utf-8")
    for required in (
        "tools/check_example_features.py",
        "tools/check_example_readability.py",
        "CTEX_SYMBOL_TRACE",
    ):
        if required not in justfile:
            failures.append(f"the examples recipe no longer runs {required}")
    if not (ROOT / "examples" / "feature_coverage.json").is_file():
        failures.append("examples/feature_coverage.json is missing")

    if failures:
        print("\n".join(failures))
        return 1
    print(f"ok: all {len(expected)} examples scenarios have mapped evidence")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
