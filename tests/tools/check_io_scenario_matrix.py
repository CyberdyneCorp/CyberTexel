#!/usr/bin/env python3

from __future__ import annotations

import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
MATRIX = ROOT / "docs" / "io-scenarios.md"
CAPABILITIES = {
    "Project I/O": ROOT
    / "openspec"
    / "changes"
    / "bootstrap-v1-cybertexel"
    / "specs"
    / "project-io"
    / "spec.md",
    "Texture export": ROOT
    / "openspec"
    / "changes"
    / "bootstrap-v1-cybertexel"
    / "specs"
    / "texture-export"
    / "spec.md",
}


def spec_scenarios(path: Path) -> list[str]:
    pattern = re.compile(r"^#### Scenario: (.+)$")
    return [
        match.group(1)
        for line in path.read_text(encoding="utf-8").splitlines()
        if (match := pattern.match(line))
    ]


def matrix_rows(text: str, section: str) -> list[tuple[str, str]]:
    lines = text.splitlines()
    heading = f"## {section}"
    try:
        start = lines.index(heading) + 1
    except ValueError as error:
        raise ValueError(f"scenario matrix is missing section {section!r}") from error
    rows: list[tuple[str, str]] = []
    for line in lines[start:]:
        if line.startswith("## "):
            break
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


def main() -> int:
    matrix = MATRIX.read_text(encoding="utf-8")
    failures: list[str] = []
    ctests = registered_ctests()
    for section, spec in CAPABILITIES.items():
        expected = spec_scenarios(spec)
        rows = matrix_rows(matrix, section)
        documented = [name for name, _ in rows]
        missing = sorted(set(expected) - set(documented))
        unexpected = sorted(set(documented) - set(expected))
        duplicates = sorted({name for name in documented if documented.count(name) > 1})
        if missing:
            failures.append(f"{section}: missing scenarios: {', '.join(missing)}")
        if unexpected:
            failures.append(f"{section}: unknown scenarios: {', '.join(unexpected)}")
        if duplicates:
            failures.append(f"{section}: duplicate scenarios: {', '.join(duplicates)}")
        for scenario, evidence in rows:
            for command in re.findall(r"`([^`]+)`", evidence):
                if not command.startswith("just ") and command not in ctests:
                    failures.append(
                        f"{section}: {scenario!r} names unknown CTest {command!r}"
                    )
    if failures:
        for failure in failures:
            print(failure)
        return 1
    total = sum(len(spec_scenarios(path)) for path in CAPABILITIES.values())
    print(f"ok: all {total} project-I/O and texture-export scenarios have mapped evidence")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
