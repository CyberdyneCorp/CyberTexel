#!/usr/bin/env python3
"""Run numbered Python examples and assert, compare, or update their outputs."""

from __future__ import annotations

import argparse
import json
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parent
OUTPUTS = ROOT / "outputs"
OUTPUT_TOLERANCES = ROOT / "output_tolerances.json"


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


def load_tolerances(path: Path = OUTPUT_TOLERANCES) -> dict[str, object]:
    if not path.is_file():
        return {"default": {"mode": "bytes"}, "overrides": {}}
    manifest = json.loads(path.read_text(encoding="utf-8"))
    if manifest.get("schema") != 1:
        raise ExampleFailure("output tolerance manifest must use schema 1")
    default = manifest.get("default")
    overrides = manifest.get("overrides")
    if default != {"mode": "bytes"} or not isinstance(overrides, dict):
        raise ExampleFailure("output tolerance manifest has an invalid policy")
    for name, policy in overrides.items():
        if not isinstance(name, str) or not isinstance(policy, dict):
            raise ExampleFailure("output tolerance override must map a path to a policy")
        if policy.get("mode") != "pixels" or policy.get("maximum_absolute_error") not in range(
            256
        ):
            raise ExampleFailure(f"invalid pixel tolerance for {name}")
    return manifest


def _maximum_pixel_difference(actual: bytes, expected: bytes, name: str) -> int:
    import cybertexel
    import numpy as np

    actual_image = cybertexel.decode_image(actual, source_name=name)
    expected_image = cybertexel.decode_image(expected, source_name=name)
    if actual_image.pixels.shape != expected_image.pixels.shape:
        raise ExampleFailure(
            f"committed image shape changed for {name}: "
            f"{actual_image.pixels.shape} != {expected_image.pixels.shape}"
        )
    difference = np.abs(
        actual_image.pixels.astype(np.float64) - expected_image.pixels.astype(np.float64)
    )
    return int(difference.max(initial=0))


def compare_outputs(
    actual: Path,
    expected: Path,
    *,
    example_name: str = "",
    tolerances: dict[str, object] | None = None,
) -> None:
    actual_files = output_files(actual)
    expected_files = output_files(expected) if expected.is_dir() else {}
    if actual_files.keys() != expected_files.keys():
        missing = sorted(actual_files.keys() - expected_files.keys())
        stale = sorted(expected_files.keys() - actual_files.keys())
        raise ExampleFailure(f"output file set changed; missing={missing}, stale={stale}")
    policies = tolerances or {"default": {"mode": "bytes"}, "overrides": {}}
    overrides = policies["overrides"]
    changed = []
    for name in actual_files:
        if actual_files[name] == expected_files[name]:
            continue
        policy_name = f"{example_name}/{name}" if example_name else name
        policy = overrides.get(policy_name, policies["default"])
        if policy["mode"] == "pixels":
            difference = _maximum_pixel_difference(
                actual_files[name], expected_files[name], policy_name
            )
            if difference <= policy["maximum_absolute_error"]:
                continue
            changed.append(f"{name} (maximum pixel error {difference})")
        else:
            changed.append(name)
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


def execute_example(script: Path, output: Path, executor: str) -> None:
    environment = os.environ.copy()
    environment.update(
        {
            "CTEX_EXECUTOR": executor,
            "CTEX_EXAMPLE_EXECUTOR": executor,
            "CTEX_EXAMPLE_SEED": "1729",
            "PYTHONHASHSEED": "0",
        }
    )
    trace = environment.get("CTEX_SYMBOL_TRACE")
    if trace:
        # sitecustomize installs the C ABI call recorder before cybertexel loads.
        recorder = ROOT.parent / "tools" / "example_trace"
        existing = environment.get("PYTHONPATH")
        environment["PYTHONPATH"] = (
            f"{recorder}{os.pathsep}{existing}" if existing else str(recorder)
        )
        environment["CTEX_SYMBOL_TRACE_NAME"] = script.stem
    completed = subprocess.run(
        [sys.executable, str(script), "--output", str(output), "--executor", executor],
        cwd=ROOT.parent,
        env=environment,
        text=True,
    )
    if completed.returncode != 0:
        raise ExampleFailure(f"{script.name} failed with exit code {completed.returncode}")
    if not output_files(output):
        raise ExampleFailure(f"{script.name} produced no output")


def run_example(script: Path, mode: str, executor: str, outputs: Path = OUTPUTS) -> None:
    with tempfile.TemporaryDirectory(prefix=f"ctex-{script.stem}-") as directory:
        actual = Path(directory)
        execute_example(script, actual, executor)
        expected = outputs / script.stem
        if mode == "compare":
            with tempfile.TemporaryDirectory(prefix=f"ctex-{script.stem}-repeat-") as repeat:
                repeated = Path(repeat)
                execute_example(script, repeated, executor)
                if output_files(actual) != output_files(repeated):
                    raise ExampleFailure(f"{script.name} output is not deterministic")
                compare_outputs(
                    actual,
                    expected,
                    example_name=script.stem,
                    tolerances=load_tolerances(),
                )
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
