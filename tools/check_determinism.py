#!/usr/bin/env python3
"""Run registered outputs twice and require byte-identical results."""

from __future__ import annotations

import json
import os
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "tests" / "determinism" / "cases.json"


def compare_outputs(
    first: Path, second: Path, outputs: list[str], case_name: str
) -> list[str]:
    failures: list[str] = []
    for output in outputs:
        first_path = first / output
        second_path = second / output
        if not first_path.is_file() or not second_path.is_file():
            failures.append(f"{case_name}: output was not produced twice: {output}")
        elif first_path.read_bytes() != second_path.read_bytes():
            failures.append(f"{case_name}: output is not byte-identical: {output}")
    return failures


def run_once(root: Path, command: list[str], output_directory: Path) -> str | None:
    environment = os.environ.copy()
    environment["CTEX_DETERMINISM_OUTPUT_DIR"] = str(output_directory)
    result = subprocess.run(
        command,
        cwd=root,
        env=environment,
        check=False,
        capture_output=True,
        text=True,
    )
    if result.returncode == 0:
        return None
    detail = result.stderr.strip() or result.stdout.strip()
    return f"command exited {result.returncode}: {detail}"


def run_case(root: Path, case: dict[str, Any]) -> list[str]:
    name = case.get("name", "unnamed case")
    command = case.get("command")
    outputs = case.get("outputs")
    if not isinstance(command, list) or not all(isinstance(part, str) for part in command):
        return [f"{name}: command must be an array of strings"]
    if not isinstance(outputs, list) or not outputs or not all(
        isinstance(output, str) for output in outputs
    ):
        return [f"{name}: outputs must be a non-empty array of paths"]

    with tempfile.TemporaryDirectory() as temporary_directory:
        temporary_root = Path(temporary_directory)
        first = temporary_root / "first"
        second = temporary_root / "second"
        first.mkdir()
        second.mkdir()
        for output_directory in (first, second):
            failure = run_once(root, command, output_directory)
            if failure:
                return [f"{name}: {failure}"]
        return compare_outputs(first, second, outputs, str(name))


def check(
    root: Path, manifest_path: Path, selected_categories: set[str] | None = None
) -> list[str]:
    try:
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    except (json.JSONDecodeError, OSError) as error:
        return [f"determinism manifest is invalid: {error}"]
    if manifest.get("schema") != 1 or not isinstance(manifest.get("categories"), list):
        return ["determinism manifest must use schema 1 with a categories array"]

    failures: list[str] = []
    discovered: set[str] = set()
    for category in manifest["categories"]:
        name = category.get("name", "unnamed category")
        discovered.add(name)
        if selected_categories is not None and name not in selected_categories:
            continue
        task = category.get("task", "unknown")
        cases = category.get("cases")
        if not isinstance(cases, list) or not cases:
            failures.append(f"{name}: no determinism cases registered; delivered by task {task}")
            continue
        for case in cases:
            if not isinstance(case, dict):
                failures.append(f"{name}: case is not an object")
                continue
            failures.extend(run_case(root, case))
    if selected_categories is not None:
        for missing in sorted(selected_categories - discovered):
            failures.append(f"unknown determinism category: {missing}")
    return failures


def main() -> int:
    selected = set(sys.argv[1:]) or None
    failures = check(ROOT, MANIFEST, selected)
    if failures:
        print("determinism gate failed:", file=sys.stderr)
        for failure in failures:
            print(f"  - {failure}", file=sys.stderr)
        return 1
    print("ok: every registered output was byte-identical across two runs")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
