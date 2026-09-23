#!/usr/bin/env python3
"""Process-level regression tests for the headless CLI skeleton."""

from __future__ import annotations

import json
import subprocess
import sys


BINARY = sys.argv[1]
COMMANDS = ("export", "bake-request", "apply", "run", "info", "validate")


def run(*arguments: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run([BINARY, *arguments], capture_output=True, text=True, check=False)


help_result = run("--help")
assert help_result.returncode == 0 and help_result.stderr == ""
for command in COMMANDS:
    assert command in help_result.stdout

missing = run("export")
assert missing.returncode == 2
assert "--document" in missing.stderr

unknown = run("wat")
assert unknown.returncode == 2
assert "accepted commands" in unknown.stderr

invalid = run("validate", "--input", "asset.ctex", "--kind", "texture")
assert invalid.returncode == 2
assert "document, material, preset" in invalid.stderr

duplicate = run(
    "info", "--document", "a.ctex", "--document", "b.ctex"
)
assert duplicate.returncode == 2 and "duplicate option" in duplicate.stderr

dispatched = run(
    "info", "--document", "asset.ctex", "--report", "json", "--quiet"
)
assert dispatched.returncode == 4
assert json.loads(dispatched.stdout) == {
    "command": "info",
    "executor": "cpu",
    "status": "unsupported",
}
assert "roadmap task 15.2" in dispatched.stderr

print("headless CLI skeleton passed")
