#!/usr/bin/env python3
"""Decide the platform packages this release slice claims.

`build-packaging` requires a package per platform containing the header, the
library, the licence and the attribution file, each smoke-tested by a program
that links it. This gate decides only the platforms declared in scope and
reports the deferred ones by name with the decision that deferred them, so an
unexercised or absent package can never be published as passed.

Evidence is the package manifest the builder writes beside the installed
licence, which records the preset and whether its smoke test linked or executed.
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
REQUIRED_FILES = (
    "include/ctex/capi.h",
    "include/ctex/version.h",
    "share/cybertexel/LICENSE",
    "share/cybertexel/THIRD_PARTY_NOTICES.md",
    "share/cybertexel/package.json",
)


class ScopeError(RuntimeError):
    pass


def load_scope(root: Path = ROOT) -> dict[str, object]:
    manifest = json.loads((root / "release" / "platforms.json").read_text(encoding="utf-8"))
    if manifest.get("schema") != 1:
        raise ScopeError("release platform manifest must use schema 1")
    in_scope = manifest.get("in_scope")
    deferred = manifest.get("deferred")
    if not isinstance(in_scope, list) or not in_scope:
        raise ScopeError("release platform manifest declares no in-scope platform")
    if not isinstance(deferred, dict):
        raise ScopeError("release platform manifest must map each deferred platform to a decision")
    for preset, decision in deferred.items():
        if not isinstance(decision, str) or not decision.strip():
            raise ScopeError(f"deferred platform {preset} records no decision")
    for entry in in_scope:
        for field in ("preset", "kind", "recipe", "smoke_test"):
            if not entry.get(field):
                raise ScopeError(f"in-scope platform entry is missing {field!r}")
        if entry["smoke_test"] not in {"linked", "executed"}:
            raise ScopeError(f"{entry['preset']} declares an unknown smoke-test outcome")
    presets = [entry["preset"] for entry in in_scope]
    overlap = sorted(set(presets) & set(deferred))
    if overlap:
        raise ScopeError(f"platforms are both in scope and deferred: {', '.join(overlap)}")
    if len(presets) != len(set(presets)):
        raise ScopeError("release platform manifest repeats a preset")
    return manifest


def package_failures(root: Path = ROOT, only: str | None = None) -> tuple[list[str], list[str]]:
    manifest = load_scope(root)
    version = (root / "VERSION").read_text(encoding="utf-8").strip()
    failures: list[str] = []
    decided: list[str] = []
    selected = [e for e in manifest["in_scope"] if only is None or e["preset"] == only]
    if only is not None and not selected:
        raise ScopeError(f"{only} is not an in-scope release platform")
    for entry in selected:
        preset = entry["preset"]
        prefix = root / "build" / "packages" / preset / "root"
        archive = root / "dist" / f"cybertexel-{version}-{preset}.zip"
        if not prefix.is_dir():
            failures.append(f"{preset}: no installed package; run `just {entry['recipe']}`")
            continue
        missing = [name for name in REQUIRED_FILES if not (prefix / name).is_file()]
        if missing:
            failures.append(f"{preset}: package is incomplete: {', '.join(missing)}")
            continue
        package = json.loads((prefix / "share" / "cybertexel" / "package.json").read_text("utf-8"))
        if package.get("platform") != preset:
            failures.append(f"{preset}: package manifest names platform {package.get('platform')!r}")
            continue
        if package.get("version") != version:
            failures.append(
                f"{preset}: package version {package.get('version')!r} is not VERSION {version!r}"
            )
            continue
        if package.get("smoke_test") != entry["smoke_test"]:
            failures.append(
                f"{preset}: smoke test recorded {package.get('smoke_test')!r}, "
                f"the scope requires {entry['smoke_test']!r}"
            )
            continue
        if not archive.is_file():
            failures.append(f"{preset}: package was built but {archive.name} was not archived")
            continue
        decided.append(f"{preset} ({entry['kind']}, smoke test {entry['smoke_test']})")
    return failures, decided


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--preset",
        help="decide only this in-scope platform; omit to decide every one",
    )
    arguments = parser.parse_args()
    try:
        failures, decided = package_failures(only=arguments.preset)
        manifest = load_scope()
    except (OSError, ScopeError, json.JSONDecodeError) as error:
        print(f"package gate failed: {error}", file=sys.stderr)
        return 1
    for preset, decision in sorted(manifest["deferred"].items()):
        print(f"deferred: {preset} — {decision}")
    if failures:
        print("package gate failed:", file=sys.stderr)
        for failure in failures:
            print(f"- {failure}", file=sys.stderr)
        return 1
    for entry in decided:
        print(f"ok: {entry}")
    scope = arguments.preset or "every in-scope platform"
    print(f"package gate passed: {len(decided)} platform(s) decided ({scope})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
