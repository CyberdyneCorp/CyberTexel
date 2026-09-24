#!/usr/bin/env python3

from __future__ import annotations

import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
MATRIX = ROOT / "docs" / "cli-headless-scenarios.md"
SPEC = (
    ROOT
    / "openspec"
    / "specs"
    / "cli-headless"
    / "spec.md"
)
LABEL = "cli-headless-scenario"


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
            raise ValueError(f"scenario {cells[0]!r} has empty executable evidence")
        rows.append((cells[0], cells[1]))
    return rows


def labeled_ctests(cmake: str) -> set[str]:
    labeled: set[str] = set()
    for match in re.finditer(
        r"(?:set_tests_properties|set_property)\((?P<body>.*?)\)", cmake, re.DOTALL
    ):
        body = match.group("body")
        if "LABELS" not in body or LABEL not in body:
            continue
        tests = re.split(r"\b(?:PROPERTIES|APPEND PROPERTY)\b", body, maxsplit=1)[0]
        labeled.update(name for name in tests.split() if name != "TEST")
    return labeled


def desktop_gate_is_wired() -> bool:
    recipe = (ROOT / "justfile").read_text(encoding="utf-8")
    workflow = (ROOT / ".github" / "workflows" / "openspec-validate.yml").read_text(
        encoding="utf-8"
    )
    required_targets = (
        "cybertexel_cli",
        "ctex_texture_document_io_test",
        "cybertexel_c",
        "ctex_cli_bake_provider",
        "ctex_cli_obj_mesh_test",
    )
    return (
        "\ntest-cli:" in recipe
        and all(target in recipe for target in required_targets)
        and "-R '^cli-headless-'" in recipe
        and "os: [ubuntu-latest, macos-15-intel]" in workflow
        and "run: just test-cli" in workflow
    )


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

    cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    registered = set(re.findall(r"add_test\(\s*NAME\s+([^\s\)]+)", cmake))
    labeled = labeled_ctests(cmake)
    for scenario, evidence in rows:
        for command in re.findall(r"`([^`]+)`", evidence):
            if command == "just test-cli":
                if not desktop_gate_is_wired():
                    failures.append("desktop CLI recipe is not wired into the CI OS matrix")
            elif command not in registered:
                failures.append(f"{scenario!r} names unknown CTest {command!r}")
            elif command not in labeled:
                failures.append(f"{scenario!r} names unlabeled CTest {command!r}")
    if "cli-headless-scenario-matrix" not in labeled:
        failures.append("scenario matrix CTest is missing its capability label")
    if failures:
        for failure in failures:
            print(failure)
        return 1
    print(f"ok: all {len(expected)} cli-headless scenarios have mapped evidence")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
