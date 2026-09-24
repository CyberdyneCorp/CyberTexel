#!/usr/bin/env python3
"""Check that the proposal, the spec directories and the task plan agree.

Three things drift apart on a spec-first repository: a capability named in the
proposal with no spec written, a spec directory nobody declared, and a
capability with no task group to deliver it. This fails the build on any of
them, which is cheap now and stops being cheap once there is code.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

CHANGE = Path("openspec/changes/archive/2026-09-24-bootstrap-v1-cybertexel")
SPECS = Path("openspec/specs")


def declared_in_proposal() -> set[str]:
    text = (CHANGE / "proposal.md").read_text(encoding="utf-8")
    start = text.index("### New Capabilities")
    end = text.index("### Modified Capabilities")
    return set(re.findall(r"^- `([a-z0-9-]+)`:", text[start:end], re.MULTILINE))


def spec_directories() -> set[str]:
    return {p.parent.name for p in (CHANGE / "specs").glob("*/spec.md")}


def mentioned_in_tasks() -> set[str]:
    """Capabilities a task group promises to turn into tests.

    A task line may name several, as in "`a` and `b` scenarios as tests", so
    collect every backticked name on a line that makes the promise rather than
    only the one adjacent to it.
    """
    text = (CHANGE / "tasks.md").read_text(encoding="utf-8")
    named: set[str] = set()
    for line in text.splitlines():
        if "scenarios as tests" in line:
            named.update(re.findall(r"`([a-z0-9-]+)`", line))
    return named


def spec_has_content(name: str) -> tuple[int, int]:
    text = (SPECS / name / "spec.md").read_text(encoding="utf-8")
    return (
        len(re.findall(r"^### Requirement:", text, re.MULTILINE)),
        len(re.findall(r"^#### Scenario:", text, re.MULTILINE)),
    )


def main() -> int:
    if not CHANGE.is_dir():
        print(f"error: {CHANGE} not found; run from the repository root", file=sys.stderr)
        return 2

    proposal = declared_in_proposal()
    specs = spec_directories()
    tasks = mentioned_in_tasks()
    failures: list[str] = []

    for name in sorted(proposal - specs):
        failures.append(f"declared in proposal.md but has no specs/{name}/spec.md")
    for name in sorted(specs - proposal):
        failures.append(f"specs/{name}/spec.md exists but is not declared in proposal.md")
    for name in sorted(specs - tasks):
        failures.append(f"specs/{name}/spec.md has no task group turning its scenarios into tests")
    for name in sorted(specs):
        if not (SPECS / name / "spec.md").is_file():
            failures.append(f"founding spec {name} is missing from official specs")

    for name in sorted(specs):
        if not (SPECS / name / "spec.md").is_file():
            continue
        requirements, scenarios = spec_has_content(name)
        if requirements == 0:
            failures.append(f"specs/{name}/spec.md declares no requirements")
        if scenarios < requirements:
            failures.append(
                f"specs/{name}/spec.md has {requirements} requirements but only "
                f"{scenarios} scenarios; every requirement needs at least one"
            )

    if failures:
        print("capability index check failed:", file=sys.stderr)
        for line in failures:
            print(f"  - {line}", file=sys.stderr)
        return 1

    total_requirements = sum(spec_has_content(n)[0] for n in specs)
    total_scenarios = sum(spec_has_content(n)[1] for n in specs)
    print(
        f"ok: {len(specs)} capabilities, {total_requirements} requirements, "
        f"{total_scenarios} scenarios, all declared and all scheduled"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
