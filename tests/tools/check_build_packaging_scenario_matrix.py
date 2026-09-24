#!/usr/bin/env python3

from __future__ import annotations

import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
MATRIX = ROOT / "docs" / "build-packaging-scenarios.md"
SPEC = ROOT / "openspec" / "specs" / "build-packaging" / "spec.md"
TASKS = ROOT / "openspec" / "changes" / "archive" / "2026-09-24-bootstrap-v1-cybertexel" / "tasks.md"
LABEL = "build-packaging-scenario"


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


def labeled_ctests(cmake: str, label: str | None = None) -> set[str]:
    result: set[str] = set()
    for match in re.finditer(r"set_tests_properties\((.*?)\)", cmake, re.DOTALL):
        body = match.group(1)
        if "LABELS" not in body:
            continue
        if label is not None and f'LABELS "{label}"' not in body:
            continue
        result.update(re.split(r"\bPROPERTIES\b", body, maxsplit=1)[0].split())
    return result


def open_tasks() -> set[str]:
    text = TASKS.read_text(encoding="utf-8")
    return set(re.findall(r"^- \[ \] (\d+\.\d+)", text, re.MULTILINE))


def closed_tasks() -> set[str]:
    text = TASKS.read_text(encoding="utf-8")
    return set(re.findall(r"^- \[x\] (\d+\.\d+)", text, re.MULTILINE))


def main() -> int:
    expected = spec_scenarios()
    rows = matrix_rows()
    documented = [scenario for scenario, _ in rows]
    failures: list[str] = []
    outstanding: list[str] = []
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
    open_task_ids = open_tasks()
    closed_task_ids = closed_tasks()

    for scenario, evidence in rows:
        deferred = re.findall(r"\btask (\d+\.\d+)\b", evidence)
        for task in deferred:
            if task in closed_task_ids:
                failures.append(
                    f"{scenario!r} defers to task {task}, which is already checked; "
                    "name its executable evidence instead"
                )
            elif task not in open_task_ids:
                failures.append(f"{scenario!r} names unknown task {task}")
            else:
                outstanding.append(f"{scenario} (task {task})")
        tests = re.findall(r"`([^`]+)`", evidence)
        if not tests and not deferred:
            failures.append(f"{scenario!r} names neither a CTest nor an open task")
        for test in tests:
            if test not in registered:
                failures.append(f"{scenario!r} names unknown CTest {test!r}")
            elif test not in labeled:
                failures.append(f"{scenario!r} names unlabeled CTest {test!r}")

    if "build-packaging-scenario-matrix" not in labeled_ctests(cmake, LABEL):
        failures.append("scenario matrix CTest is missing its capability label")
    if not (ROOT / "release" / "platforms.json").is_file():
        failures.append("release/platforms.json is missing")

    if failures:
        print("\n".join(failures))
        return 1
    if outstanding:
        print("build-packaging scenarios still without evidence:")
        for entry in outstanding:
            print(f"- {entry}")
        return 1
    print(f"ok: all {len(expected)} build-packaging scenarios have mapped evidence")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
