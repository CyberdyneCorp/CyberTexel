#!/usr/bin/env python3
"""Pick an executor route, survive a device loss, and gate CPU/host parity.

Capabilities: execution-backends, host-transport.
A host enumerates the routes it was compiled with, asks for one by name, and
pins a process default. Its GPU then disappears mid-frame: the session publishes
what a checkpoint can restore, cancels what cannot finish, names every resource
the host must recreate, and only with recovery restored authorises the CPU
fallback. That route rasterizes a camera view and a UV tile and runs staged work
under a memory ceiling it refuses to exceed; the parity gate then proves the two
routes agree inside the declared tolerance.
"""

from __future__ import annotations

import argparse
import ctypes
import json
import os
from pathlib import Path

import cybertexel
import numpy as np


CAPABILITIES = ("execution-backends", "host-transport")

capi = cybertexel.capi
byref, pointer, text, buffer = capi.byref, capi.pointer, capi.String, capi.create_string_buffer
RGBA8 = 2
SHARED_BYTES, WORKER_BYTES = 64, 8
CPU_VALUES, HOST_VALUES = (0.25, 0.5, 0.75, 1.0), (0.252, 0.5, 0.75, 1.0)
DRIFTED_VALUES = (0.35, 0.5, 0.75, 1.0)


def ok(result: int, expected: int = 0) -> None:
    assert result == expected, (result, capi.ctex_get_last_diagnostic())


def sized(kind: object, size: int) -> object:
    value = kind()
    value.size = size
    return value


def open_registry() -> tuple[object, object]:
    formats = (capi.uint32_t * 2)(RGBA8, capi.CTEX_EXECUTOR_TEXTURE_RGBA16_FLOAT)
    host = capi.ctex_host_executor_descriptor(
        capi.CTEX_HOST_EXECUTOR_DESCRIPTOR_CURRENT_SIZE, text(b"Example host GPU"),
        24, 16384, formats, 2, 1, 1, 1)
    registry = capi.POINTER(capi.ctex_executor_registry)()
    ok(capi.ctex_executor_registry_create(byref(host), byref(registry)))
    return registry, formats


def read_executors(registry: object) -> dict[str, dict]:
    read = capi.ctex_executor_registry_get_info
    count = capi.c_size_t()
    ok(capi.ctex_executor_registry_get_count(registry, byref(count)))
    assert count.value >= 2
    table: dict[str, dict] = {}
    for index in range(count.value):
        # One call sizes every caller-owned buffer, the next fills them.
        info = sized(capi.ctex_executor_info, capi.CTEX_EXECUTOR_INFO_CURRENT_SIZE)
        ok(read(registry, index, byref(info), None, 0, None, 0, None, 0, None, 0))
        supported = (capi.uint32_t * info.supported_texture_format_count)()
        name, device = buffer(info.required_identifier_size), buffer(
            info.required_device_name_size)
        display = buffer(info.required_display_name_size)
        ok(read(registry, index, byref(info), supported, len(supported), name, len(name),
                display, len(display), device, len(device)))
        table[name.value.decode()] = {
            "index": index, "route": info.route, "availability": info.availability,
            "budget": info.binding_budget, "device": device.value.decode(),
            "display": display.value.decode(), "formats": list(supported)}
    cpu, host = table["cpu"], table["host"]
    assert cpu["route"] == capi.CTEX_EXECUTOR_ROUTE_CPU_REFERENCE
    assert cpu["availability"] == capi.CTEX_EXECUTOR_AVAILABLE and len(cpu["formats"]) == 13
    # Route and display name are the registry's; budget, device and formats are the host's own.
    assert (host["route"], host["display"]) == (capi.CTEX_EXECUTOR_ROUTE_HOST_EXECUTED, "Host executed")
    assert (host["budget"], host["device"]) == (24, "Example host GPU")
    assert host["formats"] == [RGBA8, capi.CTEX_EXECUTOR_TEXTURE_RGBA16_FLOAT]
    return table


def choose_route(registry: object, table: dict[str, dict]) -> dict[str, str]:
    info_size = capi.CTEX_EXECUTOR_SELECTION_INFO_CURRENT_SIZE
    selection = sized(capi.ctex_executor_selection_info, info_size)
    select = capi.ctex_executor_registry_select
    ok(select(registry, text(b"host"), byref(selection), None, 0, None, 0))
    assert selection.source == capi.CTEX_EXECUTOR_SELECTION_EXPLICIT and selection.selected_executor_index == table["host"]["index"]
    message = buffer(selection.required_message_size)
    ok(select(registry, text(b"host"), byref(selection), None, 0, message, len(message)))
    assert b"requested" in message.value
    # With no environment override the pinned process default answers a null request.
    inherited = os.environ.pop("CTEX_EXECUTOR", None)
    ok(capi.ctex_executor_registry_pin_default(registry, text(b"cpu")))
    selection.size = info_size
    ok(select(registry, None, byref(selection), None, 0, None, 0))
    assert selection.source == capi.CTEX_EXECUTOR_SELECTION_EXPLICIT and selection.selected_executor_index == table["cpu"]["index"]
    pinned = buffer(selection.required_message_size)
    ok(select(registry, None, byref(selection), None, 0, pinned, len(pinned)))
    assert b"pinned process default" in pinned.value
    ok(capi.ctex_executor_registry_clear_default(registry))
    selection.size = info_size
    ok(select(registry, None, byref(selection), None, 0, None, 0))
    assert selection.source == capi.CTEX_EXECUTOR_SELECTION_AUTOMATIC
    os.environ["CTEX_EXECUTOR"] = inherited or "cpu"
    selection.size = info_size
    ok(select(registry, None, byref(selection), None, 0, None, 0))
    assert selection.source == capi.CTEX_EXECUTOR_SELECTION_ENVIRONMENT
    return {"explicit": message.value.decode(), "pinned": pinned.value.decode()}


def lose_the_device() -> dict[str, object]:
    state = cybertexel.ResourceState

    def resource(logical_id: str, generation: int, output: bool) -> object:
        return cybertexel.HostResource(
            logical_id, generation, "output" if output else "input", RGBA8, 64, 32, output,
            externally_initialized=not output,
            required_state=state.RENDER_TARGET if output else state.SHADER_READ)

    def work(source: tuple, target: tuple) -> list:
        return [resource(*source, False), resource(*target, True)]

    finished = cybertexel.CompletedResource("paint", 2, RGBA8, 64, 32)
    record = cybertexel.Recovery("paint-v1", 0, 96)
    with cybertexel.HostExecutionSession() as session:
        raw = ctypes.cast(session._require_open(), capi.POINTER(capi.ctex_host_execution_session))
        first = session.submit("paint", 0, work(("source", 1), ("paint", 1)))
        published = session.complete(
            first, [cybertexel.CompletedResource("paint", 1, RGBA8, 64, 32)], record)
        assert published.disposition is cybertexel.CompletionDisposition.PUBLISHED
        # Checkpoint-only work cannot publish against a deterministic record, so its
        # completion parks until the host establishes the recovery it can really keep.
        deferred = session.submit("paint", 1, work(("paint", 1), ("paint", 2)),
                                  cybertexel.ReplaySemantics.CHECKPOINT_ONLY)
        parked = session.complete(deferred, [finished], record)
        assert parked.disposition is cybertexel.CompletionDisposition.AWAITING_RECOVERY
        checkpoint = capi.ctex_host_recovery_descriptor(
            capi.CTEX_HOST_RECOVERY_DESCRIPTOR_CURRENT_SIZE,
            capi.CTEX_HOST_RECOVERY_RESULT_CHECKPOINT, 1, 2, text(b"paint-v1"), 1, 128)
        result = capi.POINTER(capi.ctex_host_completion_result)()
        ok(capi.ctex_host_execution_session_establish_recovery(
            raw, deferred, byref(checkpoint), byref(result)))
        info = sized(capi.ctex_host_completion_result_info,
                     capi.CTEX_HOST_COMPLETION_RESULT_INFO_CURRENT_SIZE)
        ok(capi.ctex_host_completion_result_get_info(
            result, byref(info), None, 0, None, 0, None, 0))
        assert info.disposition == capi.CTEX_HOST_COMPLETION_PUBLISHED
        assert (info.has_published_revision, info.published_revision) == (1, 2)
        capi.ctex_host_completion_result_destroy(result)
        abandoned = session.submit("paint", 2, work(("paint", 2), ("gone", 1)))
        cancelled = capi.uint32_t()
        ok(capi.ctex_host_execution_session_cancel(raw, abandoned, byref(cancelled)))
        assert cancelled.value == 1
        late = session.complete(abandoned, [cybertexel.CompletedResource("gone", 1, RGBA8, 64, 32)])
        assert late.disposition is cybertexel.CompletionDisposition.CANCELLED
        pending = session.submit("paint", 2, work(("paint", 2), ("pending-output", 1)))
        report = capi.POINTER(capi.ctex_host_recovery_report)()
        ok(capi.ctex_host_execution_session_report_device_loss(raw, byref(report)))
        loss = sized(capi.ctex_host_device_loss_info, capi.CTEX_HOST_DEVICE_LOSS_INFO_CURRENT_SIZE)
        read = capi.ctex_host_recovery_report_get_info
        ok(read(report, byref(loss), None, 0, None, 0, None, 0))
        assert loss.recovered_revision == 2 and loss.restored == 1
        assert loss.retained_recovery_bytes == 128
        assert (loss.cancelled_submission_count, loss.released_resource_count) == (1, 1)
        tokens = (capi.uint64_t * loss.cancelled_submission_count)()
        released = (capi.ctex_host_resource_version * loss.released_resource_count)()
        identities = buffer(loss.required_released_identity_size)
        ok(read(report, byref(loss), tokens, len(tokens), released, len(released),
                identities, len(identities)))
        assert tokens[0] == pending and released[0].generation == 1
        name = identities.raw[released[0].logical_id_offset:].split(b"\x00")[0]
        assert name == b"pending-output"
        capi.ctex_host_recovery_report_destroy(report)
        final = sized(capi.ctex_host_execution_session_info,
                      capi.CTEX_HOST_EXECUTION_SESSION_INFO_CURRENT_SIZE)
        ok(capi.ctex_host_execution_session_get_info(raw, byref(final)))
        assert (final.revision, final.active_submission_count) == (2, 0)
    return {"recovered_revision": loss.recovered_revision, "cancelled_submission": int(tokens[0]),
            "released_identity": name.decode(), "retained_bytes": loss.retained_recovery_bytes}


def authorise_fallback() -> str:
    info_size = capi.CTEX_EXECUTOR_FALLBACK_INFO_CURRENT_SIZE
    make = capi.ctex_executor_make_fallback_report
    descriptor = capi.ctex_executor_fallback_descriptor(
        capi.CTEX_EXECUTOR_FALLBACK_DESCRIPTOR_CURRENT_SIZE, text(b"host"),
        capi.CTEX_EXECUTION_FAILURE_DEVICE_LOST, text(b"device removed"),
        capi.CTEX_EXECUTOR_CPU_FALLBACK, text(b"cpu"), 0)
    info = sized(capi.ctex_executor_fallback_info, info_size)
    # A CPU fallback whose recovery state was never restored is refused outright.
    ok(make(byref(descriptor), byref(info), None, 0), capi.CTEX_RESULT_INVALID_ARGUMENT)
    assert capi.ctex_get_last_diagnostic_code() == capi.CTEX_DIAGNOSTIC_INVALID_EXECUTOR
    descriptor.recovery_restored = 1
    info.size = info_size
    ok(make(byref(descriptor), byref(info), None, 0))
    assert info.disposition == capi.CTEX_EXECUTOR_CPU_FALLBACK and info.recovery_restored == 1
    message = buffer(info.required_message_size)
    ok(make(byref(descriptor), byref(info), message, len(message)))
    assert b"fell back to 'cpu'" in message.value and capi.ctex_get_last_result() == 0
    return message.value.decode()


def rasterize_on_the_reference() -> dict[str, object]:
    vec3, vec2 = capi.ctex_vec3f, capi.ctex_vec2f
    positions = (vec3 * 3)(vec3(-1.0, -1.0, 0.0), vec3(1.0, -1.0, 0.0), vec3(-1.0, 1.0, 0.0))
    uv = (vec2 * 3)(vec2(0.0, 0.0), vec2(1.0, 0.0), vec2(0.0, 1.0))
    indices = (capi.uint32_t * 3)(0, 1, 2)
    mesh = capi.ctex_cpu_raster_mesh_descriptor(
        capi.CTEX_CPU_RASTER_MESH_DESCRIPTOR_CURRENT_SIZE, positions, uv, 3, indices, 3)
    camera = sized(capi.ctex_cpu_raster_camera_descriptor,
                   capi.CTEX_CPU_RASTER_CAMERA_DESCRIPTOR_CURRENT_SIZE)
    camera.view_projection[:] = [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1]
    camera.width, camera.height = 4, 4
    depth, coordinates = (capi.c_float * 16)(), (vec2 * 16)()
    coverage, triangles = (capi.c_ubyte * 16)(), (capi.uint32_t * 16)()
    outputs = capi.ctex_cpu_raster_outputs(
        capi.CTEX_CPU_RASTER_OUTPUTS_CURRENT_SIZE, depth, 16, coordinates, 16,
        coverage, 16, triangles, 16)
    viewport = capi.ctex_cpu_viewport_raster_descriptor(
        capi.CTEX_CPU_VIEWPORT_RASTER_DESCRIPTOR_CURRENT_SIZE, pointer(mesh), pointer(camera), 16)
    info = sized(capi.ctex_cpu_raster_info, capi.CTEX_CPU_RASTER_INFO_CURRENT_SIZE)
    raster = capi.ctex_cpu_reference_rasterize_viewport
    ok(raster(byref(viewport), byref(info), None))
    assert (info.width, info.height, info.pixel_count) == (4, 4, 16)
    ok(raster(byref(viewport), byref(info), byref(outputs)))
    covered = info.width  # row 1, column 0: u != v there, and texel 1 is uncovered
    assert coverage[covered] == 1 and triangles[covered] == 0
    np.testing.assert_allclose(
        [depth[covered], coordinates[covered].x, coordinates[covered].y], [0.5, 0.125, 0.625], atol=1e-5)
    covered_pixels = sum(int(value) for value in coverage)
    assert coverage[3] == 0 and triangles[3] == 0xFFFFFFFF and 0 < covered_pixels < 16
    # The caller-declared pixel ceiling is a refusal, never a silent clamp.
    viewport.maximum_output_pixels = 15
    ok(raster(byref(viewport), byref(info), None), capi.CTEX_RESULT_OVER_BUDGET)
    # Scaling the camera moves the projected point off the texel centre an echo would report.
    camera.view_projection[0], camera.view_projection[5] = 0.5, 0.25
    uv_raster = capi.ctex_cpu_uv_raster_descriptor(
        capi.CTEX_CPU_UV_RASTER_DESCRIPTOR_CURRENT_SIZE, pointer(mesh), pointer(camera), 4, 4,
        vec2(0.0, 0.0), 16)
    info.size = capi.CTEX_CPU_RASTER_INFO_CURRENT_SIZE
    ok(capi.ctex_cpu_reference_rasterize_uv(byref(uv_raster), byref(info), byref(outputs)))
    assert info.pixel_count == 16 and coverage[covered] == 1 and triangles[covered] == 0
    np.testing.assert_allclose(
        [depth[covered], coordinates[covered].x, coordinates[covered].y], [0.5, 1.25, 1.875], atol=1e-5)
    return {"covered_pixels": covered_pixels, "projected_screen": [1.25, 1.875]}


def run_bounded_cpu_work() -> dict[str, object]:
    committed: dict[str, bytes] = {}
    as_bytes = capi.POINTER(capi.c_ubyte)

    def execute(item, shared, shared_size, worker, worker_size, user_data):
        if worker_size:
            capi.cast(worker, as_bytes)[0] = 0x5A
        capi.cast(shared, as_bytes)[item] = (item % 251) + 1
        return 0

    def publish(shared, shared_size, user_data):
        committed["bytes"] = bytes(capi.cast(shared, as_bytes)[:shared_size])
        return 0

    info_size = capi.CTEX_CPU_EXECUTION_INFO_CURRENT_SIZE
    read = capi.ctex_cpu_execution_result_get_info
    ceiling = SHARED_BYTES + WORKER_BYTES
    descriptor = capi.ctex_cpu_bounded_execution_descriptor(
        capi.CTEX_CPU_BOUNDED_EXECUTION_DESCRIPTOR_CURRENT_SIZE, text(b"staged-fill"),
        SHARED_BYTES, SHARED_BYTES, WORKER_BYTES, 1, ceiling, 16,
        capi.ctex_cpu_work_item_callback(execute), capi.ctex_cpu_commit_callback(publish),
        capi.ctex_cpu_cancel_callback(), capi.ctex_cpu_progress_callback(), None)
    result = capi.POINTER(capi.ctex_cpu_execution_result)()
    ok(capi.ctex_cpu_execute_bounded(byref(descriptor), byref(result)))
    info = sized(capi.ctex_cpu_execution_info, info_size)
    ok(read(result, byref(info), None, 0))
    message = buffer(info.required_message_size)
    ok(read(result, byref(info), message, len(message)))
    assert info.status == capi.CTEX_CPU_EXECUTION_COMPLETED and b"completed" in message.value
    assert (info.completed_work_items, info.total_work_items) == (SHARED_BYTES, SHARED_BYTES)
    assert info.required_memory_bytes == ceiling and info.worker_count == 1
    assert committed["bytes"] == bytes((index % 251) + 1 for index in range(SHARED_BYTES))
    capi.ctex_cpu_execution_result_destroy(result)
    # A ceiling one byte under the staged requirement refuses before any work runs.
    committed.clear()
    descriptor.memory_ceiling_bytes = ceiling - 1
    refused = capi.POINTER(capi.ctex_cpu_execution_result)()
    ok(capi.ctex_cpu_execute_bounded(byref(descriptor), byref(refused)))
    info.size = info_size
    ok(read(refused, byref(info), None, 0))
    assert info.status == capi.CTEX_CPU_EXECUTION_MEMORY_CEILING_EXCEEDED and not committed
    assert info.completed_work_items == 0 and info.worker_count == 0
    assert info.required_memory_bytes == ceiling
    capi.ctex_cpu_execution_result_destroy(refused)
    return {"staged_bytes": SHARED_BYTES, "required_memory_bytes": info.required_memory_bytes}


def compare_values() -> dict[str, float]:
    size = capi.CTEX_PARITY_TOLERANCE_INFO_CURRENT_SIZE
    tolerance = sized(capi.ctex_parity_tolerance_info, size)
    ok(capi.ctex_executor_parity_get_tolerance(capi.CTEX_PARITY_UNORM8, 0, byref(tolerance)))
    assert (tolerance.absolute, tolerance.relative) == (1.0 / 255.0, 0.0)
    reference, drifted = (capi.c_double * 4)(*CPU_VALUES), (capi.c_double * 4)(*DRIFTED_VALUES)
    info = sized(capi.ctex_parity_comparison_info, capi.CTEX_PARITY_COMPARISON_INFO_CURRENT_SIZE)
    compare = capi.ctex_executor_compare_parity
    ok(compare(reference, drifted, 4, capi.CTEX_PARITY_UNORM8, 0, byref(info), None, 0))
    message = buffer(info.required_message_size)
    ok(compare(reference, drifted, 4, capi.CTEX_PARITY_UNORM8, 0, byref(info), message,
               len(message)))
    assert info.matches == 0 and info.has_failure == 1 and info.failure_value_index == 0 and info.compared_value_count == 4
    assert info.failure_absolute_deviation > info.failure_allowed_deviation
    np.testing.assert_allclose(info.maximum_absolute_deviation, 0.1, atol=1e-12)
    assert b"value 0" in message.value
    return {"unorm8_absolute": tolerance.absolute,
            "maximum_absolute_deviation": info.maximum_absolute_deviation}


def renderer(samples: tuple, held: list) -> object:
    def render(fixture, out_rendered, user_data):
        pixels = (capi.c_double * 4)(*samples)
        channel = capi.ctex_parity_rendered_channel_descriptor(
            capi.CTEX_PARITY_RENDERED_CHANNEL_DESCRIPTOR_CURRENT_SIZE,
            text(b"base_color"), pixels, 4)
        held.append((pixels, channel))
        out_rendered[0] = capi.ctex_parity_rendered_fixture_descriptor(
            capi.CTEX_PARITY_RENDERED_FIXTURE_DESCRIPTOR_CURRENT_SIZE, 1, 1, pointer(channel), 1)
        return 0

    callback = capi.ctex_parity_render_callback(render)
    held.append(callback)
    return callback


def run_gate(registry: object, table: dict[str, dict], measured: tuple) -> dict[str, object]:
    channel = capi.ctex_parity_fixture_channel_descriptor(
        capi.CTEX_PARITY_FIXTURE_CHANNEL_DESCRIPTOR_CURRENT_SIZE,
        text(b"base_color"), capi.CTEX_PARITY_UNORM8, 0, 4)
    fixture = capi.ctex_parity_fixture_descriptor(
        capi.CTEX_PARITY_FIXTURE_DESCRIPTOR_CURRENT_SIZE, text(b"paint-basic"),
        text(b"document-v1"), text(b"stroke-v1"), text(b"camera-v1"), text(b"material-v1"),
        pointer(channel), 1)
    held: list = []
    binding = capi.ctex_parity_executor_binding_descriptor
    size = capi.CTEX_PARITY_EXECUTOR_BINDING_DESCRIPTOR_CURRENT_SIZE
    bindings = (binding * 2)(
        binding(size, table["cpu"]["index"], renderer(CPU_VALUES, held), None),
        binding(size, table["host"]["index"], renderer(measured, held), None))
    result = capi.POINTER(capi.ctex_parity_gate_result)()
    ok(capi.ctex_executor_run_parity_gate(registry, byref(fixture), 1, bindings, 2, byref(result)))
    info = sized(capi.ctex_parity_gate_info, capi.CTEX_PARITY_GATE_INFO_CURRENT_SIZE)
    read = capi.ctex_parity_gate_result_get_info
    ok(read(result, byref(info), None, 0))
    report = buffer(info.required_report_size)
    ok(read(result, byref(info), report, len(report)))
    assert info.executor_count == 2 and info.reference_count == 1
    assert b'"executor":"cpu"' in report.value and b'"device":"Example host GPU"' in report.value
    assert len(held) == 4, "both bound executors must have rendered the fixture"
    summary = {"passed": int(info.passed), "passed_count": info.passed_count,
               "failed_count": info.failed_count, "drift": b"measuredDeviation" in report.value,
               "names_fixture": b"paint-basic" in report.value}
    capi.ctex_parity_gate_result_destroy(result)
    return summary


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--executor", default="cpu")
    arguments = parser.parse_args()

    registry, _formats = open_registry()
    table = read_executors(registry)
    summary = {
        "capabilities": list(CAPABILITIES),
        "executors": sorted(table),
        "selection_messages": choose_route(registry, table),
        "device_loss": lose_the_device(),
        "fallback_message": authorise_fallback(),
        "raster": rasterize_on_the_reference(),
        "staged_execution": run_bounded_cpu_work(),
        "numeric_contract": compare_values(),
        "in_tolerance_gate": run_gate(registry, table, HOST_VALUES),
        "out_of_tolerance_gate": run_gate(registry, table, DRIFTED_VALUES),
    }
    capi.ctex_executor_registry_destroy(registry)

    agreed, drifted = summary["in_tolerance_gate"], summary["out_of_tolerance_gate"]
    assert agreed["passed"] == 1 and agreed["passed_count"] == 1 and agreed["failed_count"] == 0
    assert not agreed["drift"] and drifted["passed"] == 0 and drifted["failed_count"] == 1
    assert drifted["drift"] and drifted["names_fixture"]

    arguments.output.mkdir(parents=True, exist_ok=True)
    (arguments.output / "device_loss_fallback.json").write_text(
        json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
