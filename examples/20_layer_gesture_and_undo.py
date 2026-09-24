#!/usr/bin/env python3
"""Composite a masked layer group, then edit it as one undoable gesture.

Capabilities: texture-document.
The example builds a masked group, asks what participates in a channel and
composites it from caller-owned rasters and from an owned snapshot. A bare
tile-history capture retains nothing for a layer-stack edit, while a transaction
mixing pixel writes with stack edits commits, undoes and redoes as one step.
"""

from __future__ import annotations

import argparse
import ctypes
import json
from pathlib import Path

import cybertexel

CAPABILITIES = ("texture-document",)

CAPI = cybertexel.capi
OK = CAPI.CTEX_RESULT_SUCCESS
BYREF = CAPI.byref  # the generated declarations take pointers, not Python values
TEXT = CAPI.String  # every const char* argument is a NUL-terminated byte string
NONE = CAPI.String(ctypes.POINTER(ctypes.c_char)())  # an omitted const char*

BASE = b"pbr.base_color"
CHANNEL, PAINT, GROUP, MASK, FILL = (TEXT(v) for v in (BASE, b"paint", b"group", b"mask", b"fill"))
SIDE, PIXELS = 4, 16
TILE_BYTES = 64 * 64 * 3  # history is charged per 64x64 storage tile, not per texel
PAINT_COLOUR, FILL_COLOUR, DEFAULT_TEXEL = (0.8, 0.4, 0.2, 1.0), (0.1, 0.2, 0.3, 1.0), (128,) * 3
GESTURE_ROW = (90, 40, 20, 91, 40, 20, 92, 40, 20)  # the three texels the gesture writes

def expect(result: int, wanted: int = OK) -> None:
    assert result == wanted, f"expected C ABI result {wanted}, got {result}"

def sized(structure: type) -> object:
    value = structure()  # every info struct declares the layout its caller compiled against
    value.size = ctypes.sizeof(structure)  # the header's CTEX_*_CURRENT_SIZE
    return value

def entry(identifier: bytes, kind: int, **fields: object) -> object:
    value = CAPI.ctex_layer_entry_descriptor(
        ctypes.sizeof(CAPI.ctex_layer_entry_descriptor), TEXT(identifier), TEXT(identifier),
        kind, NONE, NONE, NONE, 1, 1.0, TEXT(b"normal"), None, 0)
    for name, field in fields.items():
        setattr(value, name, field)
    return value

def channel(opacity: float) -> object:
    return CAPI.ctex_layer_channel_descriptor(
        ctypes.sizeof(CAPI.ctex_layer_channel_descriptor), CHANNEL, 1, opacity)


def creation(target: object, content: object = None, count: int = 0) -> object:
    return CAPI.ctex_layer_operation_descriptor(
        ctypes.sizeof(CAPI.ctex_layer_operation_descriptor), CAPI.CTEX_LAYER_OPERATION_CREATE,
        NONE, NONE, NONE, NONE, NONE, ctypes.pointer(target), content, count, None, 0,
        CAPI.CTEX_LAYER_SOURCE_DELETION_REFUSE, CAPI.CTEX_LAYER_ENTRY_PAINT, None, 0,
        1 << 20, 1.0e-6)


def raster(identifier: bytes, colour: tuple[float, ...], coverage: float) -> object:
    pixels = (CAPI.ctex_vec4f * PIXELS)(*(CAPI.ctex_vec4f(*colour),) * PIXELS)
    return CAPI.ctex_layer_composite_raster_descriptor(
        ctypes.sizeof(CAPI.ctex_layer_composite_raster_descriptor), TEXT(identifier), CHANNEL,
        SIDE, SIDE, pixels, PIXELS, (CAPI.c_float * PIXELS)(*(coverage,) * PIXELS), PIXELS)


def inspect(doc: object, sid: object) -> dict:
    required = CAPI.c_size_t()
    expect(CAPI.ctex_texture_set_layer_inspect(doc, sid, None, 0, BYREF(required)))
    report = CAPI.create_string_buffer(required.value)
    expect(CAPI.ctex_texture_set_layer_inspect(doc, sid, report, len(report), BYREF(required)))
    return json.loads(report.value.decode("utf-8"))


def find(stack: dict, identifier: str) -> dict | None:
    return next((item for item in stack["entries"] if item["id"] == identifier), None)


def create_panel(doc: object) -> object:
    descriptor = CAPI.ctex_texture_set_descriptor(
        ctypes.sizeof(CAPI.ctex_texture_set_descriptor), TEXT(b"Panel"),
        CAPI.CTEX_PARTITION_SOURCE_MATERIAL, TEXT(b"panel"), TEXT(b"uv0"), SIDE, SIDE, 8, 0)
    expect(CAPI.ctex_document_create_texture_set(doc, BYREF(descriptor)))
    get, required, count = CAPI.ctex_document_get_texture_set_ids, CAPI.c_size_t(), CAPI.c_size_t()
    expect(get(doc, None, 0, BYREF(required), BYREF(count)))
    buffer = CAPI.create_string_buffer(required.value)
    expect(get(doc, buffer, len(buffer), BYREF(required), BYREF(count)))
    assert count.value == 1 and buffer.value == b"material/5:panel/uv/3:uv0", buffer.value
    sid = TEXT(buffer.value)
    expect(CAPI.ctex_texture_set_set_channel_enabled(doc, sid, CHANNEL, 1, 0))
    entries = (CAPI.ctex_layer_entry_descriptor * 3)(
        entry(b"group", CAPI.CTEX_LAYER_ENTRY_GROUP, opacity=0.5),
        entry(b"paint", CAPI.CTEX_LAYER_ENTRY_PAINT, parent_identifier=GROUP,
              channels=ctypes.pointer(channel(0.5)), channel_count=1),
        entry(b"mask", CAPI.CTEX_LAYER_ENTRY_MASK, target_identifier=GROUP))
    expect(CAPI.ctex_texture_set_layer_append(doc, sid, entries, 3))
    stack = inspect(doc, sid)
    assert [item["id"] for item in stack["entries"]] == ["group", "paint", "mask"]
    assert find(stack, "paint")["parent"] == "group"
    return sid


def applicable_masks(doc: object, sid: object, names: object = None) -> tuple[int, int]:
    """A null identifier buffer only reports the size the mask identifiers need."""
    required, count = CAPI.c_size_t(), CAPI.c_size_t()
    expect(CAPI.ctex_texture_set_layer_get_applicable_masks(
        doc, sid, PAINT, names, len(names) if names else 0, BYREF(required), BYREF(count)))
    return count.value, required.value


def audit_stack(doc: object, sid: object) -> dict[str, float]:
    """Effective opacity flattens the channel, its layer, its group and its masks."""
    names = CAPI.create_string_buffer(applicable_masks(doc, sid)[1])
    assert applicable_masks(doc, sid, names) == (1, len(b"mask\0"))
    assert names.value == b"mask"
    samples = (CAPI.ctex_layer_mask_sample * 1)(CAPI.ctex_layer_mask_sample(
        ctypes.sizeof(CAPI.ctex_layer_mask_sample), MASK, 0.5))
    info = sized(CAPI.ctex_layer_participation_info)
    share = CAPI.ctex_texture_set_layer_get_participation
    expect(share(doc, sid, PAINT, CHANNEL, samples, 1, BYREF(info), names, len(names)))
    # 0.5 channel x 1.0 layer x 0.5 group x 0.5 mask sample.
    assert (info.participates, info.mask_count, info.required_mask_id_size) == (1, 1, len(names))
    assert abs(info.effective_opacity - 0.125) < 1.0e-12
    expect(share(doc, sid, PAINT, CHANNEL, None, 0, BYREF(info), None, 0),
           CAPI.CTEX_RESULT_INVALID_ARGUMENT)
    colour, blend = CAPI.ctex_vec4f(), CAPI.ctex_texture_set_layer_evaluate_blend
    expect(blend(doc, sid, PAINT, CAPI.ctex_vec4f(0.25, 0.5, 0.75, 1.0),
                 CAPI.ctex_vec4f(*PAINT_COLOUR), 0.5, BYREF(colour)))
    # "normal" at factor 0.5 lands halfway between the base and the layer colour.
    assert [round(value, 6) for value in (colour.x, colour.y, colour.z, colour.w)] == [
        0.525, 0.45, 0.475, 1.0]
    layout = CAPI.ctex_texture_set_layer_set_layout
    expect(CAPI.ctex_texture_set_layer_set_channel(doc, sid, PAINT, BYREF(channel(1.0))))
    expect(share(doc, sid, PAINT, CHANNEL, samples, 1, BYREF(info), names, len(names)))
    assert abs(info.effective_opacity - 0.25) < 1.0e-12
    # Masks apply through the group, so detaching the layer drops them.
    expect(layout(doc, sid, GROUP, GROUP, NONE), CAPI.CTEX_RESULT_INVALID_ARGUMENT)
    assert b"must precede its child" in CAPI.ctex_get_last_diagnostic()
    expect(layout(doc, sid, PAINT, NONE, NONE))
    assert applicable_masks(doc, sid, names) == (0, 0)
    expect(layout(doc, sid, PAINT, GROUP, NONE))
    assert applicable_masks(doc, sid, names) == (1, len(b"mask\0"))
    before, record = inspect(doc, sid), CAPI.ctex_texture_set_layer_record_paint
    expect(record(doc, sid, GROUP), CAPI.CTEX_RESULT_INVALID_ARGUMENT)
    assert b"must be a paint-layer entry" in CAPI.ctex_get_last_diagnostic()
    assert inspect(doc, sid) == before, "a refused paint record changed the stack"
    expect(record(doc, sid, PAINT))
    after = inspect(doc, sid)
    assert find(after, "paint")["content_revision"] == find(before, "paint")["content_revision"] + 1
    assert after["revision"] > before["revision"]
    return {"half_channel": 0.125, "full_channel": info.effective_opacity}


def composite(doc: object, sid: object, content: object, masks: object) -> tuple:
    """The CPU reference flattens the stack from caller-owned rasters, twice alike."""
    info, flatten = sized(CAPI.ctex_layer_composite_info), CAPI.ctex_texture_set_layer_composite_cpu
    expect(flatten(doc, sid, SIDE, SIDE, content, 1, masks, 1, BYREF(info), None, 0, None, 0,
                   None, 0))
    assert (info.width, info.height, info.channel_count) == (SIDE, SIDE, 1)
    assert (info.required_pixel_count, info.required_semantic_id_size) == (PIXELS, len(BASE) + 1)
    channels = (CAPI.ctex_layer_composite_channel_info * 1)()
    semantic = CAPI.create_string_buffer(info.required_semantic_id_size)
    first, again = (CAPI.ctex_vec4f * PIXELS)(), (CAPI.ctex_vec4f * PIXELS)()
    expect(flatten(doc, sid, SIDE, SIDE, content, 1, masks, 1, BYREF(info), channels, 1, semantic,
                   len(semantic), first, PIXELS))
    assert semantic.value == BASE
    assert (channels[0].component_count, channels[0].pixel_count) == (3, PIXELS)
    # 0.75 coverage x 0.5 group opacity x 0.5 mask, over the 0.5 channel default.
    flattened = (first[0].x, first[0].y, first[0].z)
    wanted = [0.5 + 0.1875 * (value - 0.5) for value in PAINT_COLOUR[:3]]
    assert max(abs(a - b) for a, b in zip(flattened, wanted)) < 1.0e-6, flattened
    expect(flatten(doc, sid, SIDE, SIDE, content, 1, masks, 1, BYREF(info), channels, 1, semantic,
                   len(semantic), again, PIXELS))
    assert bytes(again) == bytes(first), "the CPU composite is not bit-identical"
    expect(flatten(doc, sid, SIDE, SIDE, content, 0, masks, 1, BYREF(info), channels, 1, semantic,
                   len(semantic), again, PIXELS), CAPI.CTEX_RESULT_INVALID_ARGUMENT)
    assert b"content is missing" in CAPI.ctex_get_last_diagnostic()
    return first, flattened


def owned_snapshot(doc: object, sid: object, content: object, masks: object,
                   expected: object) -> object:
    """A snapshot copies the caller's rasters, so the same pixels composite later."""
    snapshot = ctypes.POINTER(CAPI.ctex_layer_snapshot)()
    expect(CAPI.ctex_layer_snapshot_create(SIDE, SIDE, content, 1, masks, 1, BYREF(snapshot)))
    read, info = CAPI.ctex_layer_snapshot_read, sized(CAPI.ctex_layer_snapshot_info)
    expect(read(snapshot, BYREF(info), None, 0, None, 0, None, 0, None, 0, None, 0, None, 0))
    assert (info.width, info.height, info.content_count, info.mask_count) == (SIDE, SIDE, 1, 1)
    assert info.required_pixel_count == info.required_mask_value_count == PIXELS
    assert info.required_coverage_count == PIXELS
    contents = (CAPI.ctex_layer_snapshot_content_info * 1)()
    mask_infos = (CAPI.ctex_layer_snapshot_mask_info * 1)()
    strings = CAPI.create_string_buffer(info.required_string_size)
    pixels, out = (CAPI.ctex_vec4f * PIXELS)(), (CAPI.ctex_vec4f * PIXELS)()
    coverage, values = (CAPI.c_float * PIXELS)(), (CAPI.c_double * PIXELS)()
    expect(read(snapshot, BYREF(info), contents, 1, mask_infos, 1, strings, len(strings), pixels,
                PIXELS, coverage, PIXELS, values, PIXELS))
    assert strings.raw[: info.required_string_size] == b"paint\0pbr.base_color\0mask\0"
    assert (contents[0].width, contents[0].pixel_count) == (SIDE, PIXELS)
    assert mask_infos[0].value_count == PIXELS
    assert abs(pixels[0].x - PAINT_COLOUR[0]) < 1.0e-6
    assert coverage[0] == 0.75 and values[0] == 0.5
    truncated = (CAPI.ctex_layer_snapshot_content_info * 1)()
    truncated[0].width = 77
    expect(read(snapshot, BYREF(info), truncated, 1, None, 0, strings, 1, None, 0, None, 0,
                None, 0), CAPI.CTEX_RESULT_BUFFER_TOO_SMALL)
    assert truncated[0].width == 77, "a short read published partial snapshot data"
    report = sized(CAPI.ctex_layer_composite_info)
    channels = (CAPI.ctex_layer_composite_channel_info * 1)()
    expect(CAPI.ctex_texture_set_layer_composite_snapshot_cpu(
        doc, sid, snapshot, BYREF(report), None, 0, None, 0, None, 0))
    semantic = CAPI.create_string_buffer(report.required_semantic_id_size)
    expect(CAPI.ctex_texture_set_layer_composite_snapshot_cpu(
        doc, sid, snapshot, BYREF(report), channels, 1, semantic, len(semantic), out, PIXELS))
    assert report.channel_count == 1 and semantic.value == BASE
    assert bytes(out) == bytes(expected), "the snapshot composite differs from the raster one"
    return snapshot


def create_fill(doc: object, sid: object, snapshot: object, operation: object) -> str:
    info = sized(CAPI.ctex_layer_operation_info)
    apply = CAPI.ctex_texture_set_apply_layer_operation
    expect(apply(doc, sid, snapshot, BYREF(operation), BYREF(info), None, 0))
    assert (info.affected_count, info.required_affected_id_size) == (1, len(b"fill\0"))
    assert find(inspect(doc, sid), "fill") is None, "the sizing call published the layer"
    affected = CAPI.create_string_buffer(info.required_affected_id_size)
    expect(apply(doc, sid, snapshot, BYREF(operation), BYREF(info), affected, len(affected)))
    assert affected.value == b"fill"
    assert find(inspect(doc, sid), "fill")["kind"] == "fill"
    read, snap = CAPI.ctex_layer_snapshot_read, sized(CAPI.ctex_layer_snapshot_info)
    expect(read(snapshot, BYREF(snap), None, 0, None, 0, None, 0, None, 0, None, 0, None, 0))
    pixels = (CAPI.ctex_vec4f * snap.required_pixel_count)()
    expect(read(snapshot, BYREF(snap), None, 0, None, 0, None, 0, pixels, len(pixels), None, 0,
                None, 0))
    # The create operation adopted the caller's fill raster behind the paint one.
    assert (snap.content_count, snap.required_pixel_count) == (2, 2 * PIXELS)
    filled = [round(value, 6) for value in (pixels[PIXELS].x, pixels[PIXELS].y, pixels[PIXELS].z)]
    assert filled == [0.1, 0.2, 0.3], filled  # the colour main handed the fill raster
    return affected.value.decode()


def written_texels(doc: object, sid: object) -> tuple[int, ...]:
    """The three texels the gesture writes, flattened as the session stores them."""
    session = ctypes.POINTER(CAPI.ctex_paint_preview_session)()
    expect(CAPI.ctex_paint_preview_session_create(doc, sid, CHANNEL, BYREF(session)))
    stored, required = (CAPI.c_ubyte * (PIXELS * 3))(), CAPI.c_size_t()
    expect(CAPI.ctex_paint_preview_session_get_pixels(session, stored, len(stored),
                                                      BYREF(required)))
    CAPI.ctex_paint_preview_session_destroy(session)
    assert required.value == PIXELS * 3
    return tuple(stored[: len(GESTURE_ROW)])


def bare_capture(doc: object, sid: object, snapshot: object, operation: object,
                 target: object) -> str:
    expect(CAPI.ctex_texture_set_configure_tile_history(doc, sid, TILE_BYTES - 1))
    capture = ctypes.POINTER(CAPI.ctex_tile_history_capture)()
    expect(CAPI.ctex_texture_set_begin_tile_history(doc, sid, TEXT(b"too-large"), target, 1,
                                                    BYREF(capture)), CAPI.CTEX_RESULT_OVER_BUDGET)
    assert not capture, "an over-budget capture was admitted"
    assert b"the declared ceiling is 12287" in CAPI.ctex_get_last_diagnostic()
    expect(CAPI.ctex_texture_set_configure_tile_history(doc, sid, 1 << 20))
    expect(CAPI.ctex_texture_set_begin_tile_history(doc, sid, TEXT(b"create-fill"), target, 1,
                                                    BYREF(capture)))
    created = create_fill(doc, sid, snapshot, operation)
    commit = sized(CAPI.ctex_tile_history_commit_info)
    expect(CAPI.ctex_tile_history_capture_commit(capture, BYREF(commit)))
    # The layer operation published itself, and no tile changed: nothing is retained.
    assert (commit.committed, commit.tile_count) == (0, 0)
    assert (commit.retained_bytes, commit.layer_stack_changed) == (0, 0)
    CAPI.ctex_tile_history_capture_destroy(capture)
    restore = sized(CAPI.ctex_tile_history_restore_info)
    expect(CAPI.ctex_texture_set_undo_tiles(doc, sid, BYREF(restore)), CAPI.CTEX_RESULT_NO_UNDO)
    return created


def gesture(doc: object, sid: object, snapshot: object, target: object,
            graph: object) -> dict[str, int]:
    """Pixel writes and stack edits commit, undo and redo as one history step."""
    transaction = ctypes.POINTER(CAPI.ctex_texture_set_transaction)()
    expect(CAPI.ctex_texture_set_begin_transaction(doc, sid, TEXT(b"gesture"), target, 1,
                                                   snapshot, BYREF(transaction)))
    for x in range(3):
        texel = (CAPI.c_ubyte * 3)(*GESTURE_ROW[x * 3 : x * 3 + 3])
        expect(CAPI.ctex_texture_set_transaction_write_pixel(transaction, CHANNEL, x, 0, texel, 3))
    region = CAPI.ctex_channel_region_descriptor(
        ctypes.sizeof(CAPI.ctex_channel_region_descriptor), 0, 0, 3, 1, 9)
    row = (CAPI.c_ubyte * 9)(*GESTURE_ROW)
    expect(CAPI.ctex_texture_set_transaction_write_region(
        transaction, CHANNEL, BYREF(region), row, len(row)))
    create = creation(entry(b"gesture-group", CAPI.CTEX_LAYER_ENTRY_GROUP))
    expect(CAPI.ctex_texture_set_transaction_apply_layer_operation(transaction, BYREF(create)))
    expect(CAPI.ctex_texture_set_transaction_set_layer_state(
        transaction, PAINT, TEXT(b"Renamed paint"), 1, 0.5, TEXT(b"multiply")))
    # "group" already precedes "fill", so the fill layer may move inside it here.
    expect(CAPI.ctex_texture_set_transaction_set_layer_layout(transaction, FILL, GROUP, NONE))
    quarter = channel(0.25)
    expect(CAPI.ctex_texture_set_transaction_set_layer_channel(transaction, PAINT, BYREF(quarter)))
    expect(CAPI.ctex_texture_set_transaction_set_fill_graph(transaction, FILL, graph, len(graph)))
    staged = inspect(doc, sid)
    assert find(staged, "gesture-group") is None, "the transaction published before commit"
    assert find(staged, "paint")["display_name"] == "paint"
    assert written_texels(doc, sid) == DEFAULT_TEXEL * 3
    commit = sized(CAPI.ctex_tile_history_commit_info)
    expect(CAPI.ctex_texture_set_transaction_commit(transaction, BYREF(commit)))
    assert (commit.committed, commit.tile_count, commit.layer_stack_changed) == (1, 1, 1)
    assert commit.retained_bytes == TILE_BYTES
    CAPI.ctex_texture_set_transaction_destroy(transaction)
    published = inspect(doc, sid)
    assert published["revision"] > staged["revision"]
    paint, fill = find(published, "paint"), find(published, "fill")
    assert (paint["display_name"], paint["opacity"], paint["blend_mode"],
            paint["channels"][0]["opacity"]) == ("Renamed paint", 0.5, "multiply", 0.25)
    assert find(published, "gesture-group")["kind"] == "group"
    # set_fill_graph replaced the fill content, so its revision moved once with the layout.
    assert (fill["content_revision"], fill["parent"]) == (
        find(staged, "fill")["content_revision"] + 1, "group")
    assert written_texels(doc, sid) == GESTURE_ROW == (90, 40, 20, 91, 40, 20, 92, 40, 20)
    restore = sized(CAPI.ctex_tile_history_restore_info)
    expect(CAPI.ctex_texture_set_undo_tiles(doc, sid, BYREF(restore)))
    assert (restore.tile_count, restore.layer_stack_exchanged) == (1, 1)
    assert restore.copied_pixel_bytes == 0, "undo copied instead of exchanging tile storage"
    undone = inspect(doc, sid)
    assert find(undone, "gesture-group") is None and find(undone, "paint")["blend_mode"] == "normal"
    assert written_texels(doc, sid) == DEFAULT_TEXEL * 3
    budget = sized(CAPI.ctex_tile_history_budget_report)
    expect(CAPI.ctex_texture_set_get_tile_history_budget(doc, sid, TILE_BYTES, BYREF(budget)))
    assert (budget.undo_steps, budget.redo_steps) == (0, 1)
    assert budget.retained_bytes == budget.proposed_step_bytes == TILE_BYTES
    assert budget.available_bytes == budget.budget_bytes - budget.retained_bytes
    expect(CAPI.ctex_texture_set_redo_tiles(doc, sid, BYREF(restore)))
    assert find(inspect(doc, sid), "paint")["display_name"] == "Renamed paint"
    assert written_texels(doc, sid) == GESTURE_ROW
    # A second transaction, cancelled instead of committed, publishes nothing at all.
    expect(CAPI.ctex_texture_set_begin_transaction(doc, sid, TEXT(b"cancel"), None, 0, snapshot,
                                                   BYREF(transaction)))
    discard = creation(entry(b"discarded", CAPI.CTEX_LAYER_ENTRY_GROUP))
    expect(CAPI.ctex_texture_set_transaction_apply_layer_operation(transaction, BYREF(discard)))
    CAPI.ctex_texture_set_transaction_cancel(transaction)
    CAPI.ctex_texture_set_transaction_destroy(transaction)
    assert find(inspect(doc, sid), "discarded") is None, "a cancelled edit was published"
    return {"retained_bytes": budget.retained_bytes, "tiles": commit.tile_count,
            "undo_steps_after_undo": budget.undo_steps}


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--executor", default="cpu")
    arguments = parser.parse_args()

    doc = ctypes.POINTER(CAPI.ctex_document)()
    expect(CAPI.ctex_document_create(BYREF(doc)))
    snapshot = ctypes.POINTER(CAPI.ctex_layer_snapshot)()
    try:
        sid = create_panel(doc)
        opacity = audit_stack(doc, sid)
        content = (CAPI.ctex_layer_composite_raster_descriptor * 1)(
            raster(b"paint", PAINT_COLOUR, 0.75))
        masks = (CAPI.ctex_layer_composite_mask_descriptor * 1)(
            CAPI.ctex_layer_composite_mask_descriptor(
                ctypes.sizeof(CAPI.ctex_layer_composite_mask_descriptor), MASK, SIDE, SIDE,
                (CAPI.c_double * PIXELS)(*(0.5,) * PIXELS), PIXELS))
        flattened, sample = composite(doc, sid, content, masks)
        snapshot = owned_snapshot(doc, sid, content, masks, flattened)

        fill_content = (CAPI.ctex_layer_composite_raster_descriptor * 1)(
            raster(b"fill", FILL_COLOUR, 1.0))
        create = creation(entry(b"fill", CAPI.CTEX_LAYER_ENTRY_FILL), fill_content, 1)
        target = (CAPI.ctex_tile_history_target_descriptor * 1)(
            CAPI.ctex_tile_history_target_descriptor(
                ctypes.sizeof(CAPI.ctex_tile_history_target_descriptor), CHANNEL, 0, 0))
        created = bare_capture(doc, sid, snapshot, create, target)
        info = sized(CAPI.ctex_material_graph_info)
        expect(CAPI.ctex_material_graph_create_default(BYREF(info), None, 0, None, 0))
        graph = (CAPI.c_ubyte * info.canonical_size)()
        expect(CAPI.ctex_material_graph_create_default(BYREF(info), graph, len(graph), None, 0))
        committed = gesture(doc, sid, snapshot, target, graph)
    finally:
        CAPI.ctex_layer_snapshot_destroy(snapshot)
        CAPI.ctex_document_destroy(doc)

    arguments.output.mkdir(parents=True, exist_ok=True)
    summary = {
        "capabilities": list(CAPABILITIES),
        "composited_texel": [round(float(value), 6) for value in sample],
        "created_layer": created,
        "effective_opacity": opacity,
        "gesture": committed,
    }
    (arguments.output / "summary.json").write_text(
        json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
