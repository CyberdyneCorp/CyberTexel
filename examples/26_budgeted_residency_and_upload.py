#!/usr/bin/env python3
"""Account every resident byte, then upload only the tiles a host has not seen.

Capabilities: resource-residency, host-transport, c-abi.
A host owns the allocator and the log sink, so every byte and every refusal is
its own. It records what it holds in the resource ledger, which admits large
work as bounded tile batches, evicts cache before breaking a limit, and degrades
preview quality rather than overrunning. It then pins a snapshot of an in-flight
paint preview, negotiates the pixel format it can upload, reads the converted
tile back, drives asynchronous readbacks through completion, cancellation and
device-loss failure, and finally spills a cold tile to its own backing store.
Two checks are non-mutation contracts rather than computed results: a refused
report and an unpublished readback must leave the host's sentinel bytes intact.
"""

from __future__ import annotations

import argparse
import ctypes
import json
from pathlib import Path

import cybertexel


CAPABILITIES = ("resource-residency", "host-transport", "c-abi")

capi = cybertexel.capi
byref, pointer, text, buffer = capi.byref, capi.pointer, capi.String, capi.create_string_buffer
SEMANTIC = b"pbr.base_color"
EXTENT = 70
TILE = 64
TILE_BYTES = TILE * TILE * 3
CPU, GPU = capi.CTEX_RESOURCE_CPU_RESIDENT, capi.CTEX_RESOURCE_GPU_RESIDENT
PIXEL = (1, 2, 3)
COMMITTED = (9, 8, 7)
RETAINED: list = []  # native callbacks outlive the call that registers them


def ok(result: int, expected: int = 0) -> None:
    assert result == expected, (result, capi.ctex_get_last_diagnostic())


def sized(kind: object, size: int) -> object:
    value = kind()
    value.size = size
    return value


def allocation(identity: int, category: int, physical: int, roles: int, device=False) -> object:
    fields = (capi.CTEX_RESOURCE_ALLOCATION_DESCRIPTOR_CURRENT_SIZE, identity, category,
              physical, roles)
    if not device:
        return capi.ctex_resource_allocation_descriptor(*fields)
    return capi.ctex_resource_allocation_descriptor(
        *fields, text(b"metal"), text(b"apple-gpu"), text(b"shared"))


def requirement(category: int, physical: int) -> object:
    return capi.ctex_resource_requirement(
        capi.CTEX_RESOURCE_REQUIREMENT_CURRENT_SIZE, category, physical, CPU)


def limits(cpu: int) -> object:
    return capi.ctex_resource_budget_limits(
        capi.CTEX_RESOURCE_BUDGET_LIMITS_CURRENT_SIZE, cpu, 5000, 3000, 200)


def account_residency() -> dict[str, object]:
    evicted, live, logs, counts = [], {}, [], [0]

    def record_eviction(identity, user_data):
        evicted.append(int(identity))

    def allocate(size, alignment, user_data):
        block = (capi.c_ubyte * (size + alignment))()
        address = capi.addressof(block)
        aligned = address + (-address % alignment)
        live[aligned] = block
        counts[0] += 1
        return aligned

    def free(address, size, alignment, user_data):
        del live[address]

    def log(severity, category, message, user_data):
        logs.append((int(severity), str(category), str(message)))

    allocator = capi.ctex_allocator_descriptor(
        capi.CTEX_ALLOCATOR_DESCRIPTOR_CURRENT_SIZE, capi.ctex_allocate_callback(allocate),
        capi.ctex_deallocate_callback(free), None)
    sink = capi.ctex_log_sink_descriptor(
        capi.CTEX_LOG_SINK_DESCRIPTOR_CURRENT_SIZE, capi.ctex_log_callback(log), None,
        capi.CTEX_LOG_SEVERITY_ERROR)
    ok(capi.ctex_set_allocator(byref(allocator)))
    ok(capi.ctex_set_log_sink(byref(sink)))
    on_evict = capi.ctex_resource_cache_eviction_callback(record_eviction)
    ledger = capi.POINTER(capi.ctex_resource_ledger)()
    ok(capi.ctex_resource_ledger_create(byref(ledger)))
    ok(capi.ctex_resource_ledger_set_cache_eviction_callback(ledger, on_evict, None))
    for descriptor in (
        allocation(41, capi.CTEX_RESOURCE_DOCUMENT_STORAGE, 4096,
                   CPU | GPU | capi.CTEX_RESOURCE_PINNED, device=True),
        allocation(42, capi.CTEX_RESOURCE_HISTORY, 1024, CPU | capi.CTEX_RESOURCE_IN_FLIGHT),
        allocation(43, capi.CTEX_RESOURCE_RECOVERY_RECORD, 2048, capi.CTEX_RESOURCE_BACKING_STORE),
    ):
        ok(capi.ctex_resource_ledger_upsert(ledger, byref(descriptor)))
    # A GPU-resident allocation without a device descriptor is not accountable.
    orphan = allocation(44, capi.CTEX_RESOURCE_TEMPORARY, 64, GPU)
    ok(capi.ctex_resource_ledger_upsert(ledger, byref(orphan)), capi.CTEX_RESULT_INVALID_ARGUMENT)
    assert logs[0][0] == capi.CTEX_LOG_SEVERITY_ERROR and logs[0][1] == "capi.diagnostic"
    assert "ctex_resource_ledger_upsert" in logs[0][2]

    report_size = capi.CTEX_RESOURCE_ACCOUNTING_REPORT_CURRENT_SIZE
    count = capi.CTEX_RESOURCE_CATEGORY_COUNT
    report = sized(capi.ctex_resource_accounting_report, report_size)
    ok(capi.ctex_resource_ledger_get_report(ledger, byref(report), None, 0))
    assert report.category_count == count
    categories = (capi.ctex_resource_category_report * count)()
    short = sized(capi.ctex_resource_accounting_report, report_size)
    short.physical_bytes = 77  # a sentinel the refused report must not overwrite
    ok(capi.ctex_resource_ledger_get_report(ledger, byref(short), categories, count - 1),
       capi.CTEX_RESULT_BUFFER_TOO_SMALL)
    assert short.physical_bytes == 77 and categories[0].physical_bytes == 0
    ok(capi.ctex_resource_ledger_get_report(ledger, byref(report), categories, count))
    assert (report.allocation_count, report.physical_bytes) == (3, 7168)
    assert (report.cpu_resident_bytes, report.gpu_resident_bytes) == (5120, 4096)
    assert (report.backing_store_bytes, report.pinned_bytes, report.in_flight_bytes) == (2048, 4096, 1024)
    document = categories[capi.CTEX_RESOURCE_DOCUMENT_STORAGE]
    assert document.physical_bytes == 4096 and document.gpu_resident_bytes == 4096
    removed = capi.uint32_t()
    ok(capi.ctex_resource_ledger_remove(ledger, 42, byref(removed)))
    assert removed.value == 1
    fixed = requirement(capi.CTEX_RESOURCE_HISTORY, 100)
    admission = capi.ctex_resource_admission_descriptor(
        capi.CTEX_RESOURCE_ADMISSION_DESCRIPTOR_CURRENT_SIZE, text(b"large export"), limits(4596),
        pointer(fixed), 1, requirement(capi.CTEX_RESOURCE_TEMPORARY, 100), 5)
    reservation = capi.POINTER(capi.ctex_resource_reservation)()
    tiled = sized(capi.ctex_resource_admission_report,
                  capi.CTEX_RESOURCE_ADMISSION_REPORT_CURRENT_SIZE)
    ok(capi.ctex_resource_ledger_admit(ledger, byref(admission), byref(reservation), byref(tiled)))
    assert tiled.status == capi.CTEX_RESOURCE_ADMITTED_TILED
    assert (tiled.work_item_count, tiled.admitted_work_items) == (5, 2)
    assert (tiled.projected_cpu_bytes, tiled.projected_temporary_bytes) == (4396, 200)
    capi.ctex_resource_reservation_release(reservation)
    capi.ctex_resource_reservation_destroy(reservation)
    cache = allocation(50, capi.CTEX_RESOURCE_CACHE, 300, CPU)
    checkpoint = capi.ctex_resource_admission_descriptor(
        capi.CTEX_RESOURCE_ADMISSION_DESCRIPTOR_CURRENT_SIZE, text(b"checkpoint"), limits(4400),
        pointer(fixed), 1, requirement(capi.CTEX_RESOURCE_TEMPORARY, 0), 0)
    ok(capi.ctex_resource_ledger_upsert(ledger, byref(cache)))
    pinned = capi.POINTER(capi.ctex_resource_reservation)()
    evicting = sized(capi.ctex_resource_admission_report,
                     capi.CTEX_RESOURCE_ADMISSION_REPORT_CURRENT_SIZE)
    ok(capi.ctex_resource_ledger_admit(ledger, byref(checkpoint), byref(pinned), byref(evicting)))
    assert evicting.evicted_allocation_count == 1 and evicted == [50]
    identities, released = capi.c_size_t(), (capi.uint64_t * 1)()
    evicted_query = capi.ctex_resource_reservation_get_evicted_allocations
    ok(evicted_query(pinned, None, 0, byref(identities)))
    assert identities.value == 1
    ok(evicted_query(pinned, released, 1, byref(identities)))
    assert released[0] == 50
    capi.ctex_resource_reservation_destroy(pinned)
    wanted = requirement(capi.CTEX_RESOURCE_COMPOSITE, 800)
    reduced = requirement(capi.CTEX_RESOURCE_COMPOSITE, 200)
    option = capi.ctex_preview_quality_option
    options = (option * 2)(
        option(capi.CTEX_PREVIEW_QUALITY_OPTION_CURRENT_SIZE, 1024, 1024, pointer(wanted), 1, 0),
        option(capi.CTEX_PREVIEW_QUALITY_OPTION_CURRENT_SIZE, 512, 512, pointer(reduced), 1, 1))
    preview = capi.ctex_preview_quality_admission_descriptor(
        capi.CTEX_PREVIEW_QUALITY_ADMISSION_DESCRIPTOR_CURRENT_SIZE, text(b"mobile viewport"),
        limits(4400), 1024, 1024, options, 2)
    spare = allocation(51, capi.CTEX_RESOURCE_CACHE, 200, CPU)
    degraded = capi.POINTER(capi.ctex_resource_reservation)()
    quality = sized(capi.ctex_preview_quality_admission_report,
                    capi.CTEX_PREVIEW_QUALITY_ADMISSION_REPORT_CURRENT_SIZE)
    ok(capi.ctex_resource_ledger_upsert(ledger, byref(spare)))
    ok(capi.ctex_resource_ledger_admit_preview_quality(
        ledger, byref(preview), byref(degraded), byref(quality)))
    assert quality.status == capi.CTEX_PREVIEW_REDUCED_AND_DEFERRED
    assert (quality.selected_option, quality.derived_work_deferred) == (1, 1)
    assert (quality.full_quality_width, quality.selected_width) == (1024, 512)
    assert quality.projected_cpu_bytes == 4296 and evicted == [50, 51]
    capi.ctex_resource_reservation_destroy(degraded)
    capi.ctex_resource_ledger_destroy(ledger)
    ok(capi.ctex_set_allocator(None))
    ok(capi.ctex_set_log_sink(None))
    assert counts[0] > 1 and not live, "the host allocator saw every byte released"
    return {"physical_bytes": report.physical_bytes, "admitted_work_items": tiled.admitted_work_items,
            "evicted_cache_identities": evicted, "preview_width": quality.selected_width,
            "host_allocations": counts[0], "logged_refusals": len(logs)}


def preview_snapshot(pool: object, session: object) -> tuple[object, object]:
    info = sized(capi.ctex_paint_preview_info, capi.CTEX_PAINT_PREVIEW_INFO_CURRENT_SIZE)
    ok(capi.ctex_paint_preview_session_get_info(session, byref(info)))
    cursor = capi.ctex_transport_revision_cursor(info.preview_epoch, info.preview_revision)
    pixel = (capi.c_ubyte * 3)(*PIXEL)
    ok(capi.ctex_paint_preview_session_write_pixel(session, 1, 2, pixel, 3))
    snapshot = capi.POINTER(capi.ctex_transport_snapshot)()
    query = sized(capi.ctex_transport_snapshot_query_info,
                  capi.CTEX_TRANSPORT_SNAPSHOT_QUERY_INFO_CURRENT_SIZE)
    ok(capi.ctex_paint_preview_session_query_snapshot(
        pool, session, cursor, byref(snapshot), byref(query)))
    assert query.changed_tile_count == 1 and query.retained_bytes == TILE_BYTES
    assert query.current_cursor.revision > cursor.revision
    return snapshot, query


def negotiated_layout(snapshot: object, version: object) -> object:
    wanted = capi.ctex_transport_pixel_format(capi.CTEX_TRANSPORT_COMPONENT_UINT16_UNORM, 3)
    selection = sized(capi.ctex_transport_format_selection,
                      capi.CTEX_TRANSPORT_FORMAT_SELECTION_CURRENT_SIZE)
    negotiate = capi.ctex_transport_snapshot_negotiate_format
    # Exact-only negotiation refuses rather than silently converting behind the host.
    ok(negotiate(snapshot, byref(wanted), 1, capi.CTEX_TRANSPORT_EXACT_FORMAT_ONLY,
                 byref(selection)), capi.CTEX_RESULT_UNSUPPORTED_OPERATION)
    ok(negotiate(snapshot, byref(wanted), 1, capi.CTEX_TRANSPORT_ALLOW_FORMAT_CONVERSION,
                 byref(selection)))
    assert selection.source_format.component_type == capi.CTEX_TRANSPORT_COMPONENT_UINT8_UNORM
    assert selection.output_format.component_type == capi.CTEX_TRANSPORT_COMPONENT_UINT16_UNORM
    layout = sized(capi.ctex_transport_tile_memory_layout,
                   capi.CTEX_TRANSPORT_TILE_MEMORY_LAYOUT_CURRENT_SIZE)
    ok(capi.ctex_transport_snapshot_get_tile_memory_layout(
        snapshot, version, byref(selection), byref(layout)))
    assert selection.conversion == capi.CTEX_TRANSPORT_CONVERSION_UINT8_TO_UINT16
    assert (layout.width, layout.height) == (TILE, TILE) and layout.pixel_stride_bytes == 6
    assert layout.row_pitch_bytes == TILE * 6 and layout.byte_size == TILE_BYTES * 2
    assert layout.channel_order == capi.CTEX_TRANSPORT_CHANNEL_ORDER_RGB
    return selection, layout


def upload_changed_tiles(document: object, texture_set: str) -> dict[str, object]:
    pool = capi.POINTER(capi.ctex_transport_snapshot_pool)()
    ok(capi.ctex_transport_snapshot_pool_create(2 * TILE_BYTES, byref(pool)))
    session = capi.POINTER(capi.ctex_paint_preview_session)()
    ok(capi.ctex_paint_preview_session_create(
        document, text(texture_set.encode()), text(SEMANTIC), byref(session)))
    snapshot, query = preview_snapshot(pool, session)
    version, versions = capi.ctex_transport_tile_version(), capi.c_size_t()
    ok(capi.ctex_transport_snapshot_get_tile_versions(snapshot, byref(version), 1, byref(versions)))
    assert versions.value == 1 and version.residency == capi.CTEX_TRANSPORT_TILE_CPU
    selection, layout = negotiated_layout(snapshot, version)

    converted = (capi.c_uint16 * TILE_BYTES)()
    destination = capi.ctex_transport_tile_readback_destination(
        capi.CTEX_TRANSPORT_TILE_READBACK_DESTINATION_CURRENT_SIZE, version, layout,
        ctypes.cast(converted, capi.c_void_p), ctypes.sizeof(converted))
    ok(capi.ctex_transport_snapshot_read_tiles(snapshot, byref(selection), byref(destination), 1))
    offset = ((2 * TILE) + 1) * 3
    assert [converted[offset + index] for index in range(3)] == [257, 514, 771]
    memory = sized(capi.ctex_transport_snapshot_memory_report,
                   capi.CTEX_TRANSPORT_SNAPSHOT_MEMORY_REPORT_CURRENT_SIZE)
    ok(capi.ctex_transport_snapshot_pool_get_memory_report(pool, byref(memory)))
    assert memory.budget_bytes == 2 * TILE_BYTES and memory.pinned_bytes == TILE_BYTES
    assert memory.active_snapshots == 1
    native = sized(capi.ctex_transport_tile_memory_layout,
                   capi.CTEX_TRANSPORT_TILE_MEMORY_LAYOUT_CURRENT_SIZE)
    ok(capi.ctex_transport_snapshot_get_tile_memory_layout(snapshot, version, None, byref(native)))
    direct = (capi.c_ubyte * TILE_BYTES)()
    readback = capi.POINTER(capi.ctex_transport_readback)()
    local = capi.ctex_transport_tile_readback_destination(
        capi.CTEX_TRANSPORT_TILE_READBACK_DESTINATION_CURRENT_SIZE, version, native,
        ctypes.cast(direct, capi.c_void_p), TILE_BYTES)
    ok(capi.ctex_transport_snapshot_begin_readback(snapshot, None, byref(local), 1, byref(readback)))
    info = sized(capi.ctex_transport_readback_info,
                 capi.CTEX_TRANSPORT_READBACK_INFO_CURRENT_SIZE)
    ok(capi.ctex_transport_readback_get_info(readback, byref(info), None, 0))
    assert info.status == capi.CTEX_TRANSPORT_READBACK_COMPLETE
    assert info.output_readable == 1 and info.tile_count == 1
    assert [direct[offset + index] for index in range(3)] == list(PIXEL)
    # Two host-owned readbacks never publish a byte: the 0xB6 fill below is the host's own.
    device_version = capi.ctex_transport_tile_version(
        version.x, version.y, version.revision, version.generation,
        capi.CTEX_TRANSPORT_TILE_HOST_DEVICE)
    outcomes = {}
    for name, finish in (("cancelled", capi.ctex_transport_readback_cancel),
                         ("failed", lambda pending: capi.ctex_transport_readback_fail(
                             pending, text(b"device lost")))):
        output = (capi.c_ubyte * TILE_BYTES)()
        capi.memset(output, 0xB6, TILE_BYTES)
        pending = capi.POINTER(capi.ctex_transport_readback)()
        destination = capi.ctex_transport_tile_readback_destination(
            capi.CTEX_TRANSPORT_TILE_READBACK_DESTINATION_CURRENT_SIZE, device_version,
            native, ctypes.cast(output, capi.c_void_p), TILE_BYTES)
        ok(capi.ctex_transport_snapshot_begin_host_readback(
            snapshot, None, byref(destination), 1, byref(pending)))
        ok(capi.ctex_transport_readback_get_info(pending, byref(info), None, 0))
        assert info.status == capi.CTEX_TRANSPORT_READBACK_PENDING and info.output_readable == 0
        ok(finish(pending))
        info.size = capi.CTEX_TRANSPORT_READBACK_INFO_CURRENT_SIZE
        read = capi.ctex_transport_readback_get_info
        ok(read(pending, byref(info), None, 0))
        detail = buffer(info.required_detail_size)
        ok(read(pending, byref(info), detail, len(detail)))
        assert info.output_readable == 0 and output[0] == 0xB6
        outcomes[name] = {"status": info.status, "detail": detail.value.decode()}
        capi.ctex_transport_readback_destroy(pending)
    assert outcomes["cancelled"]["status"] == capi.CTEX_TRANSPORT_READBACK_CANCELLED
    assert outcomes["failed"]["status"] == capi.CTEX_TRANSPORT_READBACK_FAILED
    assert outcomes["failed"]["detail"] == "device lost"
    capi.ctex_transport_readback_destroy(readback)
    capi.ctex_transport_snapshot_destroy(snapshot)
    capi.ctex_paint_preview_session_destroy(session)
    capi.ctex_transport_snapshot_pool_destroy(pool)
    # Resetting the published history forces every host back to a full resynchronisation.
    first, second = capi.ctex_transport_revision_cursor(), capi.ctex_transport_revision_cursor()
    reset = capi.ctex_texture_set_reset_channel_revision_history
    ok(reset(document, text(texture_set.encode()), text(SEMANTIC), byref(first)))
    ok(reset(document, text(texture_set.encode()), text(SEMANTIC), byref(second)))
    assert second.epoch == first.epoch + 1 and second.revision == 0
    return {"changed_tiles": query.changed_tile_count, "pinned_bytes": memory.pinned_bytes,
            "converted_row_pitch": layout.row_pitch_bytes, "readback_outcomes": outcomes,
            "reset_epoch": second.epoch}


def spill_cold_tiles(document: object, owner: object, texture_set: object) -> dict[str, object]:
    stored, counters = {}, {"stores": 0, "loads": 0, "failing": 1}

    def store(key, data, count, user_data):
        counters["stores"] += 1
        if counters["failing"]:
            return 0
        stored[(key.tile_x, key.tile_y, key.generation)] = bytes(
            capi.cast(data, capi.POINTER(capi.c_ubyte))[:count])
        return 1

    def load(key, data, count, user_data):
        counters["loads"] += 1
        payload = stored.get((key.tile_x, key.tile_y, key.generation))
        if payload is None or len(payload) != count:
            return 0
        capi.memmove(data, payload, count)
        return 1

    def discard(key, user_data):
        stored.pop((key.tile_x, key.tile_y, key.generation), None)

    def release(namespace_identity, user_data):
        stored.clear()

    held = (capi.ctex_tile_backing_store_callback(store), capi.ctex_tile_backing_load_callback(load),
            capi.ctex_tile_backing_discard_callback(discard),
            capi.ctex_tile_backing_release_callback(release))
    RETAINED.extend(held)
    backing = capi.ctex_tile_backing_store_descriptor(
        capi.CTEX_TILE_BACKING_STORE_DESCRIPTOR_CURRENT_SIZE, *held, None)
    name = text(texture_set.identifier.encode())
    report_size = capi.CTEX_TILE_EVICTION_REPORT_CURRENT_SIZE
    report = sized(capi.ctex_tile_eviction_report, report_size)
    evict = capi.ctex_texture_set_evict_channel_tile
    ok(capi.ctex_texture_set_set_channel_backing_store(
        document, name, text(SEMANTIC), 0, byref(backing)))
    # A backing store that refuses the write keeps the authored pixels resident.
    ok(evict(document, name, text(SEMANTIC), 0, 0, 0, byref(report)))
    assert report.status == capi.CTEX_TILE_BACKING_STORE_FAILED
    assert counters["stores"] == 1 and report.resident_bytes_released == 0
    resident = report.resident_pixel_bytes
    assert resident >= TILE_BYTES
    counters["failing"] = 0
    report.size = report_size
    ok(evict(document, name, text(SEMANTIC), 0, 0, 0, byref(report)))
    assert report.status == capi.CTEX_TILE_EVICTED
    assert report.resident_bytes_released == TILE_BYTES == report.backing_bytes_written
    assert report.resident_pixel_bytes == resident - TILE_BYTES
    assert report.backed_pixel_bytes == TILE_BYTES and len(stored) == 1
    reloaded = owner.read_channel(texture_set, SEMANTIC.decode())
    assert tuple(reloaded[6, 5]) == COMMITTED and counters["loads"] == 1
    return {"evicted_bytes": report.backing_bytes_written, "loads": counters["loads"],
            "resident_after_eviction": report.resident_pixel_bytes}


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--executor", default="cpu")
    arguments = parser.parse_args()

    residency = account_residency()
    with cybertexel.Document() as document:
        texture_set = document.create_texture_set(
            "Body", partition_key="body", width=EXTENT, height=EXTENT)
        document.set_channel_enabled(texture_set, SEMANTIC.decode(), bit_depth=8)
        document.write_channel_pixel(texture_set, SEMANTIC.decode(), 5, 6, bytes(COMMITTED))
        raw = ctypes.cast(document._require_open(), capi.POINTER(capi.ctex_document))
        summary = {
            "capabilities": list(CAPABILITIES),
            "residency": residency,
            "transport": upload_changed_tiles(raw, texture_set.identifier),
            "tile_spill": spill_cold_tiles(raw, document, texture_set),
        }

    arguments.output.mkdir(parents=True, exist_ok=True)
    (arguments.output / "residency_and_upload.json").write_text(
        json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
