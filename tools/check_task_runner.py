#!/usr/bin/env python3
"""Check the task runner contract that `build-packaging` states.

The justfile is the single definition of every routine command: each gate the
specification names must be reachable as a recipe, an unimplemented gate must
fail and name the task that delivers it, a recipe must report a missing
prerequisite by name, and a CI step must invoke a recipe rather than repeat its
command — otherwise the same command is defined in two places and can drift.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]

# Named by build-packaging, "One task runner is the entry point".
REQUIRED_RECIPES = (
    "build",
    "test",
    "format",
    "format-check",
    "examples",
    "bench",
    "clean",
    "check",
    "gate-layering",
    "gate-licence",
    "gate-parity",
    "gate-determinism",
    "gate-budgets",
    "gate-binding-parity",
    "gate-example-coverage",
    "gate-capability-index",
    "gate-version-consistency",
    "gate-abi-diff",
    "gate-packages",
    "gate-reproducible",
    "gate-release-assets",
    "test-sanitize",
    "fuzz-project-container",
    "fuzz-image-decoders",
)

# Gates whose value is that CI actually runs them, not that they exist.
REQUIRED_IN_CI = (
    "test-sanitize",
    "fuzz-project-container",
    "fuzz-image-decoders",
    "gate-layering",
    "gate-licence",
    "gate-abi-diff",
    "gate-version-consistency",
    "gate-release-assets",
)

# Steps that install a toolchain are not project commands and have no recipe.
TOOLCHAIN_PREFIXES = (
    "sudo apt-get",
    "apt-get",
    "brew ",
    "sdkmanager ",
    "echo ",
    "rustup ",
    "pip install",
    "npm install",
    "npm ci",
)


def recipes(justfile: str) -> dict[str, str]:
    """Recipe name -> its body."""
    found: dict[str, str] = {}
    lines = justfile.splitlines()
    for index, line in enumerate(lines):
        match = re.match(r"^([a-zA-Z0-9_-]+)(?:\s+[^:]*)?:(?!=)", line)
        if not match:
            continue
        body: list[str] = []
        for following in lines[index + 1 :]:
            if following and not following[0].isspace():
                break
            body.append(following)
        found[match.group(1)] = "\n".join(body)
    return found


def workflow_steps(root: Path) -> list[tuple[str, str]]:
    """(workflow file, command) for every `run:` step in every workflow."""
    steps: list[tuple[str, str]] = []
    for path in sorted((root / ".github" / "workflows").glob("*.yml")):
        text = path.read_text(encoding="utf-8")
        for match in re.finditer(r"^(\s*)-?\s*run:\s*(\|?)\s*(.*)$", text, re.MULTILINE):
            indent, block, first = match.groups()
            if not block:
                steps.append((path.name, first.strip()))
                continue
            offset = match.end()
            for line in text[offset:].splitlines():
                if line.strip() and not line.startswith(indent + "  "):
                    break
                if line.strip():
                    steps.append((path.name, line.strip()))
    return steps


def failures(root: Path = ROOT) -> list[str]:
    problems: list[str] = []
    justfile = (root / "justfile").read_text(encoding="utf-8")
    defined = recipes(justfile)

    missing = [name for name in REQUIRED_RECIPES if name not in defined]
    if missing:
        problems.append(f"the specification names gates with no recipe: {', '.join(missing)}")

    for name, body in sorted(defined.items()):
        if "_unimplemented" not in body or name == "_unimplemented":
            continue
        if not re.search(r"_unimplemented\s+\S+\s+\d+\.\d+", body):
            problems.append(f"recipe {name!r} defers to _unimplemented without naming its task")

    helper = defined.get("_require", "")
    if "missing prerequisite" not in helper or "{{tool}}" not in helper:
        problems.append("_require does not report the missing tool by name")

    invoked = {
        command.split()[1]
        for _, command in workflow_steps(root)
        if command.startswith("just ") and len(command.split()) > 1
    }
    absent = [name for name in REQUIRED_IN_CI if name not in invoked]
    if absent:
        problems.append(f"no CI workflow invokes: {', '.join(absent)}")

    for workflow, command in workflow_steps(root):
        if command.startswith("just ") or command == "just":
            continue
        if any(command.startswith(prefix) for prefix in TOOLCHAIN_PREFIXES):
            continue
        problems.append(
            f"{workflow} runs {command!r}, which is not a just recipe; "
            "the same command would then be defined in two places"
        )
    return problems


def main() -> int:
    try:
        problems = failures()
    except OSError as error:
        print(f"task runner gate failed: {error}", file=sys.stderr)
        return 1
    if problems:
        print("task runner gate failed:", file=sys.stderr)
        for problem in problems:
            print(f"- {problem}", file=sys.stderr)
        return 1
    print(f"task runner gate passed: {len(REQUIRED_RECIPES)} named recipes, CI invokes recipes only")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
