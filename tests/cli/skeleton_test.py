#!/usr/bin/env python3
"""Process-level regression tests for the headless CLI contract."""

from __future__ import annotations

import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile


BINARY = sys.argv[1]
ROOT = Path(sys.argv[2])
FIXTURE_WRITER = sys.argv[3]
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
    "documents": 0,
    "texture_sets": 0,
    "layer_entries": 0,
    "atlases": 0,
    "editable_entries": 0,
    "preset_applications": 0,
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
    "bake-request",
    "--document",
    str(document),
    "--provider",
    "fixture-provider",
    "--output",
    "baked.ctex",
    "--report",
    "json",
)
unsupported_report = json.loads(unsupported.stdout)
assert unsupported.returncode == 4
assert unsupported_report["status"] == "unsupported"
assert unsupported_report["outputs"] == []
assert "not implemented" in unsupported.stderr

with tempfile.TemporaryDirectory(prefix="ctex-cli-apply-") as temporary:
    directory = Path(temporary)
    source = directory / "source.ctex"
    preset = directory / "material.ctex"
    missing_preset = directory / "missing-resource.ctex"
    generated = subprocess.run(
        [
            FIXTURE_WRITER,
            "--write-cli-fixture",
            str(source),
            str(preset),
            str(missing_preset),
        ],
        capture_output=True,
        text=True,
        check=False,
    )
    assert generated.returncode == 0 and generated.stderr == ""
    texture_set = generated.stdout.strip()
    source_bytes = source.read_bytes()

    export_directory = directory / "textures"
    exported = run(
        "export",
        "--document",
        str(source),
        "--preset",
        "base-color",
        "--output",
        str(export_directory),
        "--report",
        "json",
    )
    export_report = json.loads(exported.stdout)
    assert exported.returncode == 0 and exported.stderr == ""
    assert export_report["status"] == "ok" and export_report["preset"] == "base-color"
    assert export_report["operations"] == ["read", "open", "plan", "encode", "publish"]
    assert len(export_report["outputs"]) == 2
    for exported_texture in export_report["outputs"]:
        path = Path(exported_texture["path"])
        assert path.is_file() and path.is_relative_to(export_directory)
        assert exported_texture | {
            "kind": "texture",
            "bytes": path.stat().st_size,
            "width": 8,
            "height": 8,
            "format": "PNG",
            "bit_depth": 8,
        } == exported_texture
    assert source.read_bytes() == source_bytes

    repeated_export = run(
        "export",
        "--document",
        str(source),
        "--preset",
        "base-color",
        "--output",
        str(export_directory),
        "--report",
        "json",
    )
    assert repeated_export.returncode == 2
    assert json.loads(repeated_export.stdout)["outputs"] == []
    assert "already exists" in repeated_export.stderr

    unknown_export = directory / "unknown-export"
    unknown_preset = run(
        "export",
        "--document",
        str(source),
        "--preset",
        "missing-preset",
        "--output",
        str(unknown_export),
        "--report",
        "json",
    )
    assert unknown_preset.returncode == 2 and not unknown_export.exists()
    assert "available:" in unknown_preset.stderr and "base-color" in unknown_preset.stderr

    bounded_export = directory / "bounded-export"
    over_texel_budget = run(
        "export",
        "--document",
        str(source),
        "--preset",
        "base-color",
        "--output",
        str(bounded_export),
        "--texel-ceiling",
        "1",
        "--report",
        "json",
    )
    assert over_texel_budget.returncode == 7 and not bounded_export.exists()
    assert json.loads(over_texel_budget.stdout)["outputs"] == []

    output = directory / "applied.ctex"
    applied = run(
        "apply",
        "--document",
        str(source),
        "--preset",
        str(preset),
        "--texture-set",
        texture_set,
        "--output",
        str(output),
        "--report",
        "json",
    )
    applied_report = json.loads(applied.stdout)
    assert applied.returncode == 0 and applied.stderr == ""
    assert applied_report["status"] == "ok"
    assert applied_report["texture_set"] == texture_set
    assert applied_report["entries_added"] == 1
    assert applied_report["outputs"] == [
        {"kind": "project", "path": str(output), "bytes": output.stat().st_size}
    ]
    assert output.exists() and source.read_bytes() == source_bytes

    applied_info = run("info", "--document", str(output), "--report", "json")
    applied_inventory = json.loads(applied_info.stdout)
    assert applied_info.returncode == 0
    assert applied_inventory["documents"] == 1
    assert applied_inventory["texture_sets"] == 2
    assert applied_inventory["layer_entries"] == 2
    assert applied_inventory["preset_applications"] == 2

    protected = directory / "protected.ctex"
    protected.write_bytes(b"previous-good-output")
    bad_target = run(
        "apply",
        "--document",
        str(source),
        "--preset",
        str(preset),
        "--texture-set",
        "missing-set",
        "--output",
        str(protected),
        "--report",
        "json",
    )
    assert bad_target.returncode == 2
    assert protected.read_bytes() == b"previous-good-output"
    assert json.loads(bad_target.stdout)["outputs"] == []

    missing_output = directory / "missing-output.ctex"
    missing_resource = run(
        "apply",
        "--document",
        str(source),
        "--preset",
        str(missing_preset),
        "--texture-set",
        texture_set,
        "--output",
        str(missing_output),
        "--report",
        "json",
    )
    assert missing_resource.returncode == 5
    assert not missing_output.exists()
    assert "missing resource" in missing_resource.stderr

print("headless CLI contract passed")
