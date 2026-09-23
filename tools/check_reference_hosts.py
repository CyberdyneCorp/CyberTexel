#!/usr/bin/env python3
"""Check that the reference hosts stay honest about the emitted pass plan.

`build-packaging` requires desktop and mobile reference hosts that exercise the
host-executed route on real device APIs and are built in CI, and requires that a
change to the pass plan's contents break them rather than be ignored.

Both hosts parse the plan strictly, so a new field fails at run time. This gate
makes the same thing fail at check time: every field the emitted plan carries
must be named in both host sources, and both hosts must be reachable as recipes
that CI invokes.
"""

from __future__ import annotations

import json
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
PLAN = ROOT / "examples" / "outputs" / "04_software_host_execution" / "pass_plan.json"
HOSTS = {
    "desktop-wgpu": (
        ROOT / "hosts" / "desktop-wgpu" / "src" / "plan.rs",
        ROOT / "hosts" / "desktop-wgpu" / "src" / "main.rs",
    ),
    "ipad-metal": (
        ROOT / "hosts" / "ipad-metal" / "Sources" / "CyberTexelHostMetal" / "PassPlan.swift",
        ROOT / "hosts" / "ipad-metal" / "Sources" / "CyberTexelHostMetal" / "main.swift",
    ),
}
RECIPES = ("host-desktop", "host-desktop-build", "host-ipad", "hosts")


def plan_field_names(plan: dict[str, object]) -> set[str]:
    """The structural contract a host must account for.

    The plan's own fields, each pass's fields and each resource's fields. A new
    element here is new structure the host has to handle or explicitly refuse.
    Values a host treats as opaque are not walked into: how a blend mode or a
    subresource range is spelled is not the host's structural concern.
    """
    names = set(plan.keys())
    for pass_entry in plan.get("passes", []):
        names.update(pass_entry.keys())
    for resource in plan.get("resources", []):
        names.update(resource.keys())
    return names


def failures(root: Path = ROOT) -> list[str]:
    problems: list[str] = []
    plan_path = root / PLAN.relative_to(ROOT)
    if not plan_path.is_file():
        return [f"no committed pass plan to check the hosts against: {plan_path}"]
    fields = plan_field_names(json.loads(plan_path.read_text(encoding="utf-8")))

    for host, sources in HOSTS.items():
        text = ""
        for source in sources:
            path = root / source.relative_to(ROOT)
            if not path.is_file():
                problems.append(f"{host}: missing source {path.relative_to(root)}")
                continue
            text += path.read_text(encoding="utf-8")
        if not text:
            continue
        absent = sorted(name for name in fields if name not in text)
        if absent:
            problems.append(
                f"{host} does not name pass-plan fields it must account for: {', '.join(absent)}"
            )

    strict = root / "hosts" / "desktop-wgpu" / "src" / "plan.rs"
    if strict.is_file() and "deny_unknown_fields" not in strict.read_text(encoding="utf-8"):
        problems.append("the desktop host no longer rejects unknown pass-plan fields")

    justfile = (root / "justfile").read_text(encoding="utf-8")
    for recipe in RECIPES:
        if f"\n{recipe}:" not in justfile and f"\n{recipe} " not in justfile:
            problems.append(f"the {recipe!r} recipe is missing")

    invoked = ""
    for workflow in sorted((root / ".github" / "workflows").glob("*.yml")):
        invoked += workflow.read_text(encoding="utf-8")
    # CI may name the recipe directly or through a job matrix entry.
    for recipe in ("host-desktop", "host-ipad"):
        if f"just {recipe}" not in invoked and f"recipe: {recipe}" not in invoked:
            problems.append(f"no CI workflow runs the {recipe!r} recipe")
    return problems


def main() -> int:
    try:
        problems = failures()
    except (OSError, json.JSONDecodeError) as error:
        print(f"reference host gate failed: {error}", file=sys.stderr)
        return 1
    if problems:
        print("reference host gate failed:", file=sys.stderr)
        for problem in problems:
            print(f"- {problem}", file=sys.stderr)
        return 1
    print("reference host gate passed: desktop WGSL and mobile MSL hosts account for the plan")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
