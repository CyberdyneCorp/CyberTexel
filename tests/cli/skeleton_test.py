#!/usr/bin/env python3
"""Process-level regression tests for the headless CLI contract."""

from __future__ import annotations

import json
import os
from pathlib import Path
import subprocess
import sys


BINARY = sys.argv[1]
ROOT = Path(sys.argv[2])
COMMANDS = ("export", "bake-request", "apply", "run", "info", "validate")
GLOBAL_OPTIONS = (
    "--report",
    "--quiet",
    "--executor",
    "--memory-ceiling",
    "--texel-ceiling",
    "--workers",
    "--help",
)
COMMAND_OPTIONS = {
    "export": ("--document", "--output", "--preset", "--mesh", "--mesh-policy"),
    "bake-request": ("--document", "--provider", "--output"),
    "apply": ("--document", "--preset", "--texture-set", "--output"),
    "run": ("--document", "--script", "--output"),
    "info": ("--document",),
    "validate": ("--input", "--kind"),
}


def run(
    *arguments: str, environment: dict[str, str] | None = None
) -> subprocess.CompletedProcess[str]:
    child_environment = os.environ.copy()
    child_environment.pop("CTEX_EXECUTOR", None)
    if environment:
        child_environment.update(environment)
    return subprocess.run(
        [BINARY, *arguments],
        capture_output=True,
        text=True,
        check=False,
        env=child_environment,
    )


help_result = run("--help")
assert help_result.returncode == 0 and help_result.stderr == ""
for command in COMMANDS:
    assert command in help_result.stdout
for option in GLOBAL_OPTIONS:
    assert option in help_result.stdout
assert "default: CTEX_EXECUTOR or auto" in help_result.stdout

for command in COMMANDS:
    command_help = run(command, "--help")
    assert command_help.returncode == 0 and command_help.stderr == ""
    for option in (*COMMAND_OPTIONS[command], *GLOBAL_OPTIONS):
        assert option in command_help.stdout
    assert "default:" in command_help.stdout

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

machine_invalid = run("info", "--report", "json")
assert machine_invalid.returncode == 2
assert "--document" in machine_invalid.stderr
assert json.loads(machine_invalid.stdout)["status"] == "invalid_arguments"

document = ROOT / "examples" / "outputs" / "01_version_and_project_container" / "empty.ctex"
dispatched = run(
    "info", "--document", str(document), "--report", "json", "--quiet"
)
assert dispatched.returncode == 0
info_report = json.loads(dispatched.stdout)
assert info_report | {
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
    "texels": 0,
    "tiled_images": 0,
    "unknown_parts": 0,
} == info_report
assert info_report["exit_code"] == 0
assert info_report["executor_requested"] == "auto"
assert info_report["fallback"] is False
assert info_report["inputs"] == [{"kind": "document", "path": str(document)}]
assert info_report["operations"] == ["read", "inspect"]
assert info_report["outputs"] == [] and info_report["clamped_parameters"] == []
assert info_report["timings_ms"]["total"] >= 0
assert info_report["limits"]["workers"] >= 1
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

fallback = run(
    "validate",
    "--input",
    str(document),
    "--kind",
    "document",
    "--report",
    "json",
    environment={"CTEX_EXECUTOR": "host"},
)
fallback_report = json.loads(fallback.stdout)
assert fallback.returncode == 0 and fallback.stderr == ""
assert fallback_report["executor"] == "cpu"
assert fallback_report["executor_requested"] == "host"
assert fallback_report["executor_source"] == "environment"
assert fallback_report["fallback"] is True

flag_override = run(
    "validate",
    "--input",
    str(document),
    "--kind",
    "document",
    "--report",
    "json",
    "--executor",
    "cpu",
    environment={"CTEX_EXECUTOR": "host"},
)
flag_report = json.loads(flag_override.stdout)
assert flag_override.returncode == 0
assert flag_report["executor_requested"] == "cpu"
assert flag_report["executor_source"] == "flag"
assert flag_report["fallback"] is False

missing_input = run("info", "--document", "does-not-exist.ctex")
assert missing_input.returncode == 3
assert "could not read input" in missing_input.stderr

budget = run(
    "info",
    "--document",
    str(document),
    "--memory-ceiling",
    "99",
    "--report",
    "json",
)
budget_report = json.loads(budget.stdout)
assert budget.returncode == 7
assert budget_report["exit_code"] == 7 and budget_report["status"] == "error"
assert "memory ceiling is 99" in budget_report["diagnostic"]
assert budget_report["outputs"] == []
assert budget_report["limits"]["memory_bytes"] == 99

bounded_workers = run(
    "info",
    "--document",
    str(document),
    "--workers",
    "2",
    "--texel-ceiling",
    "1",
    "--report",
    "json",
)
bounded_report = json.loads(bounded_workers.stdout)
assert bounded_workers.returncode == 0
assert bounded_report["limits"]["workers"] == 2
assert bounded_report["limits"]["texels"] == 1

quiet = run("info", "--document", str(document), "--quiet")
assert quiet.returncode == 0 and quiet.stdout == "" and quiet.stderr == ""

unsupported = run(
    "apply",
    "--document",
    str(document),
    "--preset",
    "material.ctex",
    "--texture-set",
    "body",
    "--output",
    "result.ctex",
    "--report",
    "json",
)
unsupported_report = json.loads(unsupported.stdout)
assert unsupported.returncode == 4
assert unsupported_report["status"] == "unsupported"
assert unsupported_report["outputs"] == []
assert "not implemented" in unsupported.stderr

print("headless CLI contract passed")
