#!/usr/bin/env python3
"""Check that the numbered Python examples exercise the public C ABI surface.

Evidence is a call trace recorded while the examples run, not a declaration: a
symbol counts as exercised only when an example actually called it. Symbols that
no example reaches yet must be named in ``examples/feature_coverage.json`` with
the task that closes them, so the uncovered surface is reported rather than
silently absent.
"""

from __future__ import annotations

import json
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_TRACES = ROOT / "build" / "example-traces"


def manifest_symbols(root: Path = ROOT) -> dict[str, tuple[str, str]]:
    manifest = json.loads((root / "abi" / "capi-capabilities.json").read_text(encoding="utf-8"))
    symbols: dict[str, tuple[str, str]] = {}
    for capability in manifest["capabilities"]:
        for requirement in capability.get("requirements", []):
            for symbol in requirement.get("symbols", []):
                symbols.setdefault(symbol, (capability["name"], requirement["requirement"]))
    if not symbols:
        raise ValueError("capability manifest declares no C ABI symbols")
    return symbols


def traced_symbols(directory: Path) -> tuple[set[str], set[str]]:
    if not directory.is_dir():
        raise ValueError(f"no example call traces in {directory}; run `just examples` first")
    called: set[str] = set()
    examples: set[str] = set()
    for path in sorted(directory.glob("*.json")):
        trace = json.loads(path.read_text(encoding="utf-8"))
        if trace.get("schema") != 1:
            raise ValueError(f"{path.name} is not a schema-1 call trace")
        examples.add(trace["example"])
        called.update(trace["symbols"])
    if not called:
        raise ValueError(f"example call traces in {directory} recorded no C ABI calls")
    return called, examples


def deferred_symbols(root: Path = ROOT) -> dict[str, str]:
    path = root / "examples" / "feature_coverage.json"
    manifest = json.loads(path.read_text(encoding="utf-8"))
    if manifest.get("schema") != 1:
        raise ValueError("feature coverage manifest must use schema 1")
    deferred = manifest.get("deferred")
    if not isinstance(deferred, dict):
        raise ValueError("feature coverage manifest must map each deferred symbol to a reason")
    for symbol, reason in deferred.items():
        if not isinstance(reason, str) or not reason.strip():
            raise ValueError(f"deferred symbol {symbol} has no stated reason")
    return deferred


def failures(traces: Path, root: Path = ROOT) -> list[str]:
    problems: list[str] = []
    symbols = manifest_symbols(root)
    called, examples = traced_symbols(traces)
    deferred = deferred_symbols(root)

    numbered = {path.stem for path in sorted((root / "examples").glob("[0-9][0-9]_*.py"))}
    absent = sorted(numbered - examples)
    if absent:
        problems.append(f"numbered examples produced no call trace: {', '.join(absent)}")

    unknown = sorted(set(deferred) - set(symbols))
    if unknown:
        problems.append(f"deferred symbols are not in the capability manifest: {', '.join(unknown)}")

    stale = sorted(set(deferred) & called)
    if stale:
        problems.append(
            "deferred symbols are now exercised and must leave "
            f"examples/feature_coverage.json: {', '.join(stale)}"
        )

    missing = sorted(set(symbols) - called - set(deferred))
    if missing:
        listed = ", ".join(f"{name} ({symbols[name][0]})" for name in missing)
        problems.append(f"C ABI symbols no example exercises and none declares deferred: {listed}")
    return problems


def report(traces: Path, root: Path = ROOT) -> None:
    symbols = manifest_symbols(root)
    called, _ = traced_symbols(traces)
    deferred = deferred_symbols(root)
    exercised = len(set(symbols) & called)
    print(
        f"example feature coverage: {exercised}/{len(symbols)} C ABI symbols exercised, "
        f"{len(deferred)} declared deferred"
    )


def main() -> int:
    traces = Path(sys.argv[1]) if len(sys.argv) > 1 else DEFAULT_TRACES
    try:
        problems = failures(traces)
    except (OSError, ValueError, json.JSONDecodeError) as error:
        print(f"example feature coverage gate failed: {error}", file=sys.stderr)
        return 1
    if problems:
        print("example feature coverage gate failed:", file=sys.stderr)
        for problem in problems:
            print(f"- {problem}", file=sys.stderr)
        return 1
    report(traces)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
