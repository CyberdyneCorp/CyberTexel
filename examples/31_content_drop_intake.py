#!/usr/bin/env python3
"""Vet a vendor content drop before any of it reaches the preset shelf.

Capabilities: image-io, color-management, smart-materials, project-io.
A vendor ships a thumbnail, shelf metadata and the operation records behind a
material. The script decodes the thumbnail under an explicit working-memory
ceiling with progress and cancellation, names the colour spaces the decoder
resolved, makes the library refuse every malformed shelf by name before it is
published, and asks whether this build could still replay the recorded strokes.
"""

from __future__ import annotations

import argparse
import ctypes
import json
from pathlib import Path

import cybertexel
import numpy as np


CAPABILITIES = ("image-io", "color-management", "smart-materials", "project-io")
CAPI = cybertexel.capi
OK, ROOT = CAPI.CTEX_RESULT_SUCCESS, Path(__file__).resolve().parent
THUMBNAIL = ROOT / "fixtures" / "images" / "checker.png"
PRESET_ID, THUMBNAIL_ID = b"presets/brass", b"thumbnails/brass"
ALGORITHM, RECORD_ID = b"cybertexel.paint.brush", b"operations/brass-pass"
PROGRESS_ROWS = 4
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


def decode_thumbnail() -> tuple[np.ndarray, int]:
    """Decode under a working-memory ceiling, then prove the budget and the abort."""
    encoded = THUMBNAIL.read_bytes()
    source = (CAPI.c_ubyte * len(encoded)).from_buffer_copy(encoded)
    observed: list[tuple[int, int, int, int]] = []
    cancel_calls: list[int] = []

    def report_progress(user_data: object, progress: object) -> None:
        # Never assert inside a ctypes callback: record, then check after the call.
        step = progress[0]
        observed.append((step.phase, step.completed_rows, step.total_rows,
                         step.estimated_peak_working_bytes))

    def is_cancelled(user_data: object) -> int:
        cancel_calls.append(1)
        return 0

    progress_callback = CAPI.ctex_image_decode_progress_callback(report_progress)
    cancel_callback = CAPI.ctex_image_decode_cancel_callback(is_cancelled)
    control = sized("ctex_image_decode_control_descriptor", maximum_working_bytes=8 << 20,
                    progress_interval_rows=PROGRESS_ROWS, is_cancelled=cancel_callback,
                    report_progress=progress_callback)
    execution = sized("ctex_image_decode_execution_info")
    info = sized("ctex_decoded_image_info")
    required = CAPI.c_size_t()

    def decode(pixels: object, capacity: int, expect: int = OK) -> None:
        call("ctex_image_decode_memory_bounded", source, len(source), CAPI.String(b"thumb.png"),
             CAPI.CTEX_CHANNEL_SEMANTIC_BASE_COLOR, CAPI.CTEX_INPUT_COLOR_SPACE_AUTOMATIC, None,
             CAPI.byref(control), CAPI.byref(execution), CAPI.byref(info), pixels, capacity,
             CAPI.byref(required), expect=expect)

    decode(None, 0)
    assert (info.width, info.height, info.channel_count) == (8, 8, 3)
    assert required.value == info.width * info.height * info.channel_count
    assert info.detected_format == CAPI.CTEX_IMAGE_FILE_FORMAT_PNG and info.bit_depth == 8
    assert info.color_space_source == CAPI.CTEX_COLOR_SPACE_SOURCE_AUTOMATIC_RULE
    assert info.extension_mismatch == 0 and info.uninterpretable_profile == 0
    # The decoder reported every checkpoint it claims, and each one saw the same ceiling.
    peak = execution.estimated_peak_working_bytes
    assert execution.progress_event_count == len(observed) > 0
    assert execution.cancelled == 0 and peak > 0 and cancel_calls
    assert [step[0] for step in observed][-1] == CAPI.CTEX_IMAGE_DECODE_PHASE_COMPLETE
    assert {step[3] for step in observed} == {0, peak}
    assert max(step[1] for step in observed) == info.height

    checkpoints = len(observed)
    pixels = (CAPI.c_ubyte * required.value)()
    execution.size = CAPI.CTEX_IMAGE_DECODE_EXECUTION_INFO_CURRENT_SIZE
    decode(pixels, len(pixels))
    decoded = np.ctypeslib.as_array(pixels).reshape(info.height, info.width, info.channel_count)
    assert execution.estimated_peak_working_bytes == peak
    assert execution.progress_event_count == len(observed) - checkpoints == checkpoints
    assert int(decoded.max()) > int(decoded.min())

    control.maximum_working_bytes = peak - 1
    execution.size = CAPI.CTEX_IMAGE_DECODE_EXECUTION_INFO_CURRENT_SIZE
    decode(None, 0, CAPI.CTEX_RESULT_OVER_BUDGET)
    assert execution.estimated_peak_working_bytes > control.maximum_working_bytes
    assert CAPI.ctex_get_last_diagnostic_code() == CAPI.CTEX_DIAGNOSTIC_IMAGE_LIMIT_EXCEEDED
    SUMMARY.update(thumbnail_peak_working_bytes=peak, thumbnail_decoded_bytes=required.value,
                   thumbnail_progress_events=checkpoints)
    return decoded.copy(), info.color_space


def refuse_a_cancelled_decode() -> None:
    """A cancelled decode publishes no metadata, no size and no pixel bytes."""
    encoded = THUMBNAIL.read_bytes()
    source = (CAPI.c_ubyte * len(encoded)).from_buffer_copy(encoded)
    calls: list[int] = []

    def is_cancelled(user_data: object) -> int:
        calls.append(1)
        return 1 if len(calls) > 1 else 0

    callback = CAPI.ctex_image_decode_cancel_callback(is_cancelled)
    control = sized("ctex_image_decode_control_descriptor", maximum_working_bytes=8 << 20,
                    progress_interval_rows=PROGRESS_ROWS, is_cancelled=callback)
    execution = sized("ctex_image_decode_execution_info")
    info = sized("ctex_decoded_image_info", width=99)
    required, guard = CAPI.c_size_t(77), (CAPI.c_ubyte * 1)(0xA5)
    call("ctex_image_decode_memory_bounded", source, len(source), CAPI.String(b"thumb.png"),
         CAPI.CTEX_CHANNEL_SEMANTIC_BASE_COLOR, CAPI.CTEX_INPUT_COLOR_SPACE_AUTOMATIC, None,
         CAPI.byref(control), CAPI.byref(execution), CAPI.byref(info), guard, 1,
         CAPI.byref(required), expect=CAPI.CTEX_RESULT_CANCELLED)
    assert len(calls) == 2 and execution.cancelled == 1
    assert (info.width, required.value, guard[0]) == (99, 77, 0xA5)
    assert CAPI.ctex_get_last_diagnostic_code() == CAPI.CTEX_DIAGNOSTIC_IMAGE_DECODE_CANCELLED
    SUMMARY["cancel_checks_before_abort"] = len(calls)


def colour_space_name(color_space: int) -> str:
    """Two calls: a null buffer publishes the size, the second copies the name."""
    required = CAPI.c_size_t()
    call("ctex_color_space_get_name", color_space, None, 0, CAPI.byref(required))
    buffer = CAPI.create_string_buffer(required.value)
    call("ctex_color_space_get_name", color_space, buffer, len(buffer), CAPI.byref(required))
    assert len(buffer.value) + 1 == required.value
    return buffer.value.decode("utf-8")


def name_the_colour_spaces(detected: int) -> dict[str, str]:
    working = CAPI.ctex_get_working_color_space()
    assert working == CAPI.CTEX_COLOR_SPACE_LINEAR_REC709
    names = {"detected": colour_space_name(detected), "working": colour_space_name(working)}
    assert names == {"detected": "sRGB (Rec. 709 primaries)", "working": "Linear Rec. 709"}
    assert detected == CAPI.CTEX_COLOR_SPACE_SRGB_REC709 != working

    cramped, required = CAPI.create_string_buffer(b"\xa5"), CAPI.c_size_t()
    call("ctex_color_space_get_name", detected, cramped, 1, CAPI.byref(required),
         expect=CAPI.CTEX_RESULT_BUFFER_TOO_SMALL)
    assert cramped.raw[0] == 0xA5 and required.value == len(names["detected"]) + 1
    call("ctex_color_space_get_name", 99, None, 0, CAPI.byref(required),
         expect=CAPI.CTEX_RESULT_UNSUPPORTED_OPERATION)
    assert CAPI.ctex_get_last_diagnostic().endswith(b"color_space=99")
    assert CAPI.ctex_get_last_diagnostic_code() == CAPI.CTEX_DIAGNOSTIC_UNSUPPORTED_COLOR_SPACE
    SUMMARY["colour_space_names"] = names
    return names


def empty_container() -> bytes:
    required = CAPI.c_size_t()
    call("ctex_project_container_create_empty", None, 0, CAPI.byref(required))
    buffer = CAPI.create_string_buffer(required.value)
    call("ctex_project_container_create_empty", buffer, len(buffer), CAPI.byref(required))
    return buffer.raw[: required.value]


def two_call(name: str, head: tuple, info: object) -> tuple[bytes, str]:
    """The atomic two-call contract every container-producing entry point shares."""
    call(name, *head, CAPI.byref(info), None, 0, None, 0)
    output = CAPI.create_string_buffer(info.canonical_size)
    report = CAPI.create_string_buffer(info.report_size)
    call(name, *head, CAPI.byref(info), output, info.canonical_size, report, len(report))
    return output.raw[: info.canonical_size], report.value.decode("utf-8")


def delivered_material() -> bytes:
    """One canonical smart material, packaged exactly as a vendor would ship it."""
    graph, text = sized("ctex_material_graph_info"), lambda v: v.encode("utf-8").hex()
    call("ctex_material_graph_create_default", CAPI.byref(graph), None, 0, None, 0)
    default = CAPI.create_string_buffer(graph.canonical_size)
    call("ctex_material_graph_create_default", CAPI.byref(graph), default, len(default), None, 0)
    material = ("\n".join([
        "CTEX_SMART_MATERIAL\t6",
        f"PRESET\t{PRESET_ID.hex()}\t{text('Foundry brass')}",
        f"ENTRY\t0\t{text('surface')}\t\t{text('Surface')}\t1\t3ff0000000000000"
        f"\t{default.raw[: graph.canonical_size].hex()}\t0", "END"]) + "\n").encode("utf-8")
    options = sized("ctex_project_asset_export_options_descriptor", self_contained=0)
    info = sized("ctex_project_container_info")
    package, report = two_call("ctex_smart_material_package", (
        CAPI.create_string_buffer(material), len(material), None, 0, CAPI.byref(options)), info)
    assert (info.asset_count, info.tiled_image_count, info.resource_count) == (1, 0, 0)
    assert json.loads(report)["assets"][0] == {
        "id": PRESET_ID.decode(), "kind": "smart-material", "version": 6,
        "resource_dependencies": 0, "image_dependencies": 0, "payload_bytes": len(material)}
    return package


def shelf(identifier: bytes, contents: bytes, entries: list) -> tuple[object, tuple]:
    array = (CAPI.ctex_preset_shelf_entry_descriptor * len(entries))(*entries)
    descriptor = CAPI.ctex_preset_shelf_descriptor(
        CAPI.CTEX_PRESET_SHELF_DESCRIPTOR_CURRENT_SIZE, CAPI.String(identifier),
        CAPI.String(b"Vendor drop"),
        ctypes.cast(CAPI.create_string_buffer(contents), ctypes.c_void_p), len(contents),
        array, len(entries))
    return descriptor, (array, entries)


def shelf_entry(asset: bytes, thumbnail: bytes) -> tuple[object, object]:
    tags = (ctypes.c_char_p * 2)(b"featured", b"metal")
    return CAPI.ctex_preset_shelf_entry_descriptor(
        CAPI.CTEX_PRESET_SHELF_ENTRY_DESCRIPTOR_CURRENT_SIZE, CAPI.String(asset),
        CAPI.String(b"Foundry brass"),
        ctypes.cast(tags, ctypes.POINTER(ctypes.POINTER(ctypes.c_char))), 2,
        CAPI.String(thumbnail)), tags


def enumerate_shelves(shelves: list, expect: int = OK) -> tuple[object, str]:
    array = (CAPI.ctex_preset_shelf_descriptor * len(shelves))(*shelves)
    library = CAPI.ctex_preset_library_descriptor(
        CAPI.CTEX_PRESET_LIBRARY_DESCRIPTOR_CURRENT_SIZE, array, len(shelves), None)
    info = sized("ctex_preset_library_info")
    call("ctex_preset_library_enumerate", CAPI.byref(library), CAPI.byref(info), None, 0,
         expect=expect)
    if expect != OK:
        assert CAPI.ctex_get_last_diagnostic_code() == CAPI.CTEX_DIAGNOSTIC_INVALID_PRESET_LIBRARY
        return library, CAPI.ctex_get_last_diagnostic().decode("utf-8")
    report = CAPI.create_string_buffer(info.report_size)
    call("ctex_preset_library_enumerate", CAPI.byref(library), CAPI.byref(info), report,
         len(report))
    assert len(report.value) + 1 == info.report_size
    assert info.shelf_count == len(shelves)
    return library, report.value.decode("utf-8")


def vet_the_shelf(package: bytes) -> list[str]:
    """Every malformed shelf is refused by name before anything is published."""
    keep, empty = [], empty_container()
    published, borrowed = shelf(b"studio", empty, [])
    keep.append(borrowed)
    library, report = enumerate_shelves([published])
    listing = json.loads(report)
    assert listing == {"shelves": [{"id": "studio", "display_name": "Vendor drop",
                                    "preset_count": 0}], "presets": []}

    container = sized("ctex_project_container_info")
    call("ctex_preset_library_resolve", CAPI.byref(library), CAPI.String(PRESET_ID),
         CAPI.byref(container), None, 0, None, 0, expect=CAPI.CTEX_RESULT_INVALID_ARGUMENT)
    assert CAPI.ctex_get_last_diagnostic().endswith(
        b"preset does not exist: " + PRESET_ID)
    assert CAPI.ctex_get_last_diagnostic_code() == CAPI.CTEX_DIAGNOSTIC_INVALID_PRESET_LIBRARY

    unlisted, borrowed = shelf(b"vendor", package, [])
    keep.append(borrowed)
    named, tags = shelf_entry(b"presets/absent", THUMBNAIL_ID)
    absent, borrowed = shelf(b"vendor", package, [named])
    keep += [tags, borrowed]
    entry, tags = shelf_entry(PRESET_ID, THUMBNAIL_ID)
    untouched, borrowed = shelf(b"vendor", package, [entry])
    keep += [tags, borrowed]
    refusals = [
        enumerate_shelves([unlisted], CAPI.CTEX_RESULT_INVALID_ARGUMENT)[1],
        enumerate_shelves([absent], CAPI.CTEX_RESULT_INVALID_ARGUMENT)[1],
        enumerate_shelves([untouched], CAPI.CTEX_RESULT_INVALID_ARGUMENT)[1],
        enumerate_shelves([published, published], CAPI.CTEX_RESULT_INVALID_ARGUMENT)[1],
    ]
    assert [message.split(": ", 1)[1] for message in refusals] == [
        "preset shelf requires exactly one metadata entry per asset",
        "shelf metadata names an absent preset 'presets/absent'",
        f"shelf preset thumbnail is absent: {THUMBNAIL_ID.decode()}",
        "preset library repeats shelf identity 'studio'"]

    broken = enumerate_shelves([untouched], CAPI.CTEX_RESULT_INVALID_ARGUMENT)[0]
    call("ctex_preset_library_resolve", CAPI.byref(broken), CAPI.String(PRESET_ID),
         CAPI.byref(container), None, 0, None, 0, expect=CAPI.CTEX_RESULT_INVALID_ARGUMENT)
    # Resolution validates the whole library first: the shelf is refused, not the preset.
    assert CAPI.ctex_get_last_diagnostic().endswith(
        b"shelf preset thumbnail is absent: " + THUMBNAIL_ID)
    assert container.asset_count == 0 and keep
    SUMMARY["shelf_refusals"] = [message.split(": ", 1)[1] for message in refusals]
    return SUMMARY["shelf_refusals"]


def delivered_record() -> bytes:
    """The vendor also ships the editable record behind the material."""
    channel = CAPI.ctex_operation_channel_descriptor(
        CAPI.CTEX_OPERATION_CHANNEL_DESCRIPTOR_CURRENT_SIZE, CAPI.String(b"pbr.base_color"), 3,
        CAPI.CTEX_SCALAR_REPRESENTATION_UNSIGNED_NORMALIZED, 8,
        CAPI.CTEX_COLOR_SPACE_SRGB_REC709, (ctypes.c_double * 3)(0.0, 0.0, 0.0), 3)
    payload = CAPI.create_string_buffer(b"resolved-stamps-v1")
    descriptor = sized(
        "ctex_operation_record_descriptor", identifier=CAPI.String(RECORD_ID),
        algorithm_identifier=CAPI.String(ALGORITHM), algorithm_version=2,
        preset_identifier=CAPI.String(PRESET_ID), preset_version=1,
        replay_class=CAPI.CTEX_OPERATION_REPLAY_RESOLUTION_INDEPENDENT,
        input_document_revision=4, seed=1729,
        mesh_content_identity=CAPI.String(b"sha256:vendor-mesh"),
        coordinate_frame=(ctypes.c_double * 16)(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1),
        payload_kind=CAPI.CTEX_OPERATION_PAYLOAD_RESOLVED_STAMPS, payload_version=1,
        channels=ctypes.pointer(channel), channel_count=1,
        payload=ctypes.cast(payload, ctypes.c_void_p), payload_size=18)
    info, head = sized("ctex_operation_record_info"), (CAPI.byref(descriptor),)
    call("ctex_operation_record_create", *head, CAPI.byref(info), None, 0, None, 0)
    record = CAPI.create_string_buffer(info.canonical_size)
    call("ctex_operation_record_create", *head, CAPI.byref(info), record, len(record), None, 0)
    assert info.replay_class == CAPI.CTEX_OPERATION_REPLAY_RESOLUTION_INDEPENDENT
    assert (info.channel_count, info.checkpoint_image_count, info.seed) == (1, 0, 1729)
    return record.raw[: info.canonical_size]


def assess_replay(project: bytes, maximum_version: int) -> tuple[object, dict]:
    """Ask whether a build supporting versions 1..N could still replay the drop."""
    support = CAPI.ctex_operation_algorithm_support_descriptor(
        CAPI.CTEX_OPERATION_ALGORITHM_SUPPORT_DESCRIPTOR_CURRENT_SIZE, CAPI.String(ALGORITHM),
        1, maximum_version)
    assessment = CAPI.ctex_operation_replay_assessment_descriptor(
        CAPI.CTEX_OPERATION_REPLAY_ASSESSMENT_DESCRIPTOR_CURRENT_SIZE, ctypes.pointer(support), 1, 0)
    info = sized("ctex_project_operation_replay_info")
    head = (CAPI.create_string_buffer(project), len(project), None, CAPI.byref(assessment),
            CAPI.byref(info))
    call("ctex_project_container_assess_operation_replay", *head, None, 0)
    report = CAPI.create_string_buffer(info.required_report_size)
    call("ctex_project_container_assess_operation_replay", *head, report, len(report))
    assert len(report.value) + 1 == info.required_report_size
    return info, json.loads(report.value.decode("utf-8"))["operation_records"][0]


def vet_the_records() -> None:
    info = sized("ctex_project_container_info")
    empty, record = empty_container(), delivered_record()
    project, _ = two_call("ctex_project_container_upsert_operation_record", (
        empty, len(empty), None, CAPI.create_string_buffer(record), len(record)), info)
    assert info.asset_count == 1
    current, entry = assess_replay(project, 3)
    assert (current.record_count, current.replay_available_count) == (1, 1)
    assert current.unsupported_algorithm_count == 0 and entry["replay_available"] is True
    assert entry["id"] == RECORD_ID.decode()
    stale, refused = assess_replay(project, 1)
    assert (stale.record_count, stale.replay_available_count) == (1, 0)
    assert stale.unsupported_algorithm_count == 1 and refused["checkpoint_available"] is False
    assert refused["diagnostic"].startswith(f"algorithm {ALGORITHM.decode()} version 2")
    SUMMARY.update(replay_diagnostic=refused["diagnostic"],
                   replay_report_bytes=stale.required_report_size)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--executor", default="cpu")
    arguments = parser.parse_args()
    decoded, detected = decode_thumbnail()
    refuse_a_cancelled_decode()
    name_the_colour_spaces(detected)
    vet_the_shelf(delivered_material())
    vet_the_records()

    arguments.output.mkdir(parents=True, exist_ok=True)
    scaled = np.repeat(np.repeat(decoded, 16, axis=0), 16, axis=1)
    (arguments.output / "vendor_thumbnail.png").write_bytes(cybertexel.encode_image(scaled))
    (arguments.output / "summary.json").write_text(
        json.dumps(SUMMARY, indent=2, sort_keys=True) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
