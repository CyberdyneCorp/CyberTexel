#!/usr/bin/env python3
"""Process-level regression tests for the headless CLI contract."""

from __future__ import annotations

import json
import os
from pathlib import Path
import signal
import subprocess
import sys
import tempfile
import time


BINARY = sys.argv[1]
ROOT = Path(sys.argv[2])
FIXTURE_WRITER = sys.argv[3]
NATIVE_LIBRARY = sys.argv[4]
BAKE_PROVIDER = sys.argv[5]
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


def process_environment(overrides: dict[str, str] | None = None) -> dict[str, str]:
    child_environment = os.environ.copy()
    child_environment.pop("CTEX_EXECUTOR", None)
    child_environment["CTEX_PYTHON"] = sys.executable
    child_environment["CYBERTEXEL_LIBRARY"] = NATIVE_LIBRARY
    child_environment["PYTHONPATH"] = str(ROOT / "python" / "src")
    asan_runtime = child_environment.get("CTEX_SANITIZER_ASAN_LIBRARY")
    if asan_runtime and Path(asan_runtime).is_file():
        child_environment["LD_PRELOAD"] = asan_runtime
        child_environment["ASAN_OPTIONS"] = "detect_leaks=0"
    if overrides:
        child_environment.update(overrides)
    return child_environment


def run(
    *arguments: str, environment: dict[str, str] | None = None
) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [BINARY, *arguments],
        capture_output=True,
        text=True,
        check=False,
        env=process_environment(environment),
    )


def assert_identical_trees(first: Path, second: Path) -> None:
    first_files = sorted(path.relative_to(first) for path in first.rglob("*") if path.is_file())
    second_files = sorted(path.relative_to(second) for path in second.rglob("*") if path.is_file())
    assert first_files == second_files
    for relative in first_files:
        assert (first / relative).read_bytes() == (second / relative).read_bytes(), relative


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

    missing_provider = run(
        "bake-request",
        "--document",
        str(source),
        "--provider",
        str(directory / "missing-provider-library"),
        "--output",
        str(directory / "missing-provider.ctex"),
        "--report",
        "json",
    )
    assert missing_provider.returncode == 3
    assert json.loads(missing_provider.stdout)["outputs"] == []
    assert "could not load bake provider" in missing_provider.stderr

    unsupported_provider = run(
        "bake-request",
        "--document",
        str(source),
        "--provider",
        NATIVE_LIBRARY,
        "--output",
        str(directory / "unsupported-provider.ctex"),
        "--report",
        "json",
    )
    assert unsupported_provider.returncode == 4
    assert json.loads(unsupported_provider.stdout)["outputs"] == []
    assert "does not export" in unsupported_provider.stderr

    missing_map_output = directory / "missing-map.ctex"
    missing_map = run(
        "bake-request",
        "--document",
        str(source),
        "--provider",
        BAKE_PROVIDER,
        "--output",
        str(missing_map_output),
        "--report",
        "json",
        environment={"CTEX_CLI_FIXTURE_MISSING_AO": "1"},
    )
    assert missing_map.returncode == 5
    assert missing_map.returncode != missing.returncode
    assert not missing_map_output.exists()
    assert json.loads(missing_map.stdout)["outputs"] == []
    assert "ambient-occlusion" in missing_map.stderr

    baked_output = directory / "baked.ctex"
    baked = run(
        "bake-request",
        "--document",
        str(source),
        "--provider",
        BAKE_PROVIDER,
        "--output",
        str(baked_output),
        "--report",
        "json",
    )
    baked_report = json.loads(baked.stdout)
    assert baked.returncode == 0, (baked.stdout, baked.stderr)
    assert baked.stderr == ""
    assert baked_report["provider"] == "fixture-provider"
    assert baked_report["requests"] == 2
    assert baked_report["replaced_maps"] == 1
    assert baked_report["operations"] == ["read", "open", "bake", "bind", "save"]
    assert baked_report["outputs"] == [
        {"kind": "project", "path": str(baked_output), "bytes": baked_output.stat().st_size}
    ]
    assert source.read_bytes() == source_bytes
    baked_info = run("info", "--document", str(baked_output), "--report", "json")
    assert baked_info.returncode == 0
    assert json.loads(baked_info.stdout)["bound_maps"] == 2

    repeated_baked_output = directory / "baked-repeat.ctex"
    repeated_bake = run(
        "bake-request",
        "--document",
        str(source),
        "--provider",
        BAKE_PROVIDER,
        "--output",
        str(repeated_baked_output),
        "--report",
        "json",
    )
    assert repeated_bake.returncode == 0
    assert baked_output.read_bytes() == repeated_baked_output.read_bytes()

    unsupported_mesh_output = directory / "unsupported-mesh-export"
    unsupported_mesh = run(
        "export",
        "--document",
        str(source),
        "--preset",
        "base-color",
        "--mesh",
        str(directory / "replacement.glb"),
        "--output",
        str(unsupported_mesh_output),
        "--report",
        "json",
    )
    assert unsupported_mesh.returncode == 4
    assert not unsupported_mesh_output.exists()
    assert json.loads(unsupported_mesh.stdout)["outputs"] == []

    replacement_mesh = directory / "replacement.obj"
    replacement_mesh.write_text(
        "o Replacement\n"
        "v 0 0 0\nv 1 0 0\nv 0 1 0\nv 2 0 0\nv 3 0 0\nv 2 1 0\n"
        "vn 0 0 1\nvt 0 0\nvt 0.8 0\nvt 0 0.8\n"
        "usemtl body\nf 1/1/1 2/2/1 3/3/1\n"
        "usemtl cloth\nf 4/1/1 5/2/1 6/3/1\n",
        encoding="utf-8",
    )
    replacement_exports: dict[str, Path] = {}
    for policy in ("keep", "clear", "reproject"):
        replacement_output = directory / f"replacement-{policy}"
        replacement_export = run(
            "export",
            "--document",
            str(source),
            "--preset",
            "base-color",
            "--mesh",
            str(replacement_mesh),
            "--mesh-policy",
            policy,
            "--output",
            str(replacement_output),
            "--report",
            "json",
        )
        replacement_report = json.loads(replacement_export.stdout)
        assert replacement_export.returncode == 0, (
            policy,
            replacement_export.stdout,
            replacement_export.stderr,
        )
        assert replacement_export.stderr == ""
        assert replacement_report["mesh_replacement"]["policy"] == policy
        assert replacement_report["mesh_replacement"]["changed_texture_sets"] == 2
        assert replacement_report["operations"] == [
            "read",
            "open",
            "analyze-mesh",
            "reconcile",
            "plan",
            "encode",
            "publish",
        ]
        assert len(replacement_report["outputs"]) == 2
        replacement_exports[policy] = replacement_output
    assert replacement_exports["keep"].is_dir()
    assert replacement_exports["clear"].is_dir()
    assert replacement_exports["reproject"].is_dir()

    unrepresentable_output = directory / ("x" * 300)
    internal_failure = run(
        "export",
        "--document",
        str(source),
        "--preset",
        "base-color",
        "--output",
        str(unrepresentable_output),
        "--report",
        "json",
    )
    assert internal_failure.returncode == 70
    internal_report = json.loads(internal_failure.stdout)
    assert internal_report["exit_code"] == 70
    assert internal_report["diagnostic"] in internal_failure.stderr
    assert internal_report["outputs"] == []

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

    deterministic_export_directory = directory / "textures-repeat"
    deterministic_export = run(
        "export",
        "--document",
        str(source),
        "--preset",
        "base-color",
        "--output",
        str(deterministic_export_directory),
        "--report",
        "json",
    )
    assert deterministic_export.returncode == 0 and deterministic_export.stderr == ""
    assert_identical_trees(export_directory, deterministic_export_directory)

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

    interrupt_source = directory / "interrupt-source.ctex"
    interrupt_fixture = subprocess.run(
        [FIXTURE_WRITER, "--write-cli-interrupt-fixture", str(interrupt_source)],
        capture_output=True,
        text=True,
        check=False,
    )
    assert interrupt_fixture.returncode == 0, interrupt_fixture.stderr
    interrupted_export_output = directory / "interrupted-export"
    creation_flags = subprocess.CREATE_NEW_PROCESS_GROUP if os.name == "nt" else 0
    interrupted_export = subprocess.Popen(
        [
            BINARY,
            "export",
            "--document",
            str(interrupt_source),
            "--preset",
            "base-color",
            "--output",
            str(interrupted_export_output),
            "--report",
            "json",
        ],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        env=process_environment(),
        creationflags=creation_flags,
    )
    try:
        time.sleep(0.05)
        assert interrupted_export.poll() is None, "twenty-texture export finished before interrupt"
        interrupted_export.send_signal(
            signal.CTRL_BREAK_EVENT if os.name == "nt" else signal.SIGINT
        )
        export_interrupt_stdout, export_interrupt_stderr = interrupted_export.communicate(
            timeout=60
        )
    finally:
        if interrupted_export.poll() is None:
            interrupted_export.kill()
            interrupted_export.communicate()
    assert interrupted_export.returncode == 6, (
        export_interrupt_stdout,
        export_interrupt_stderr,
    )
    assert json.loads(export_interrupt_stdout)["exit_code"] == 6
    assert "interrupted" in export_interrupt_stderr
    assert not interrupted_export_output.exists()
    assert not list(directory.glob(".interrupted-export.ctex-stage-*"))

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

    repeated_application = directory / "applied-repeat.ctex"
    applied_again = run(
        "apply",
        "--document",
        str(source),
        "--preset",
        str(preset),
        "--texture-set",
        texture_set,
        "--output",
        str(repeated_application),
        "--report",
        "json",
    )
    assert applied_again.returncode == 0 and applied_again.stderr == ""
    assert output.read_bytes() == repeated_application.read_bytes()

    applied_info = run("info", "--document", str(output), "--report", "json")
    applied_inventory = json.loads(applied_info.stdout)
    assert applied_info.returncode == 0
    assert applied_inventory["documents"] == 1
    assert applied_inventory["texture_sets"] == 2
    assert applied_inventory["layer_entries"] == 2
    assert applied_inventory["preset_applications"] == 2
    assert applied_inventory["mesh_bindings"] == 1
    assert applied_inventory["bound_maps"] == 1
    assert applied_inventory["mesh_map_bytes"] > 0

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

    script = directory / "edit.py"
    script.write_text(
        "import os\n"
        "def main(document):\n"
        "    print('script-progress')\n"
        "    os.write(1, b'raw-script-progress\\n')\n"
        "    texture_set = document.texture_set_ids()[0]\n"
        "    document.set_channel_enabled(texture_set, 'pbr.roughness')\n",
        encoding="utf-8",
    )
    scripted_output = directory / "scripted.ctex"
    scripted_output.write_bytes(b"previous-output")
    scripted = run(
        "run",
        "--document",
        str(source),
        "--script",
        str(script),
        "--output",
        str(scripted_output),
        "--report",
        "json",
    )
    scripted_report = json.loads(scripted.stdout)
    assert scripted.returncode == 0, (scripted.stdout, scripted.stderr)
    assert "script-progress" in scripted.stderr
    assert "raw-script-progress" in scripted.stderr
    assert scripted_report["status"] == "ok"
    assert scripted_report["document_asset"] == "document/main"
    assert scripted_report["operations"] == ["read", "open", "script", "save"]
    assert scripted_report["outputs"] == [
        {
            "kind": "project",
            "path": str(scripted_output),
            "bytes": scripted_output.stat().st_size,
        }
    ]
    assert source.read_bytes() == source_bytes
    scripted_validation = run(
        "validate",
        "--input",
        str(scripted_output),
        "--kind",
        "document",
        "--report",
        "json",
    )
    assert scripted_validation.returncode == 0

    repeated_scripted_output = directory / "scripted-repeat.ctex"
    scripted_again = run(
        "run",
        "--document",
        str(source),
        "--script",
        str(script),
        "--output",
        str(repeated_scripted_output),
        "--report",
        "json",
    )
    assert scripted_again.returncode == 0
    assert scripted_output.read_bytes() == repeated_scripted_output.read_bytes()

    missing_interpreter_output = directory / "missing-interpreter.ctex"
    missing_interpreter = run(
        "run",
        "--document",
        str(source),
        "--script",
        str(script),
        "--output",
        str(missing_interpreter_output),
        "--report",
        "json",
        environment={"CTEX_PYTHON": str(directory / "missing-python")},
    )
    assert missing_interpreter.returncode == 3
    assert not missing_interpreter_output.exists()
    assert json.loads(missing_interpreter.stdout)["outputs"] == []

    bounded_script_output = directory / "bounded-script.ctex"
    combined_ceiling = len(source_bytes) + len(script.read_bytes()) - 1
    bounded_script = run(
        "run",
        "--document",
        str(source),
        "--script",
        str(script),
        "--output",
        str(bounded_script_output),
        "--memory-ceiling",
        str(combined_ceiling),
        "--report",
        "json",
    )
    assert bounded_script.returncode == 7
    assert not bounded_script_output.exists()
    assert json.loads(bounded_script.stdout)["outputs"] == []

    failing_script = directory / "fail.py"
    failing_script.write_text(
        "def main(document):\n"
        "    document.set_channel_enabled(document.texture_set_ids()[0], 'pbr.roughness')\n"
        "    raise RuntimeError('intentional-script-failure')\n",
        encoding="utf-8",
    )
    protected_script_output = directory / "protected-script.ctex"
    protected_script_output.write_bytes(b"previous-good-output")
    failed_script = run(
        "run",
        "--document",
        str(source),
        "--script",
        str(failing_script),
        "--output",
        str(protected_script_output),
        "--report",
        "json",
    )
    assert failed_script.returncode == 2
    assert "intentional-script-failure" in failed_script.stderr
    assert protected_script_output.read_bytes() == b"previous-good-output"
    assert json.loads(failed_script.stdout)["outputs"] == []

    if os.name != "nt":
        interrupt_script = directory / "interrupt.py"
        interrupt_script.write_text(
            "import time\n"
            "def main(document):\n"
            "    time.sleep(30)\n",
            encoding="utf-8",
        )
        interrupted_output = directory / "interrupted.ctex"
        interrupted_output.write_bytes(b"previous-good-output")
        interrupted = subprocess.Popen(
            [
                BINARY,
                "run",
                "--document",
                str(source),
                "--script",
                str(interrupt_script),
                "--output",
                str(interrupted_output),
                "--report",
                "json",
            ],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            env=process_environment(),
        )
        try:
            for _ in range(200):
                if list(directory.glob(".interrupted.ctex.ctex-run-*")):
                    break
                if interrupted.poll() is not None:
                    raise AssertionError("interrupt fixture exited before staging")
                time.sleep(0.01)
            else:
                raise AssertionError("interrupt fixture did not reach staging")
            interrupted.send_signal(signal.SIGINT)
            interrupt_stdout, interrupt_stderr = interrupted.communicate(timeout=10)
        finally:
            if interrupted.poll() is None:
                interrupted.kill()
                interrupted.communicate()
        assert interrupted.returncode == 6, (interrupt_stdout, interrupt_stderr)
        assert json.loads(interrupt_stdout)["exit_code"] == 6
        assert "interrupted" in interrupt_stderr
        assert interrupted_output.read_bytes() == b"previous-good-output"
        assert not list(directory.glob(".interrupted.ctex.ctex-run-*"))

print("headless CLI contract passed")
