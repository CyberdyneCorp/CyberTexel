#!/usr/bin/env python3
"""Publish a smart material into a project library, then recover it after a lost session.

Capabilities: project-io, smart-materials, texture-export.
The material is packaged with its image resource and imported back packed,
referenced and missing; the library it is installed into is normalized, saved
atomically, autosaved, enumerated from the recovery directory and resumed, and
a published texture-export preset plans the delivery maps it would write.
"""

from __future__ import annotations

import argparse
import ctypes
import json
import tempfile
from pathlib import Path

import cybertexel


CAPABILITIES = ("project-io", "smart-materials", "texture-export")
MATERIAL_ID = "materials/foundry-brass"
RESOURCE_ID = "images/noise"
RESOURCE_PATH = "textures/noise.bin"
RESOURCE_BYTES = b"noise"
RECOVERY_KEY = "foundry-brass"
EXPORT_PRESETS = [
    "pbr-individual", "occlusion-roughness-metallic", "metallic-occlusion-smoothness",
    "metallic-emission-roughness", "base-color", "specular-glossiness",
]


def check(result: int, expected: int = 0) -> None:
    """Fail with the library's own diagnostic rather than a bare result code."""
    diagnostic = cybertexel.capi.ctex_get_last_diagnostic().decode("utf-8")
    assert result == expected, diagnostic


def container_info(capi: object) -> object:
    info = capi.ctex_project_container_info()
    info.size = capi.CTEX_PROJECT_CONTAINER_INFO_CURRENT_SIZE
    return info


def two_call(capi: object, function: object, head: tuple, info: object) -> tuple[bytes, str]:
    """Run the atomic two-call contract shared by every container-producing entry point."""
    check(function(*head, capi.byref(info), None, 0, None, 0))
    output = capi.create_string_buffer(info.canonical_size)
    report = capi.create_string_buffer(info.report_size)
    check(function(*head, capi.byref(info), output, info.canonical_size, report, len(report)))
    return output.raw[: info.canonical_size], report.value.decode("utf-8")


def authored_material(capi: object) -> bytes:
    """One derived entry whose image node reads the declared portable resource."""
    info = capi.ctex_material_graph_info()
    info.size = capi.CTEX_MATERIAL_GRAPH_INFO_CURRENT_SIZE
    check(capi.ctex_material_graph_create_default(capi.byref(info), None, 0, None, 0))
    default = capi.create_string_buffer(info.canonical_size)
    check(capi.ctex_material_graph_create_default(
        capi.byref(info), default, len(default), None, 0))
    graph, node = default.raw[: info.canonical_size], capi.uint64_t()
    head = (capi.create_string_buffer(graph), len(graph), capi.String(b"ctex.texture.image"),
            capi.ctex_vec2f(0.0, 0.0), capi.byref(info), capi.byref(node))
    check(capi.ctex_material_graph_add_builtin_node(*head, None, 0))
    added = capi.create_string_buffer(info.canonical_size)
    check(capi.ctex_material_graph_add_builtin_node(*head, added, len(added)))
    graph = added.raw[: info.canonical_size]

    value = capi.ctex_smart_material_value_descriptor()
    value.size = capi.CTEX_SMART_MATERIAL_VALUE_DESCRIPTOR_CURRENT_SIZE
    value.type = capi.CTEX_SMART_MATERIAL_VALUE_IMAGE
    value.text = capi.String(RESOURCE_ID.encode())
    head = (capi.create_string_buffer(graph), len(graph), node.value, capi.String(b"image"),
            capi.byref(value), capi.byref(info))
    check(capi.ctex_material_graph_set_property_value(*head, None, 0))
    output = capi.create_string_buffer(info.canonical_size)
    check(capi.ctex_material_graph_set_property_value(*head, output, len(output)))
    assert info.node_count == 2

    def hexed(text: str) -> str:
        return text.encode("utf-8").hex()

    records = [
        "CTEX_SMART_MATERIAL\t6",
        f"PRESET\t{hexed(MATERIAL_ID)}\t{hexed('Foundry brass')}",
        f"ENTRY\t0\t{hexed('surface')}\t\t{hexed('Surface')}\t1\t3ff0000000000000"
        f"\t{output.raw[: info.canonical_size].hex()}\t0",
        f"RESOURCE\t{hexed(RESOURCE_ID)}\t{hexed('image')}",
        "END",
    ]
    return ("\n".join(records) + "\n").encode("utf-8")


def package_material(capi: object, material: bytes, directory: Path | None) -> tuple[bytes, str]:
    resource = capi.ctex_project_resource_descriptor(
        capi.CTEX_PROJECT_RESOURCE_DESCRIPTOR_CURRENT_SIZE, capi.String(RESOURCE_ID.encode()),
        capi.String(b"image"), capi.String(RESOURCE_PATH.encode()), 0, None, 0)
    options = capi.ctex_project_asset_export_options_descriptor()
    options.size = capi.CTEX_PROJECT_ASSET_EXPORT_OPTIONS_DESCRIPTOR_CURRENT_SIZE
    options.self_contained = 1 if directory is not None else 0
    if directory is not None:
        options.source_directory = capi.String(str(directory).encode())
    head = (capi.create_string_buffer(material), len(material), capi.byref(resource), 1,
            capi.byref(options))
    info = container_info(capi)
    package, report = two_call(capi, capi.ctex_smart_material_package, head, info)
    assert (info.asset_count, info.resource_count) == (1, 1)
    if directory is not None:
        assert info.packed_resource_bytes == len(RESOURCE_BYTES)
    return package, report


def import_material(capi: object, package: bytes, search: Path | None) -> tuple[bytes, dict]:
    paths = None
    if search is not None:
        buffer = capi.create_string_buffer(str(search).encode())
        pointer = ctypes.cast(buffer, ctypes.POINTER(ctypes.c_char))
        descriptor = capi.ctex_project_asset_search_paths_descriptor(
            capi.CTEX_PROJECT_ASSET_SEARCH_PATHS_DESCRIPTOR_CURRENT_SIZE,
            (ctypes.POINTER(ctypes.c_char) * 1)(pointer), 1,
        )
        paths = capi.byref(descriptor)
    info = capi.ctex_smart_material_info()
    info.size = capi.CTEX_SMART_MATERIAL_INFO_CURRENT_SIZE
    head = (capi.create_string_buffer(package), len(package), None, paths)
    canonical, report = two_call(capi, capi.ctex_smart_material_import, head, info)
    assert (info.entry_count, info.resource_reference_count) == (1, 1)
    return canonical, json.loads(report)


def install_into_library(capi: object, package: bytes) -> tuple[bytes, str]:
    """Install the packaged preset into a fresh project library and refuse a second copy."""
    required = capi.c_size_t()
    check(capi.ctex_project_container_create_empty(None, 0, capi.byref(required)))
    empty = capi.create_string_buffer(required.value)
    check(capi.ctex_project_container_create_empty(empty, len(empty), capi.byref(required)))
    version = capi.ctex_project_container_version()
    version.size = capi.CTEX_PROJECT_CONTAINER_VERSION_CURRENT_SIZE
    check(capi.ctex_project_container_probe_version(empty, len(empty), capi.byref(version)))
    assert version.major == capi.ctex_get_version().major

    info = container_info(capi)
    head = (empty, len(empty), capi.create_string_buffer(package), len(package), None, None)
    library, report = two_call(capi, capi.ctex_project_asset_install, head, info)
    assert (info.asset_count, info.resource_count) == (1, 1)
    assert info.packed_resource_bytes == len(RESOURCE_BYTES)
    assert json.loads(report)["assets"][0]["kind"] == "smart-material"

    untouched = capi.create_string_buffer(b"\xa5" * len(library))
    check(capi.ctex_project_asset_install(
        capi.create_string_buffer(library), len(library), capi.create_string_buffer(package),
        len(package), None, None, capi.byref(info), untouched, len(library), None, 0),
        capi.CTEX_RESULT_INVALID_ARGUMENT)
    assert capi.ctex_get_last_diagnostic_code() == capi.CTEX_DIAGNOSTIC_INVALID_PROJECT_CONTAINER
    assert untouched.raw[: len(library)] == b"\xa5" * len(library)
    return library, report


def export_asset(capi: object, library: bytes, directory: Path) -> bytes:
    options = capi.ctex_project_asset_export_options_descriptor()
    options.size = capi.CTEX_PROJECT_ASSET_EXPORT_OPTIONS_DESCRIPTOR_CURRENT_SIZE
    options.self_contained = 1
    options.source_directory = capi.String(str(directory).encode())
    info = container_info(capi)
    head = (capi.create_string_buffer(library), len(library), None,
            capi.String(MATERIAL_ID.encode()), capi.byref(options))
    package, report = two_call(capi, capi.ctex_project_asset_export, head, info)
    assert (info.asset_count, info.resource_count) == (1, 1)
    assert f'"id":"{MATERIAL_ID}"' in report
    return package


def autosave_and_recover(capi: object, project: bytes, directory: Path) -> str:
    """Coalesce three submissions into one durable recovery write, then refuse an unsafe key."""
    config = capi.ctex_project_autosave_config_descriptor(
        capi.CTEX_PROJECT_AUTOSAVE_CONFIG_DESCRIPTOR_CURRENT_SIZE,
        capi.String(str(directory).encode()), capi.String(RECOVERY_KEY.encode()), 60_000,
    )
    session = ctypes.POINTER(capi.ctex_project_autosave_session)()
    check(capi.ctex_project_autosave_session_create(capi.byref(config), capi.byref(session)))
    status, statuses = capi.uint32_t(), []
    for revision in (1, 2, 1):
        check(capi.ctex_project_autosave_session_submit(
            session, revision, capi.create_string_buffer(project), len(project), None,
            capi.byref(status)))
        statuses.append(status.value)
    queued, stale = capi.CTEX_PROJECT_AUTOSAVE_QUEUED, capi.CTEX_PROJECT_AUTOSAVE_STALE_REVISION
    assert statuses == [queued, queued, stale]

    idle = capi.uint32_t(1)
    check(capi.ctex_project_autosave_session_wait(session, 0, capi.byref(idle)))
    assert idle.value == 0
    info = capi.ctex_project_autosave_info()
    info.size = capi.CTEX_PROJECT_AUTOSAVE_INFO_CURRENT_SIZE
    check(capi.ctex_project_autosave_session_get_info(session, capi.byref(info), None, 0, None, 0))
    assert info.has_pending_revision == 1 and info.pending_revision == 2

    check(capi.ctex_project_autosave_session_flush(session))
    info.size = capi.CTEX_PROJECT_AUTOSAVE_INFO_CURRENT_SIZE
    path = capi.create_string_buffer(info.required_recovery_path_size)
    error = capi.create_string_buffer(info.required_last_error_size)
    check(capi.ctex_project_autosave_session_get_info(
        session, capi.byref(info), path, len(path), error, len(error)))
    assert (info.has_last_saved_revision, info.last_saved_revision) == (1, 2)
    assert info.has_pending_revision == 0 and info.successful_writes == 1
    assert (info.required_recovery_path_size, info.required_last_error_size) == (len(path), 1)
    assert error.value == b"" and Path(path.value.decode()).parent == directory
    capi.ctex_project_autosave_session_destroy(session)

    refused = ctypes.POINTER(capi.ctex_project_autosave_session)()
    unsafe = capi.ctex_project_autosave_config_descriptor(
        capi.CTEX_PROJECT_AUTOSAVE_CONFIG_DESCRIPTOR_CURRENT_SIZE,
        capi.String(str(directory).encode()), capi.String(b"../escape"), 1,
    )
    check(capi.ctex_project_autosave_session_create(capi.byref(unsafe), capi.byref(refused)),
          capi.CTEX_RESULT_INVALID_ARGUMENT)
    assert not refused
    return Path(path.value.decode()).name


def enumerate_recovery(capi: object, directory: Path) -> tuple[str, list[str]]:
    (directory / "broken.ctex-recovery").write_bytes(b"bad")
    info = capi.ctex_project_recovery_enumeration_info()
    info.size = capi.CTEX_PROJECT_RECOVERY_ENUMERATION_INFO_CURRENT_SIZE
    root = capi.String(str(directory).encode())
    check(capi.ctex_project_recovery_enumerate(root, capi.byref(info), None, 0, None, 0, None, 0))
    assert (info.required_recoverable_count, info.required_rejected_count) == (1, 1)
    recoverable = (capi.ctex_project_recovery_entry * 1)()
    rejected = (capi.ctex_project_recovery_rejection * 1)()
    strings = capi.create_string_buffer(info.required_string_size)
    check(capi.ctex_project_recovery_enumerate(
        root, capi.byref(info), recoverable, 1, rejected, 1, strings, len(strings)))

    def read(offset: int, size: int) -> str:
        return strings.raw[offset : offset + size - 1].decode("utf-8")

    entry, broken = recoverable[0], rejected[0]
    assert read(entry.recovery_key_offset, entry.recovery_key_size) == RECOVERY_KEY
    assert entry.schema.major == capi.ctex_get_version().major and entry.file_bytes > 0
    rejection = read(broken.message_offset, broken.message_size)
    assert rejection == "recovery file has a truncated header"
    assert Path(read(broken.path_offset, broken.path_size)).name == "broken.ctex-recovery"
    return read(entry.path_offset, entry.path_size), [rejection]


def resume_recovery(capi: object, path: str, project: bytes) -> tuple[int, str]:
    info = container_info(capi)
    check(capi.ctex_project_recovery_read(
        capi.String(path.encode()), None, capi.byref(info), None, 0, None, 0))
    assert info.canonical_size == len(project)
    checkpoint = capi.ctex_project_recovery_checkpoint_info()
    checkpoint.size = capi.CTEX_PROJECT_RECOVERY_CHECKPOINT_INFO_CURRENT_SIZE
    opened = capi.create_string_buffer(info.canonical_size)
    report = capi.create_string_buffer(info.report_size)
    check(capi.ctex_project_recovery_resume(
        capi.String(path.encode()), None, capi.byref(checkpoint), capi.byref(info),
        opened, info.canonical_size, report, len(report)))
    assert checkpoint.has_revision == 1 and checkpoint.revision == 2
    assert opened.raw[: info.canonical_size] == project
    recovered = json.loads(report.value)
    assert info.report_size == len(report.value) + 1 and recovered["newer_schema"] is False
    assert recovered["resources"] == [
        {"id": RESOURCE_ID, "kind": "image", "path": RESOURCE_PATH, "storage": "packed",
         "packed_bytes": len(RESOURCE_BYTES)}]
    return checkpoint.revision, report.value.decode("utf-8")


def built_in_export_presets(capi: object) -> list[str]:
    required, count = capi.c_size_t(), capi.c_size_t()
    check(capi.ctex_texture_export_get_built_in_preset_ids(
        None, 0, capi.byref(required), capi.byref(count)))
    buffer = capi.create_string_buffer(required.value)
    check(capi.ctex_texture_export_get_built_in_preset_ids(
        buffer, len(buffer), capi.byref(required), capi.byref(count)))
    identifiers = buffer.raw[: required.value].decode("utf-8").split("\0")[:-1]
    assert identifiers == EXPORT_PRESETS and len(identifiers) == count.value
    return identifiers


def plan_delivery(capi: object, preset_id: str) -> None:
    """Run a published preset as a dry run: the library names every map it would write."""
    plans: list[dict[str, object]] = []

    @capi.ctex_texture_export_report_callback
    def receive(payload: object, size: int, _user_data: object) -> int:
        plans.append(json.loads(ctypes.string_at(payload.data, size)))
        return capi.CTEX_RESULT_SUCCESS

    sources = (capi.ctex_texture_export_texture_set_source_descriptor * 1)(
        capi.ctex_texture_export_texture_set_source_descriptor(
            capi.CTEX_TEXTURE_EXPORT_TEXTURE_SET_SOURCE_DESCRIPTOR_CURRENT_SIZE,
            capi.String(b"body"), capi.String(b"Body"), 32, 32, None, 0, None, 0))
    callbacks = capi.ctex_texture_export_callbacks_descriptor()
    callbacks.size = capi.CTEX_TEXTURE_EXPORT_CALLBACKS_DESCRIPTOR_CURRENT_SIZE
    callbacks.report = receive
    info = capi.ctex_texture_export_info()
    info.size = capi.CTEX_TEXTURE_EXPORT_INFO_CURRENT_SIZE
    check(capi.ctex_texture_export_run(
        capi.byref(capi.ctex_texture_export_catalogue_descriptor(
            capi.CTEX_TEXTURE_EXPORT_CATALOGUE_DESCRIPTOR_CURRENT_SIZE,
            capi.String(b"foundry"), sources, 1, None, 0)),
        capi.byref(capi.ctex_texture_export_preset_descriptor(
            capi.CTEX_TEXTURE_EXPORT_PRESET_DESCRIPTOR_CURRENT_SIZE,
            capi.String(preset_id.encode()), capi.String(b"Delivery"), None, 0)),
        capi.byref(capi.ctex_texture_export_options_descriptor(
            capi.CTEX_TEXTURE_EXPORT_OPTIONS_DESCRIPTOR_CURRENT_SIZE, None, 2, 90, 1)),
        capi.byref(callbacks),
        capi.byref(info)))
    plan = plans[0]
    assert (info.dry_run, info.encoded_output_count, info.planned_output_count) == (1, 0, 9)
    assert plan["preset"] == preset_id and len(plan["outputs"]) == 9
    assert {item["preset_entry"]: item["bit_depth"] for item in plan["outputs"]} == {
        "_BaseColor": 8, "_Opacity": 8, "_Roughness": 8, "_Metallic": 8, "_Normal": 16,
        "_Height": 16, "_Occlusion": 8, "_Emission": 8, "_Subsurface": 8}
    assert plan["outputs"][0]["path"] == "foundry_Body_BaseColor_32_8_single_visible.png"


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--executor", default="cpu")
    arguments = parser.parse_args()

    capi = cybertexel.capi
    material = authored_material(capi)
    with tempfile.TemporaryDirectory(prefix="ctex-example-22-") as workspace:
        root = Path(workspace)
        sources, recovery = root / "resources", root / "recovery"
        (sources / "textures").mkdir(parents=True)
        recovery.mkdir()
        (sources / RESOURCE_PATH).write_bytes(RESOURCE_BYTES)

        packed_package, _ = package_material(capi, material, sources)
        referenced_package, _ = package_material(capi, material, None)
        canonical, packed_report = import_material(capi, packed_package, None)
        assert canonical == material and packed_report["resources_complete"] is True
        assert packed_report["resource_resolution"] == [
            {"id": RESOURCE_ID, "kind": "image", "status": "packed", "bytes": len(RESOURCE_BYTES)}]
        assert packed_report["image_inputs"] == [
            {"entry": "surface", "resource": RESOURCE_ID, "status": "packed"}]
        _, referenced = import_material(capi, referenced_package, sources)
        assert referenced["resource_resolution"][0]["status"] == "referenced"
        assert referenced["resource_resolution"][0]["bytes"] == len(RESOURCE_BYTES)

        library, install_report = install_into_library(capi, packed_package)
        exported = export_asset(capi, library, sources)
        reexported, exported_report = import_material(capi, exported, None)
        assert reexported == material
        assert exported_report["resource_resolution"][0]["status"] == "packed"

        (sources / RESOURCE_PATH).unlink()
        _, missing = import_material(capi, referenced_package, sources)
        assert missing["resource_resolution"] == [
            {"id": RESOURCE_ID, "kind": "image", "status": "missing", "bytes": 0}]
        assert missing["image_inputs"][0]["status"] == "missing"
        assert missing["resources_complete"] is False

        info = container_info(capi)
        head = (capi.create_string_buffer(library), len(library), None)
        normalized, normal = two_call(capi, capi.ctex_project_container_normalize, head, info)
        assert normalized == library and json.loads(normal)["newer_schema"] is False
        assert info.asset_count == 1 and info.resource_count == 1

        saved = root / "foundry.ctex"
        check(capi.ctex_project_container_save_atomic(
            capi.create_string_buffer(library), len(library), None,
            capi.String(str(saved).encode()), capi.byref(info)))
        assert saved.read_bytes() == library

        recovery_file = autosave_and_recover(capi, library, recovery)
        path, rejections = enumerate_recovery(capi, recovery)
        revision, recovery_report = resume_recovery(capi, path, library)
        assert json.loads(recovery_report)["assets"] == json.loads(install_report)["assets"]
        presets = built_in_export_presets(capi)
        plan_delivery(capi, presets[0])

    arguments.output.mkdir(parents=True, exist_ok=True)
    (arguments.output / "foundry_brass.ctex-asset").write_bytes(packed_package)
    (arguments.output / "foundry_library.ctex").write_bytes(library)
    (arguments.output / "recovered_project.json").write_text(
        recovery_report + "\n", encoding="utf-8"
    )
    summary = {
        "capabilities": list(CAPABILITIES), "export_preset_ids": presets,
        "exported_package_bytes": len(exported), "library_bytes": len(library),
        "install_report_assets": json.loads(install_report)["assets"],
        "packed_package_bytes": len(packed_package), "recovered_revision": revision,
        "recovery_file": recovery_file, "recovery_rejections": len(rejections),
        "referenced_package_bytes": len(referenced_package),
    }
    (arguments.output / "summary.json").write_text(
        json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )


if __name__ == "__main__":
    main()
