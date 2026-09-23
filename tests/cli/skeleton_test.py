#!/usr/bin/env python3
"""Process-level regression tests for the headless CLI skeleton."""

from __future__ import annotations

import json
from pathlib import Path
import subprocess
import sys


BINARY = sys.argv[1]
ROOT = Path(sys.argv[2])
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

document = ROOT / "examples" / "outputs" / "01_version_and_project_container" / "empty.ctex"
dispatched = run(
    "info", "--document", str(document), "--report", "json", "--quiet"
)
assert dispatched.returncode == 0
assert json.loads(dispatched.stdout) == {
    "assets": 0,
    "command": "info",
    "decoded_image_bytes": 0,
    "executor": "cpu",
    "file_bytes": 100,
    "newer_schema": False,
    "occupied_tiles": 0,
    "opaque_sections": 0,
    "resources": 0,
    "schema": "0.1.0",
    "status": "ok",
    "tiled_images": 0,
    "unknown_parts": 0,
}
assert dispatched.stderr == ""

validated = run(
    "validate",
    "--input",
    str(document),
    "--kind",
    "document",
    "--report",
    "json",
)
assert validated.returncode == 0 and validated.stderr == ""
assert json.loads(validated.stdout)["status"] == "valid"

missing_input = run("info", "--document", "does-not-exist.ctex")
assert missing_input.returncode == 3
assert "could not read input" in missing_input.stderr

print("headless CLI skeleton passed")
