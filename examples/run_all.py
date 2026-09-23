#!/usr/bin/env python3
"""Run numbered Python examples and assert, compare, or update their outputs."""

from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parent
OUTPUTS = ROOT / "outputs"


class ExampleFailure(RuntimeError):
    pass


def discover_examples(root: Path = ROOT) -> list[Path]:
    examples = sorted(root.glob("[0-9][0-9]_*.py"))
    if not examples:
        raise ExampleFailure("no numbered examples were found")
    ordinals = [path.name.split("_", 1)[0] for path in examples]
    if len(ordinals) != len(set(ordinals)):
        raise ExampleFailure("numbered examples repeat an ordinal")
    return examples


def output_files(root: Path) -> dict[str, bytes]:
    return {
        path.relative_to(root).as_posix(): path.read_bytes()
        for path in sorted(root.rglob("*"))
        if path.is_file()
    }


def compare_outputs(actual: Path, expected: Path) -> None:
    actual_files = output_files(actual)
    expected_files = output_files(expected) if expected.is_dir() else {}
    if actual_files.keys() != expected_files.keys():
        missing = sorted(actual_files.keys() - expected_files.keys())
        stale = sorted(expected_files.keys() - actual_files.keys())
        raise ExampleFailure(f"output file set changed; missing={missing}, stale={stale}")
    changed = [name for name in actual_files if actual_files[name] != expected_files[name]]
    if changed:
        raise ExampleFailure(f"committed output changed: {changed}")


def update_outputs(actual: Path, expected: Path) -> None:
    expected.parent.mkdir(parents=True, exist_ok=True)
    staged = expected.with_name(f".{expected.name}.staged")
    if staged.exists():
        shutil.rmtree(staged)
    shutil.copytree(actual, staged)
    if expected.exists():
        shutil.rmtree(expected)
    staged.replace(expected)


def run_example(script: Path, mode: str, executor: str, outputs: Path = OUTPUTS) -> None:
    with tempfile.TemporaryDirectory(prefix=f"ctex-{script.stem}-") as directory:
        actual = Path(directory)
        environment = os.environ.copy()
        environment.update(
            {
                "CTEX_EXAMPLE_EXECUTOR": executor,
                "CTEX_EXAMPLE_SEED": "1729",
                "PYTHONHASHSEED": "0",
            }
        )
        completed = subprocess.run(
            [sys.executable, str(script), "--output", str(actual), "--executor", executor],
            cwd=ROOT.parent,
            env=environment,
            text=True,
        )
        if completed.returncode != 0:
            raise ExampleFailure(f"{script.name} failed with exit code {completed.returncode}")
        if not output_files(actual):
            raise ExampleFailure(f"{script.name} produced no output")
        expected = outputs / script.stem
        if mode == "compare":
            compare_outputs(actual, expected)
        elif mode == "update":
            if executor != "cpu":
                raise ExampleFailure("committed outputs may only be updated from the CPU executor")
            update_outputs(actual, expected)


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("mode", choices=("assert", "compare", "update"), nargs="?", default="compare")
    parser.add_argument("--executor", default="cpu")
    parser.add_argument("--example", action="append", default=[], help="numbered script stem")
    return parser.parse_args()


def main() -> int:
    arguments = parse_arguments()
    try:
        examples = discover_examples()
        if arguments.example:
            selected = set(arguments.example)
            examples = [path for path in examples if path.stem in selected]
            missing = selected - {path.stem for path in examples}
            if missing:
                raise ExampleFailure(f"unknown examples: {sorted(missing)}")
        for script in examples:
            run_example(script, arguments.mode, arguments.executor)
            print(f"ok: {script.stem} ({arguments.mode}, {arguments.executor})")
    except ExampleFailure as error:
        print(f"examples failed: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
