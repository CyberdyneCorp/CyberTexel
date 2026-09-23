#!/usr/bin/env python3
"""Create and round-trip a replayable editable operation record.

Capabilities: editable-authoring.
The record pins the exact brush-alpha bytes and survives insertion into and
extraction from a canonical project container.
"""

from __future__ import annotations

import argparse
import ctypes
import hashlib
import json
from pathlib import Path

import cybertexel


ROOT = Path(__file__).resolve().parent
CAPABILITIES = ("editable-authoring",)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--executor", default="cpu")
    arguments = parser.parse_args()
    assert arguments.executor == "cpu", "operation records are executor-independent"

    capi = cybertexel.capi
    alpha = (ROOT / "fixtures" / "alphas" / "soft_round.png").read_bytes()
    alpha_storage = ctypes.create_string_buffer(alpha)
    alpha_identity = f"sha256:{hashlib.sha256(alpha).hexdigest()}"
    defaults = (ctypes.c_double * 3)(0.0, 0.0, 0.0)
    channel = capi.ctex_operation_channel_descriptor(
        capi.CTEX_OPERATION_CHANNEL_DESCRIPTOR_CURRENT_SIZE,
        capi.String(b"pbr.base_color"),
        3,
        capi.CTEX_SCALAR_REPRESENTATION_UNSIGNED_NORMALIZED,
        8,
        capi.CTEX_COLOR_SPACE_SRGB_REC709,
        defaults,
        3,
    )
    pinned = capi.ctex_pinned_operation_resource_descriptor(
        capi.CTEX_PINNED_OPERATION_RESOURCE_DESCRIPTOR_CURRENT_SIZE,
        capi.String(b"tip-alpha"),
        capi.String(alpha_identity.encode()),
        ctypes.cast(alpha_storage, ctypes.c_void_p),
        len(alpha),
    )
    payload = ctypes.create_string_buffer(b"resolved-stamps-v1")
    record_descriptor = capi.ctex_operation_record_descriptor()
    record_descriptor.size = capi.CTEX_OPERATION_RECORD_DESCRIPTOR_CURRENT_SIZE
    record_descriptor.identifier = capi.String(b"operations/stroke-7")
    record_descriptor.algorithm_identifier = capi.String(b"cybertexel.paint.brush")
    record_descriptor.algorithm_version = 1
    record_descriptor.preset_identifier = capi.String(b"brushes/soft-round")
    record_descriptor.preset_version = 1
    record_descriptor.replay_class = capi.CTEX_OPERATION_REPLAY_RESOLUTION_INDEPENDENT
    record_descriptor.input_document_revision = 6
    record_descriptor.seed = 1729
    record_descriptor.mesh_content_identity = capi.String(b"sha256:fixture-quad")
    record_descriptor.coordinate_frame = (ctypes.c_double * 16)(
        1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1
    )
    record_descriptor.payload_kind = capi.CTEX_OPERATION_PAYLOAD_RESOLVED_STAMPS
    record_descriptor.payload_version = 1
    record_descriptor.channels = ctypes.pointer(channel)
    record_descriptor.channel_count = 1
    record_descriptor.pinned_resources = ctypes.pointer(pinned)
    record_descriptor.pinned_resource_count = 1
    record_descriptor.payload = ctypes.cast(payload, ctypes.c_void_p)
    record_descriptor.payload_size = len(b"resolved-stamps-v1")

    record_info = capi.ctex_operation_record_info()
    record_info.size = capi.CTEX_OPERATION_RECORD_INFO_CURRENT_SIZE
    assert (
        capi.ctex_operation_record_create(
            capi.byref(record_descriptor), capi.byref(record_info), None, 0, None, 0
        )
        == capi.CTEX_RESULT_SUCCESS
    )
    record = ctypes.create_string_buffer(record_info.canonical_size)
    record_report = ctypes.create_string_buffer(record_info.report_size)
    assert (
        capi.ctex_operation_record_create(
            capi.byref(record_descriptor),
            capi.byref(record_info),
            record,
            len(record),
            record_report,
            len(record_report),
        )
        == capi.CTEX_RESULT_SUCCESS
    )

    empty_size = capi.c_size_t()
    assert capi.ctex_project_container_create_empty(None, 0, capi.byref(empty_size)) == 0
    empty = ctypes.create_string_buffer(empty_size.value)
    assert (
        capi.ctex_project_container_create_empty(empty, len(empty), capi.byref(empty_size)) == 0
    )
    project_info = capi.ctex_project_container_info()
    project_info.size = capi.CTEX_PROJECT_CONTAINER_INFO_CURRENT_SIZE
    result = capi.ctex_project_container_upsert_operation_record(
        empty,
        len(empty),
        None,
        record,
        len(record),
        capi.byref(project_info),
        None,
        0,
        None,
        0,
    )
    assert result == capi.CTEX_RESULT_SUCCESS, str(capi.ctex_get_last_diagnostic())
    project = ctypes.create_string_buffer(project_info.canonical_size)
    project_report = ctypes.create_string_buffer(project_info.report_size)
    assert (
        capi.ctex_project_container_upsert_operation_record(
            empty,
            len(empty),
            None,
            record,
            len(record),
            capi.byref(project_info),
            project,
            len(project),
            project_report,
            len(project_report),
        )
        == capi.CTEX_RESULT_SUCCESS
    )
    restored_size = capi.c_size_t()
    identifier = b"operations/stroke-7"
    assert (
        capi.ctex_project_container_get_operation_record(
            project, len(project), None, identifier, None, 0, capi.byref(restored_size)
        )
        == capi.CTEX_RESULT_SUCCESS
    )
    restored = ctypes.create_string_buffer(restored_size.value)
    assert (
        capi.ctex_project_container_get_operation_record(
            project,
            len(project),
            None,
            identifier,
            restored,
            len(restored),
            capi.byref(restored_size),
        )
        == capi.CTEX_RESULT_SUCCESS
    )
    assert restored.raw == record.raw
    assert record_info.pinned_resource_bytes == len(alpha)
    assert record_info.checkpoint_image_count == 0

    arguments.output.mkdir(parents=True, exist_ok=True)
    (arguments.output / "stroke.operation").write_bytes(record.raw)
    (arguments.output / "editable.ctex").write_bytes(project.raw)
    (arguments.output / "record_report.json").write_bytes(record_report.value + b"\n")
    (arguments.output / "project_report.json").write_bytes(project_report.value + b"\n")
    summary = {
        "capabilities": list(CAPABILITIES),
        "checkpoint_count": record_info.checkpoint_image_count,
        "operation_bytes": len(record),
        "operation_sha256": hashlib.sha256(record.raw).hexdigest(),
        "pinned_alpha_bytes": record_info.pinned_resource_bytes,
        "pinned_alpha_identity": alpha_identity,
        "project_assets": project_info.asset_count,
        "project_bytes": len(project),
        "seed": record_info.seed,
    }
    (arguments.output / "summary.json").write_text(
        json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )


if __name__ == "__main__":
    main()
