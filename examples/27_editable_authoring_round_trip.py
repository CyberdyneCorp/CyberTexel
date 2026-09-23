#!/usr/bin/env python3
"""Keep authored text and a surface path editable across edits, saves and a resize.

Capabilities: editable-authoring.
Text and surface paths stay first-class source in the document: they carry their
own revision, refuse an edit whose report cannot be delivered, undo and redo as
a unit, and resolve through the same stroke model the paint engine uses. The
project container round-trips them without flattening, a resolution change
replays the operation records that can be replayed and resamples the rest, and
the replay assessment plus the resource ledger decide whether the recovery bytes
that make all of that possible actually fit in the host's budget.
"""

from __future__ import annotations

import argparse
import ctypes
import json
from pathlib import Path

import cybertexel


CAPABILITIES = ("editable-authoring",)

capi = cybertexel.capi
byref, pointer, text, buffer = capi.byref, capi.pointer, capi.String, capi.create_string_buffer
SEMANTIC = b"pbr.base_color"
EXTENT = 128
REPORT_BYTES = 8192


def ok(result: int, expected: int = 0) -> None:
    assert result == expected, (result, capi.ctex_get_last_diagnostic())


def handle(kind: object) -> object:
    return capi.POINTER(kind)()


def sized(kind: object, size: int) -> object:
    value = kind()
    value.size = size
    return value


def entry_info() -> object:
    return sized(capi.ctex_editable_entry_info, capi.CTEX_EDITABLE_ENTRY_INFO_CURRENT_SIZE)


def tile(x: int, y: int) -> object:
    return capi.ctex_editable_tile_dependency_descriptor(
        capi.CTEX_EDITABLE_TILE_DEPENDENCY_DESCRIPTOR_CURRENT_SIZE, text(b"base-color"), x, y)


def parameter(roughness: float) -> object:
    return capi.ctex_editable_material_parameter_descriptor(
        capi.CTEX_EDITABLE_MATERIAL_PARAMETER_DESCRIPTOR_CURRENT_SIZE, text(b"roughness"), 1,
        (capi.c_double * 4)(roughness, 0.0, 0.0, 0.0))


def placement() -> object:
    return capi.ctex_editable_placement_frame(
        capi.ctex_vec3d(1.0, 2.0, 3.0), capi.ctex_vec3d(0.0, 0.0, 1.0), 0.25, 2.0,
        capi.ctex_vec2d(1.0, 0.5))


def text_entry(body: bytes, revision: int, tiles: object, material: object) -> object:
    return capi.ctex_editable_entry_descriptor(
        capi.CTEX_EDITABLE_ENTRY_DESCRIPTOR_CURRENT_SIZE, text(b"title"),
        capi.CTEX_EDITABLE_ENTRY_TEXT, revision, placement(), text(b"paint/gold"),
        pointer(material), 1, text(body), text(b"fonts/inter-bold-v4"), 0, None, 0,
        tiles, len(tiles))


def author_text(document: object, name: object) -> dict[str, object]:
    material = parameter(0.35)
    one, two = (capi.ctex_editable_tile_dependency_descriptor * 1)(tile(2, 1)), (
        capi.ctex_editable_tile_dependency_descriptor * 2)(tile(2, 1), tile(3, 1))
    report = buffer(REPORT_BYTES)
    info = entry_info()
    added = text_entry(b"CyberTexel", 0, one, material)
    ok(capi.ctex_texture_set_editable_entry_add(
        document, name, byref(added), byref(info), report, len(report)))
    assert info.entry_revision == 1 and info.undo_step_count == 1
    assert b"CyberTexel" in report.value and info.material_parameter_count == 1

    edited = text_entry(b"CyberTexel 2", 1, two, material)
    info = entry_info()
    ok(capi.ctex_texture_set_editable_entry_edit(
        document, name, byref(edited), byref(info), report, len(report)))
    assert info.entry_revision == 2 and info.invalidated_tile_count == 2

    # A report that cannot be delivered whole refuses the edit rather than half-applying it.
    doomed = text_entry(b"must-not-commit", 2, two, material)
    info = entry_info()
    ok(capi.ctex_texture_set_editable_entry_edit(
        document, name, byref(doomed), byref(info), report, 1), capi.CTEX_RESULT_BUFFER_TOO_SMALL)
    info = entry_info()
    ok(capi.ctex_texture_set_editable_entry_inspect(
        document, name, text(b"title"), byref(info), report, len(report)))
    assert info.entry_revision == 2 and info.entry_present == 1
    assert b"CyberTexel 2" in report.value and b"must-not-commit" not in report.value

    info = entry_info()
    ok(capi.ctex_texture_set_editable_entry_undo(
        document, name, byref(info), report, len(report)))
    assert info.entry_revision == 1 and b"CyberTexel 2" not in report.value
    assert info.redo_step_count == 1
    info = entry_info()
    ok(capi.ctex_texture_set_editable_entry_redo(
        document, name, byref(info), report, len(report)))
    assert info.entry_revision == 2 and b"CyberTexel 2" in report.value
    assert info.redo_step_count == 0
    info = entry_info()
    ok(capi.ctex_texture_set_editable_entry_undo(
        document, name, byref(info), report, len(report)))
    assert info.entry_revision == 1
    return {"entry_count": info.entry_count, "revision_after_undo": info.entry_revision,
            "undo_steps": info.undo_step_count}


def surface_point(x: float, width: float, triangle: int, along: float) -> object:
    return capi.ctex_editable_surface_point_descriptor(
        capi.CTEX_EDITABLE_SURFACE_POINT_DESCRIPTOR_CURRENT_SIZE,
        capi.ctex_vec3d(x, 0.0, 0.0), capi.ctex_vec3d(0.0, 0.0, 1.0), triangle,
        (capi.c_double * 3)(1.0 - along, along, 0.0), width)


def path_entry(points: object, revision: int, material: object, tiles: object) -> object:
    frame = capi.ctex_editable_placement_frame(
        capi.ctex_vec3d(0.0, 0.0, 0.0), capi.ctex_vec3d(0.0, 0.0, 1.0), 0.0, 1.0,
        capi.ctex_vec2d(1.0, 1.0))
    entry = capi.ctex_editable_entry_descriptor(
        capi.CTEX_EDITABLE_ENTRY_DESCRIPTOR_CURRENT_SIZE, text(b"seam-line"),
        capi.CTEX_EDITABLE_ENTRY_SURFACE_PATH, revision, frame, text(b"paint/thread"),
        pointer(material), 1)
    entry.mesh_revision = 44
    entry.surface_points, entry.surface_point_count = points, len(points)
    entry.dependent_tiles, entry.dependent_tile_count = tiles, len(tiles)
    return entry


def resolve_path(document: object, name: object) -> tuple[object, int, int]:
    settings = capi.ctex_stroke_settings_descriptor()
    ok(capi.ctex_stroke_settings_init(byref(settings)))
    settings.spacing_fraction = 0.5
    info = sized(capi.ctex_resolved_stroke_info, capi.CTEX_RESOLVED_STROKE_INFO_CURRENT_SIZE)
    stamps, segments = capi.c_size_t(), capi.c_size_t()
    resolve = capi.ctex_texture_set_editable_surface_path_resolve
    ok(resolve(document, name, text(b"seam-line"), byref(settings), byref(info),
               None, 0, byref(stamps), None, 0, byref(segments)))
    assert stamps.value >= 2 and segments.value >= 1
    resolved = (capi.ctex_resolved_stamp * stamps.value)()
    swept = (capi.ctex_swept_segment * segments.value)()
    info.size = capi.CTEX_RESOLVED_STROKE_INFO_CURRENT_SIZE
    ok(resolve(document, name, text(b"seam-line"), byref(settings), byref(info),
               resolved, len(resolved), byref(stamps), swept, len(swept), byref(segments)))
    assert segments.value == stamps.value - 1
    return resolved[0], stamps.value, segments.value


def author_path(document: object, name: object) -> dict[str, object]:
    material = parameter(0.35)
    points = (capi.ctex_editable_surface_point_descriptor * 2)(
        surface_point(0.0, 0.5, 7, 0.0), surface_point(2.0, 1.0, 8, 1.0))
    tiles = (capi.ctex_editable_tile_dependency_descriptor * 2)(tile(0, 0), tile(1, 0))
    report = buffer(REPORT_BYTES)
    info = entry_info()
    ok(capi.ctex_texture_set_editable_entry_add(
        document, name, byref(path_entry(points, 0, material, tiles)), byref(info),
        report, len(report)))
    assert info.surface_point_count == 2 and info.entry_count == 2
    first, stamps, segments = resolve_path(document, name)
    assert first.position.y == 0.0 and first.radius == 0.5

    # Editing a control point reaches stroke reconstruction, and undo puts it back.
    points[0] = surface_point(0.0, 0.75, 7, 0.0)
    points[0].position.y = 1.0
    wider = parameter(0.6)
    info = entry_info()
    ok(capi.ctex_texture_set_editable_entry_edit(
        document, name, byref(path_entry(points, 1, wider, tiles)), byref(info),
        report, len(report)))
    assert info.entry_revision == 2 and info.invalidated_tile_count == 2
    assert b'"values":[0.600000]' in report.value
    edited, _, _ = resolve_path(document, name)
    assert edited.position.y == 1.0 and edited.radius == 0.75

    info = entry_info()
    ok(capi.ctex_texture_set_editable_entry_undo(
        document, name, byref(info), report, len(report)))
    assert b'"values":[0.350000]' in report.value
    restored, _, _ = resolve_path(document, name)
    assert restored.position.y == 0.0 and restored.radius == 0.5

    info = entry_info()
    ok(capi.ctex_texture_set_editable_entry_plan_rasterization(
        document, name, text(b"seam-line"), byref(info), report, len(report)))
    assert info.invalidated_tile_count == 2 and b"rasterize" in report.value
    return {"stamp_count": stamps, "swept_segment_count": segments,
            "planned_tiles": info.invalidated_tile_count}


def empty_project() -> tuple[object, int]:
    size = capi.c_size_t()
    ok(capi.ctex_project_container_create_empty(None, 0, byref(size)))
    project = buffer(size.value)
    ok(capi.ctex_project_container_create_empty(project, size.value, byref(size)))
    return project, size.value


def round_trip(document: object, name: object) -> dict[str, object]:
    empty, empty_size = empty_project()
    asset = text(b"set/body/editable")
    info = sized(capi.ctex_project_container_info, capi.CTEX_PROJECT_CONTAINER_INFO_CURRENT_SIZE)
    save = capi.ctex_project_container_upsert_editable_authoring
    ok(save(empty, empty_size, None, document, name, asset, byref(info), None, 0, None, 0))
    saved, report = buffer(info.canonical_size), buffer(info.report_size)
    ok(save(empty, empty_size, None, document, name, asset, byref(info), saved,
            info.canonical_size, report, info.report_size))
    assert info.asset_count == 1 and b"editable-authoring" in report.value

    with cybertexel.Document() as reopened:
        texture_set = reopened.create_texture_set(
            "Body", partition_key="body", width=EXTENT, height=EXTENT)
        target = ctypes.cast(reopened._require_open(), capi.POINTER(capi.ctex_document))
        reopened_name = text(texture_set.identifier.encode())
        entry = entry_info()
        restored = buffer(REPORT_BYTES)
        ok(capi.ctex_project_container_restore_editable_authoring(
            saved, info.canonical_size, None, asset, target, reopened_name, byref(entry),
            restored, len(restored)))
        assert entry.entry_count == 2
        entry = entry_info()
        ok(capi.ctex_texture_set_editable_entry_inspect(
            target, reopened_name, text(b"title"), byref(entry), restored, len(restored)))
        assert b"CyberTexel" in restored.value and b"inter-bold-v4" in restored.value
    return {"canonical_bytes": info.canonical_size, "asset_count": info.asset_count,
            "restored_entry_count": entry.entry_count}


def operation_record(replay_class: int) -> tuple[object, int]:
    default_value = (capi.c_double * 3)(0.5, 0.5, 0.5)
    channel = capi.ctex_operation_channel_descriptor(
        capi.CTEX_OPERATION_CHANNEL_DESCRIPTOR_CURRENT_SIZE, text(SEMANTIC), 3,
        capi.CTEX_SCALAR_REPRESENTATION_UNSIGNED_NORMALIZED, 8,
        capi.CTEX_COLOR_SPACE_SRGB_REC709, default_value, 3)
    frame = (capi.c_double * 16)(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1)
    checkpoint = buffer(b"checkpoints/base-color")
    checkpoints = (capi.POINTER(capi.c_char) * 1)(
        capi.cast(checkpoint, capi.POINTER(capi.c_char)))
    payload = buffer(b"stamps")
    descriptor = capi.ctex_operation_record_descriptor(
        capi.CTEX_OPERATION_RECORD_DESCRIPTOR_CURRENT_SIZE, text(b"operations/stroke"),
        text(b"cybertexel.paint.brush"), 3, text(b"brushes/basic"), 1, replay_class, 1, 7,
        text(b"sha256:mesh"), frame, capi.CTEX_OPERATION_PAYLOAD_RESOLVED_STAMPS, 1,
        pointer(channel), 1, None, 0, checkpoints, 1, capi.cast(payload, capi.c_void_p), 6)
    info = sized(capi.ctex_operation_record_info, capi.CTEX_OPERATION_RECORD_INFO_CURRENT_SIZE)
    create = capi.ctex_operation_record_create
    ok(create(byref(descriptor), byref(info), None, 0, None, 0))
    canonical = buffer(info.canonical_size)
    ok(create(byref(descriptor), byref(info), canonical, info.canonical_size, None, 0))
    return canonical, info.canonical_size


def change_resolution(document: object, name: object, record: object, size: int) -> dict:
    records = (capi.ctex_resolution_operation_record_descriptor * 1)(
        capi.ctex_resolution_operation_record_descriptor(
            capi.CTEX_RESOLUTION_OPERATION_RECORD_DESCRIPTOR_CURRENT_SIZE,
            capi.cast(record, capi.c_void_p), size))
    support = capi.ctex_operation_algorithm_support_descriptor(
        capi.CTEX_OPERATION_ALGORITHM_SUPPORT_DESCRIPTOR_CURRENT_SIZE,
        text(b"cybertexel.paint.brush"), 3, 3)
    target = EXTENT * 2
    pixels = (capi.c_ubyte * (target * target * 3))()
    pixels[-3] = 255
    raster = (capi.ctex_resolution_replay_raster_descriptor * 1)(
        capi.ctex_resolution_replay_raster_descriptor(
            capi.CTEX_RESOLUTION_REPLAY_RASTER_DESCRIPTOR_CURRENT_SIZE, text(SEMANTIC), 0,
            capi.cast(pixels, capi.c_void_p), len(pixels)))
    resize = capi.ctex_texture_set_resolution_change_descriptor(
        capi.CTEX_TEXTURE_SET_RESOLUTION_CHANGE_DESCRIPTOR_CURRENT_SIZE, target, target,
        capi.CTEX_RESOLUTION_REPLAY_ELIGIBLE, capi.CTEX_CHECKPOINT_RESAMPLE_REFUSE,
        records, 1, pointer(support), 1, raster, 1, 0, 0)
    info_size = capi.CTEX_TEXTURE_SET_RESOLUTION_CHANGE_INFO_CURRENT_SIZE
    info = sized(capi.ctex_texture_set_resolution_change_info, info_size)
    change = capi.ctex_texture_set_change_resolution
    # An eligible record still needs a declared filter for anything it cannot replay.
    ok(change(document, name, byref(resize), byref(info)), capi.CTEX_RESULT_UNSUPPORTED_OPERATION)
    resize.checkpoint_policy = capi.CTEX_CHECKPOINT_RESAMPLE_BILINEAR
    info.size = info_size
    ok(change(document, name, byref(resize), byref(info)))
    assert info.committed == 1 and info.resampled_source_count == 1
    assert info.replayed_source_count == 0
    assert (info.source_width, info.target_width) == (EXTENT, target)

    restore_size = capi.CTEX_TEXTURE_SET_RESOLUTION_RESTORE_INFO_CURRENT_SIZE
    restored = sized(capi.ctex_texture_set_resolution_restore_info, restore_size)
    ok(capi.ctex_texture_set_undo_resolution_change(document, name, byref(restored)))
    assert (restored.width, restored.height) == (EXTENT, EXTENT)
    restored.size = restore_size
    ok(capi.ctex_texture_set_redo_resolution_change(document, name, byref(restored)))
    assert (restored.width, restored.height) == (target, target)
    return {"target_width": info.target_width, "replayed": info.replayed_source_count,
            "resampled": info.resampled_source_count}


def assess_recovery(record: object, size: int) -> dict[str, object]:
    info = sized(capi.ctex_operation_record_info, capi.CTEX_OPERATION_RECORD_INFO_CURRENT_SIZE)
    inspect, serialized = capi.ctex_operation_record_inspect, capi.cast(record, capi.c_void_p)
    # The record sizes its own report, so nothing here guesses a ceiling for it.
    ok(inspect(serialized, size, byref(info), None, 0, None, 0))
    report = buffer(info.report_size)
    ok(inspect(serialized, size, byref(info), None, 0, report, info.report_size))
    assert len(report.value) + 1 == info.report_size and info.canonical_size == size
    assert info.input_document_revision == 1 and info.seed == 7
    assert info.checkpoint_image_count == 1 and info.channel_count == 1
    assert b"checkpoints/base-color" in report.value

    support = capi.ctex_operation_algorithm_support_descriptor(
        capi.CTEX_OPERATION_ALGORITHM_SUPPORT_DESCRIPTOR_CURRENT_SIZE,
        text(b"cybertexel.paint.brush"), 3, 3)
    assessment = capi.ctex_operation_replay_assessment_descriptor(
        capi.CTEX_OPERATION_REPLAY_ASSESSMENT_DESCRIPTOR_CURRENT_SIZE, pointer(support), 1, 1)
    replay_size = capi.CTEX_OPERATION_REPLAY_INFO_CURRENT_SIZE
    replay = sized(capi.ctex_operation_replay_info, replay_size)
    assess = capi.ctex_operation_record_assess_replay
    ok(assess(capi.cast(record, capi.c_void_p), size, byref(assessment), byref(replay), None, 0))
    assert replay.replay_available == 1 and replay.checkpoint_available == 1
    assert replay.disposition == capi.CTEX_OPERATION_REPLAY_RESOLUTION_INDEPENDENT_AVAILABLE

    # An algorithm the host no longer implements falls back to the retained checkpoint.
    support.minimum_version, support.maximum_version = 1, 1
    replay.size = replay_size
    ok(assess(capi.cast(record, capi.c_void_p), size, byref(assessment), byref(replay), None, 0))
    fallback = buffer(replay.required_report_size)
    replay.size = replay_size
    ok(assess(capi.cast(record, capi.c_void_p), size, byref(assessment), byref(replay),
              fallback, len(fallback)))
    assert replay.replay_available == 0 and replay.checkpoint_available == 1
    assert replay.disposition == capi.CTEX_OPERATION_REPLAY_UNSUPPORTED_ALGORITHM
    assert b"raster checkpoint retained" in fallback.value

    ledger = handle(capi.ctex_resource_ledger)
    ok(capi.ctex_resource_ledger_create(byref(ledger)))
    limits = capi.ctex_resource_budget_limits(
        capi.CTEX_RESOURCE_BUDGET_LIMITS_CURRENT_SIZE, size, 0, 64, 0)
    admit = capi.ctex_resource_ledger_admit_operation_recovery
    report_size = capi.CTEX_RESOURCE_ADMISSION_REPORT_CURRENT_SIZE
    admission = sized(capi.ctex_resource_admission_report, report_size)
    reservation = handle(capi.ctex_resource_reservation)
    ok(admit(ledger, byref(limits), capi.cast(record, capi.c_void_p), size, 64,
             byref(reservation), byref(admission)))
    assert admission.status == capi.CTEX_RESOURCE_ADMITTED_WHOLE and reservation
    capi.ctex_resource_reservation_destroy(reservation)

    limits.backing_store_bytes = 63
    admission.size = report_size
    refused = handle(capi.ctex_resource_reservation)
    ok(admit(ledger, byref(limits), capi.cast(record, capi.c_void_p), size, 64,
             byref(refused), byref(admission)))
    assert admission.status == capi.CTEX_RESOURCE_OVER_BUDGET and not refused
    capi.ctex_resource_ledger_destroy(ledger)
    return {"checkpoint_images": info.checkpoint_image_count, "canonical_bytes": size,
            "fallback_disposition": replay.disposition}


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--executor", default="cpu")
    arguments = parser.parse_args()

    with cybertexel.Document() as document:
        texture_set = document.create_texture_set(
            "Body", partition_key="body", width=EXTENT, height=EXTENT)
        document.set_channel_enabled(texture_set, SEMANTIC.decode(), bit_depth=8)
        raw = ctypes.cast(document._require_open(), capi.POINTER(capi.ctex_document))
        name = text(texture_set.identifier.encode())
        record, record_size = operation_record(capi.CTEX_OPERATION_REPLAY_RESOLUTION_INDEPENDENT)
        staged, staged_size = operation_record(capi.CTEX_OPERATION_REPLAY_CHECKPOINT_ONLY)
        summary = {
            "capabilities": list(CAPABILITIES),
            "text_entry": author_text(raw, name),
            "surface_path": author_path(raw, name),
            "project_round_trip": round_trip(raw, name),
            "resolution_change": change_resolution(raw, name, staged, staged_size),
            "replay_assessment": assess_recovery(record, record_size),
        }

    arguments.output.mkdir(parents=True, exist_ok=True)
    (arguments.output / "editable_authoring.json").write_text(
        json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
