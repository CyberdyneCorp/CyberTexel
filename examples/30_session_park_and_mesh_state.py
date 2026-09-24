#!/usr/bin/env python3
"""Park a painting session: persist its mesh bindings, then quiesce for shutdown.

Capabilities: project-io, smart-materials, resource-residency.
An artist stops for the day. The script accounts for what the live document is
holding, writes the document's mesh resource and baked occlusion binding into
the project container as a versioned companion asset, proves the binding comes
back byte-for-byte into a fresh map set, and finally stops the resource ledger,
flushes autosave and reports exactly which revisions never reached the disk.
"""

from __future__ import annotations

import argparse
import ctypes
import json
import tempfile
from pathlib import Path

import cybertexel
import numpy as np


CAPABILITIES = ("project-io", "smart-materials", "resource-residency")
CAPI = cybertexel.capi
OK = CAPI.CTEX_RESULT_SUCCESS
DOCUMENT_ID, MESH_ID, MESH_BYTES = b"document/main", b"mesh/hero", b"o hero-shell\n"
MATERIAL_ID, RECOVERY_KEY = "materials/hero-shell", "hero-shell"
SIZE, SET_POINTER = 4, ctypes.POINTER(CAPI.ctex_mesh_map_set)
OCCLUSION = np.arange(16, dtype=np.uint8).reshape(4, 4) * 17
CENTRES = [((x + 0.5) / SIZE, (y + 0.5) / SIZE) for y in range(SIZE) for x in range(SIZE)]
CORNERS = [(0.0, 1.0), (0.5, 0.5), (1.0, 0.0)]
SUMMARY: dict[str, object] = {"capabilities": list(CAPABILITIES)}


def call(name: str, *arguments: object, expect: int = OK) -> None:
    """Fail with the library's own diagnostic rather than a bare result code."""
    result = getattr(CAPI, name)(*arguments)
    assert result == expect, f"{name} -> {result}: {CAPI.ctex_get_last_diagnostic().decode()}"


def sized(name: str, **fields: object) -> object:
    """Allocate a versioned ABI struct with its CURRENT_SIZE and named fields."""
    value = getattr(CAPI, name)()
    value.size = getattr(CAPI, f"{name.upper()}_CURRENT_SIZE")
    for field, item in fields.items():
        setattr(value, field, item)
    return value


def two_call(name: str, head: tuple, info: object) -> tuple[bytes, str]:
    """The atomic two-call contract every container-producing entry point shares."""
    call(name, *head, CAPI.byref(info), None, 0, None, 0)
    output = CAPI.create_string_buffer(info.canonical_size)
    report = CAPI.create_string_buffer(info.report_size)
    call(name, *head, CAPI.byref(info), output, info.canonical_size, report, len(report))
    return output.raw[: info.canonical_size], report.value.decode("utf-8")


def handle_array(*maps: object) -> object:
    borrowed = (SET_POINTER * len(maps))(*maps)
    return ctypes.cast(borrowed, ctypes.POINTER(SET_POINTER))


def admit(ledger: object, admission: object) -> tuple[object, int]:
    reservation, report = ctypes.POINTER(CAPI.ctex_resource_reservation)(), sized(
        "ctex_resource_admission_report")
    call("ctex_resource_ledger_admit", ledger, CAPI.byref(admission), CAPI.byref(reservation),
         CAPI.byref(report))
    return reservation, report.status


def library_declaring_the_mesh() -> bytes:
    """A shipped material declares the mesh file, so the project can carry it."""
    graph, text = sized("ctex_material_graph_info"), lambda v: v.encode("utf-8").hex()
    call("ctex_material_graph_create_default", CAPI.byref(graph), None, 0, None, 0)
    default = CAPI.create_string_buffer(graph.canonical_size)
    call("ctex_material_graph_create_default", CAPI.byref(graph), default, len(default), None, 0)
    hexed = default.raw[: graph.canonical_size].hex()
    material = ("\n".join([
        "CTEX_SMART_MATERIAL\t6",
        f"PRESET\t{text(MATERIAL_ID)}\t{text('Hero shell')}",
        f"ENTRY\t0\t{text('surface')}\t\t{text('Surface')}\t1\t3ff0000000000000\t{hexed}\t0",
        f"RESOURCE\t{MESH_ID.hex()}\t{text('mesh')}", "END"]) + "\n").encode("utf-8")
    payload = ctypes.cast(CAPI.create_string_buffer(MESH_BYTES), ctypes.c_void_p)
    resource = CAPI.ctex_project_resource_descriptor(
        CAPI.CTEX_PROJECT_RESOURCE_DESCRIPTOR_CURRENT_SIZE, CAPI.String(MESH_ID),
        CAPI.String(b"mesh"), CAPI.String(b"meshes/hero.obj"), 1, payload, len(MESH_BYTES))
    options = sized("ctex_project_asset_export_options_descriptor", self_contained=0)
    info = sized("ctex_project_container_info")
    package, _ = two_call("ctex_smart_material_package", (
        CAPI.create_string_buffer(material), len(material), CAPI.byref(resource), 1,
        CAPI.byref(options)), info)
    assert (info.asset_count, info.resource_count, info.packed_resource_bytes) == (1, 1, 13)
    required = CAPI.c_size_t()
    call("ctex_project_container_create_empty", None, 0, CAPI.byref(required))
    empty = CAPI.create_string_buffer(required.value)
    call("ctex_project_container_create_empty", empty, len(empty), CAPI.byref(required))
    library, report = two_call("ctex_project_asset_install", (
        empty, len(empty), CAPI.create_string_buffer(package), len(package), None, None), info)
    installed = json.loads(report)
    assert installed["resources"] == [{"id": MESH_ID.decode(), "kind": "mesh",
                                       "path": "meshes/hero.obj", "storage": "packed",
                                       "packed_bytes": len(MESH_BYTES)}]
    assert installed["assets"][0]["kind"] == "smart-material"
    assert (info.asset_count, info.tiled_image_count) == (1, 0)
    return library


def account_for_the_open_document(document: object, resident: int) -> str:
    """The aggregate report and its per-texture-set detail must agree exactly."""
    memory = sized("ctex_document_memory_info")
    call("ctex_document_get_memory_report", document, CAPI.byref(memory), None, 0, None, 0)
    # Nothing is painted yet, so every resident byte is the delivered mesh map.
    assert memory.texture_set_count == 1 and memory.mesh_map_pixel_bytes == resident > 0
    assert memory.channel_pixel_bytes == memory.history_retained_bytes == 0
    assert memory.total_resident_bytes == memory.mesh_map_pixel_bytes
    details = (CAPI.ctex_document_texture_set_memory_info * memory.texture_set_count)()
    identifiers = CAPI.create_string_buffer(memory.required_texture_set_id_size)  # sized by it
    call("ctex_document_get_memory_report", document, CAPI.byref(memory), details, len(details),
         identifiers, len(identifiers))
    detail, packed = details[0], identifiers.raw[: memory.required_texture_set_id_size]
    identifier = packed[detail.texture_set_id_offset:][: detail.texture_set_id_size - 1]
    assert detail.texture_set_id_offset == 0
    assert memory.required_texture_set_id_size == detail.texture_set_id_size == len(identifier) + 1
    assert detail.mesh_map_pixel_bytes == memory.mesh_map_pixel_bytes
    assert detail.total_resident_bytes == memory.total_resident_bytes
    # The document adds its own envelope on top of every texture set it saves.
    assert detail.total_resident_bytes < detail.estimated_save_bytes < memory.estimated_save_bytes
    sentinel = (CAPI.ctex_document_texture_set_memory_info * 1)()
    sentinel[0].texture_set_id_offset, memory.texture_set_count = 77, 99
    call("ctex_document_get_memory_report", document, CAPI.byref(memory), sentinel, 1,
         identifiers, 1, expect=CAPI.CTEX_RESULT_BUFFER_TOO_SMALL)
    assert sentinel[0].texture_set_id_offset == 77 and memory.texture_set_count == 99
    assert CAPI.ctex_get_last_diagnostic_code() == CAPI.CTEX_DIAGNOSTIC_BUFFER_TOO_SMALL
    SUMMARY.update(document_resident_bytes=detail.total_resident_bytes,
                   document_estimated_save_bytes=detail.estimated_save_bytes)
    return identifier.decode("utf-8")


def bind_occlusion(maps: object) -> int:
    """Deliver the baked occlusion map into the live map set."""
    buffer = sized("ctex_mesh_map_pixel_buffer_descriptor", width=SIZE, height=SIZE,
                   component_type=CAPI.CTEX_TRANSPORT_COMPONENT_UINT8_UNORM, component_count=1,
                   row_stride_bytes=SIZE, pixels=OCCLUSION.ctypes.data_as(ctypes.c_void_p),
                   pixel_bytes=OCCLUSION.nbytes)
    descriptor = sized("ctex_mesh_map_import_descriptor", buffer=buffer,
                       kind=CAPI.CTEX_MESH_MAP_AMBIENT_OCCLUSION,
                       channel_meaning=CAPI.CTEX_MESH_MAP_SCALAR_DATA,
                       color_space=CAPI.CTEX_COLOR_SPACE_LINEAR_REC709)
    report = sized("ctex_mesh_map_import_info")
    call("ctex_mesh_map_set_import_external", maps, CAPI.byref(descriptor), CAPI.byref(report))
    assert (report.map_width, report.map_height, report.replaced_existing) == (SIZE, SIZE, 0)
    info = sized("ctex_mesh_map_set_info")
    call("ctex_mesh_map_set_get_info", maps, CAPI.byref(info), None, 0, None, 0)
    assert info.bound_map_count == 1 and info.mesh_revision != 0
    SUMMARY.update(mesh_revision=info.mesh_revision, map_resident_bytes=info.resident_pixel_bytes)
    return info.mesh_revision


def sample_map(maps: object, coordinates: object) -> np.ndarray:
    """Read the bound occlusion back through the public sampler."""
    sample, values = sized("ctex_mesh_map_sample_info"), []
    for u, v in coordinates:
        call("ctex_mesh_map_set_sample", maps, CAPI.CTEX_MESH_MAP_AMBIENT_OCCLUSION, u, v,
             CAPI.byref(sample))
        assert sample.component_count == 1 and sample.stale == 0
        values.append(float(sample.values[0]))
    return np.asarray(values, dtype=np.float64)


def texture_document_project(library: bytes, document: object) -> bytes:
    info = sized("ctex_project_container_info")
    project, _ = two_call("ctex_project_container_upsert_texture_document", (
        CAPI.create_string_buffer(library), len(library), None, document,
        CAPI.String(DOCUMENT_ID)), info)
    assert info.asset_count == 2
    return project


def persist_mesh_state(project: bytes, maps: object, revision: int) -> bytes:
    """One companion asset carries the mesh reference and every bound map."""
    state = CAPI.ctex_document_mesh_state_descriptor(
        CAPI.CTEX_DOCUMENT_MESH_STATE_DESCRIPTOR_CURRENT_SIZE, CAPI.String(DOCUMENT_ID),
        CAPI.String(MESH_ID), revision, handle_array(maps), 1)
    info = sized("ctex_project_container_info")
    saved, report = two_call("ctex_project_container_upsert_document_mesh_state", (
        CAPI.create_string_buffer(project), len(project), None, CAPI.byref(state)), info)
    assert sorted(asset["kind"] for asset in json.loads(report)["assets"]) == [
        "document-mesh-state", "smart-material", "texture-document"]
    assert (info.asset_count, info.tiled_image_count) == (3, 2)
    state.current_mesh_revision = revision + 1
    call("ctex_project_container_upsert_document_mesh_state",
         CAPI.create_string_buffer(project), len(project), None, CAPI.byref(state),
         CAPI.byref(info), None, 0, None, 0, expect=CAPI.CTEX_RESULT_STALE_STATE)
    assert b"revision does not match" in CAPI.ctex_get_last_diagnostic()
    SUMMARY.update(saved_asset_count=3, saved_image_count=2)
    return saved


def inventory_mesh_state(saved: bytes, project: bytes, texture_set_id: str) -> None:
    """Null buffers publish the exact sizes; a short buffer publishes nothing."""
    info = sized("ctex_document_mesh_state_info", map_count=99)
    head = (CAPI.create_string_buffer(saved), len(saved), None, CAPI.String(DOCUMENT_ID),
            CAPI.byref(info))
    call("ctex_project_container_get_document_mesh_state_info", *head, None, 0, None, 0)
    assert info.map_count == 1 and info.texture_set_count == 1
    assert info.required_mesh_resource_id_size == len(MESH_ID) + 1
    assert info.required_texture_set_ids_size == len(texture_set_id) + 1
    mesh_id = CAPI.create_string_buffer(info.required_mesh_resource_id_size)
    texture_sets = CAPI.create_string_buffer(info.required_texture_set_ids_size)
    call("ctex_project_container_get_document_mesh_state_info", *head, mesh_id, len(mesh_id),
         texture_sets, len(texture_sets))
    assert mesh_id.value == MESH_ID and info.current_mesh_revision == SUMMARY["mesh_revision"]
    assert texture_sets.value.decode("utf-8") == texture_set_id
    # A short buffer still publishes the sizes to retry with, but writes no bytes.
    cramped, info.map_count = CAPI.create_string_buffer(b"\xa5"), 99
    call("ctex_project_container_get_document_mesh_state_info", *head, cramped, 1, None, 0,
         expect=CAPI.CTEX_RESULT_BUFFER_TOO_SMALL)
    assert info.map_count == 1 and cramped.raw[0] == 0xA5
    assert CAPI.ctex_get_last_diagnostic().endswith(
        f"buffer_size=1 required_size={len(MESH_ID) + 1}".encode())
    call("ctex_project_container_get_document_mesh_state_info",
         CAPI.create_string_buffer(project), len(project), None, CAPI.String(DOCUMENT_ID),
         CAPI.byref(info), None, 0, None, 0, expect=CAPI.CTEX_RESULT_MISSING_RESOURCE)


def park_the_session(project: bytes, directory: Path) -> None:
    """Stop admissions, flush autosave and name the revisions that never landed."""
    recovery = directory / "recovery"
    config = CAPI.ctex_project_autosave_config_descriptor(
        CAPI.CTEX_PROJECT_AUTOSAVE_CONFIG_DESCRIPTOR_CURRENT_SIZE,
        CAPI.String(str(recovery).encode()), CAPI.String(RECOVERY_KEY.encode()), 60_000)
    session, status = ctypes.POINTER(CAPI.ctex_project_autosave_session)(), CAPI.uint32_t()
    call("ctex_project_autosave_session_create", CAPI.byref(config), CAPI.byref(session))
    for revision in (1, 2):
        call("ctex_project_autosave_session_submit", session, revision,
             CAPI.create_string_buffer(project), len(project), None, CAPI.byref(status))
        assert status.value == CAPI.CTEX_PROJECT_AUTOSAVE_QUEUED
    call("ctex_project_autosave_session_flush", session)
    autosave = sized("ctex_project_autosave_info")
    expected = recovery / f"{RECOVERY_KEY}.ctex-recovery"
    call("ctex_project_autosave_session_get_info", session, CAPI.byref(autosave), None, 0, None, 0)
    assert autosave.last_saved_revision == 2 and autosave.successful_writes == 1
    assert autosave.required_recovery_path_size == len(str(expected).encode()) + 1
    path = CAPI.create_string_buffer(autosave.required_recovery_path_size)
    error = CAPI.create_string_buffer(autosave.required_last_error_size)
    call("ctex_project_autosave_session_get_info", session, CAPI.byref(autosave), path,
         len(path), error, len(error))
    assert path.value.decode("utf-8") == str(expected) and error.value == b""
    ledger, requirement = ctypes.POINTER(CAPI.ctex_resource_ledger)(), CAPI.ctex_resource_requirement(
        CAPI.CTEX_RESOURCE_REQUIREMENT_CURRENT_SIZE, CAPI.CTEX_RESOURCE_TEMPORARY, 64,
        CAPI.CTEX_RESOURCE_CPU_RESIDENT)
    call("ctex_resource_ledger_create", CAPI.byref(ledger))
    admission = CAPI.ctex_resource_admission_descriptor(
        CAPI.CTEX_RESOURCE_ADMISSION_DESCRIPTOR_CURRENT_SIZE, CAPI.String(b"in-flight edit"),
        CAPI.ctex_resource_budget_limits(
            CAPI.CTEX_RESOURCE_BUDGET_LIMITS_CURRENT_SIZE, 1024, 1024, 1024, 1024),
        ctypes.pointer(requirement), 1)
    reservation, admitted = admit(ledger, admission)
    assert bool(reservation) and admitted == CAPI.CTEX_RESOURCE_ADMITTED_WHOLE
    cancelled: list[int] = []
    def request_cancel(user_data: object) -> None:
        # Never assert inside a ctypes callback: record, then check after the call.
        cancelled.append(1)
        CAPI.ctex_resource_reservation_destroy(reservation)
    callback = CAPI.ctex_project_quiesce_cancel_callback(request_cancel)
    descriptor = CAPI.ctex_project_quiesce_descriptor(
        CAPI.CTEX_PROJECT_QUIESCE_DESCRIPTOR_CURRENT_SIZE, 3, 0, callback, None)
    quiesce = sized("ctex_project_quiesce_report")
    call("ctex_project_lifecycle_quiesce", ledger, session, CAPI.byref(descriptor),
         CAPI.byref(quiesce))
    assert cancelled == [1] and quiesce.status == CAPI.CTEX_PROJECT_QUIESCE_DEADLINE_EXCEEDED
    assert (quiesce.admissions_stopped, quiesce.cancellation_requested) == (1, 1)
    assert quiesce.work_drained == 1 and quiesce.active_operation_count == 0
    assert quiesce.has_durable_revision == 1 and quiesce.durable_revision == 2
    assert quiesce.has_uncheckpointed_range == 1
    assert (quiesce.uncheckpointed_first_revision, quiesce.uncheckpointed_last_revision) == (3, 3)
    refused, blocked = admit(ledger, admission)
    assert not refused and blocked == CAPI.CTEX_RESOURCE_QUIESCING
    call("ctex_project_lifecycle_resume", ledger)
    resumed, readmitted = admit(ledger, admission)
    assert bool(resumed) and readmitted == CAPI.CTEX_RESOURCE_ADMITTED_WHOLE
    CAPI.ctex_resource_reservation_destroy(resumed)
    descriptor.current_revision = 2
    descriptor.request_cancel = CAPI.ctex_project_quiesce_cancel_callback()
    quiesce = sized("ctex_project_quiesce_report")
    call("ctex_project_lifecycle_quiesce", ledger, session, CAPI.byref(descriptor),
         CAPI.byref(quiesce))
    assert quiesce.status == CAPI.CTEX_PROJECT_QUIESCE_DURABLE
    assert quiesce.cancellation_requested == 0 and quiesce.has_uncheckpointed_range == 0
    assert quiesce.durable_revision == 2 and cancelled == [1]
    call("ctex_project_lifecycle_resume", ledger)
    CAPI.ctex_resource_ledger_destroy(ledger)
    CAPI.ctex_project_autosave_session_destroy(session)
    SUMMARY.update(durable_revision=2, uncheckpointed_revisions=[3, 3],
                   recovery_bytes=expected.stat().st_size)


def authored_and_restored(library: bytes) -> tuple[np.ndarray, np.ndarray, bytes]:
    """Bind, save, inventory and restore the mesh state around one live document."""
    uv = np.asarray([(0.0, 0.0), (1.0, 0.0), (1.0, 1.0), (0.0, 1.0)], dtype=np.float32)
    normals, triangles = np.full((4, 3), (0, 0, 1), np.float32), np.asarray(
        [[0, 1, 2], [0, 2, 3]], dtype=np.uint32)
    with cybertexel.Document() as document, cybertexel.Mesh(
        np.pad(uv, ((0, 0), (0, 1))), triangles, normals=normals, uv=uv
    ) as mesh:
        texture_set = document.create_texture_set("Shell", partition_key="mesh", width=SIZE,
                                                  height=SIZE)
        document.set_channel_enabled(texture_set, "pbr.base_color")
        owner = ctypes.cast(document._require_open(), ctypes.POINTER(CAPI.ctex_document))
        mesh_pointer = ctypes.cast(mesh._require_open(), ctypes.POINTER(CAPI.ctex_mesh))
        project, sets = texture_document_project(library, owner), []
        for _ in range(2):
            maps = SET_POINTER()
            call("ctex_mesh_map_set_create", owner, CAPI.String(texture_set.identifier.encode()),
                 mesh_pointer, CAPI.byref(maps))
            sets.append(maps)
        try:
            revision = bind_occlusion(sets[0])
            identifier = account_for_the_open_document(owner, SUMMARY["map_resident_bytes"])
            assert identifier == texture_set.identifier
            authored = sample_map(sets[0], CENTRES)
            saved = persist_mesh_state(project, sets[0], revision)
            inventory_mesh_state(saved, project, identifier)
            call("ctex_project_container_restore_document_mesh_state",
                 CAPI.create_string_buffer(saved), len(saved), None, CAPI.String(DOCUMENT_ID),
                 handle_array(sets[1]), 1)
            return authored, sample_map(sets[1], CENTRES + CORNERS), saved
        finally:
            for maps in sets:
                CAPI.ctex_mesh_map_set_destroy(maps)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--executor", default="cpu")
    arguments = parser.parse_args()
    authored, restored, saved = authored_and_restored(library_declaring_the_mesh())
    grid = restored[: len(CENTRES)].reshape(SIZE, SIZE)
    np.testing.assert_array_equal(grid, authored.reshape(SIZE, SIZE))
    # The delivered ramp survives as a ramp: brighter with u, darker with v.
    assert np.all(np.diff(grid, axis=1) > 0) and np.all(np.diff(grid, axis=0) < 0)
    np.testing.assert_allclose(restored[len(CENTRES):], [0.0, 0.5, 1.0], atol=1e-12)
    SUMMARY["restored_corner_samples"] = [0.0, 0.5, 1.0]
    with tempfile.TemporaryDirectory(prefix="ctex-session-") as root:
        park_the_session(saved, Path(root))
    arguments.output.mkdir(parents=True, exist_ok=True)
    scaled = np.repeat(np.repeat(np.rint(grid * 255.0).astype(np.uint8), 32, 0), 32, 1)
    (arguments.output / "restored_occlusion.png").write_bytes(
        cybertexel.encode_image(np.repeat(scaled[:, :, None], 3, axis=2)))
    (arguments.output / "summary.json").write_text(
        json.dumps(SUMMARY, indent=2, sort_keys=True) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
