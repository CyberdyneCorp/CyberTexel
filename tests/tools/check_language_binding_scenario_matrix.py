#!/usr/bin/env python3

from __future__ import annotations

import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
MATRIX = ROOT / "docs" / "language-binding-scenarios.md"
SPEC = (
    ROOT
    / "openspec"
    / "changes"
    / "bootstrap-v1-cybertexel"
    / "specs"
    / "language-bindings"
    / "spec.md"
)
LABEL = "language-bindings-scenario"


def scenario_headings() -> list[str]:
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
            raise ValueError(f"scenario {cells[0]!r} has incomplete evidence")
        rows.append((cells[0], cells[1]))
    return rows


def just_recipes(source: str) -> set[str]:
    return set(re.findall(r"^([a-z][a-z0-9_-]*)(?=[^:\n]*:)", source, re.MULTILINE))


def main() -> int:
    expected = scenario_headings()
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

    justfile = (ROOT / "justfile").read_text(encoding="utf-8")
    workflow = (ROOT / ".github/workflows/openspec-validate.yml").read_text(
        encoding="utf-8"
    )
    recipes = just_recipes(justfile)
    for scenario, evidence in rows:
        for recipe in re.findall(r"`just ([a-z0-9_-]+)`", evidence):
            if recipe not in recipes:
                failures.append(f"{scenario!r} names unknown just recipe {recipe!r}")
            elif not re.search(rf"run:\s+just {re.escape(recipe)}(?:\s|$)", workflow):
                failures.append(f"{scenario!r} names recipe absent from CI {recipe!r}")

    required_workflow_hooks = {
        "Python examples": "python3 tools/run_python_examples.py compare",
        "Swift example": '"CyberTexelWorkflowExample"',
        "Rust example": "-p cybertexel-sys --example workflow",
    }
    hook_sources = {
        "Python examples": justfile,
        "Swift example": (ROOT / "tools/test_swift_package.py").read_text(encoding="utf-8"),
        "Rust example": justfile,
    }
    for name, marker in required_workflow_hooks.items():
        if marker not in hook_sources[name]:
            failures.append(f"{name} is not executed by its binding gate")

    cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    if not re.search(
        rf"set_tests_properties\(\s*language-binding-scenario-matrix.*?LABELS\s+{LABEL}",
        cmake,
        re.DOTALL,
    ):
        failures.append("scenario matrix CTest is missing its capability label")
    if failures:
        for failure in failures:
            print(failure)
        return 1
    print(f"ok: all {len(expected)} language-binding scenarios have mapped CI evidence")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
