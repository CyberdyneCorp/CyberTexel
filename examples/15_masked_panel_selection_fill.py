#!/usr/bin/env python3
"""Decide what is paintable on a two-island panel, then fill, blend and erase it.

Capabilities: paint-engine, paint-tools
A trim pass on a hull panel: cache the surface map for one tile, read the
channels under the cursor, narrow the paintable area with colour-ID, screen,
polygon and stencil masks, intersect every mask class, then plan the tiles,
fill, blend, erase and dilate the result across the island gutter.
"""

from __future__ import annotations

import argparse
import ctypes
import json
from pathlib import Path

import cybertexel
import numpy as np


CAPABILITIES = ("paint-engine", "paint-tools")
SUCCESS = cybertexel.capi.CTEX_RESULT_SUCCESS
TOO_SMALL = cybertexel.capi.CTEX_RESULT_BUFFER_TOO_SMALL
WIDTH = HEIGHT = 4
TEXELS = WIDTH * HEIGHT
VIEWPORT = 100
POSITIONS = np.array(
    [[-1.0, -0.5, 0.0], [-0.2, -0.5, 0.0], [-0.2, 0.5, 0.0], [-1.0, 0.5, 0.0],
     [0.2, -0.5, 0.0], [1.0, -0.5, 0.0], [1.0, 0.5, 0.0], [0.2, 0.5, 0.0]], dtype=np.float32)
UV = np.array([[0.0, 0.0], [0.3, 0.0], [0.3, 1.0], [0.0, 1.0],
               [0.7, 0.0], [1.0, 0.0], [1.0, 1.0], [0.7, 1.0]], dtype=np.float32)
TRIANGLES = np.array([[0, 1, 2], [0, 2, 3], [4, 5, 6], [4, 6, 7]], dtype=np.uint32)
NORMALS = np.tile(np.array([[0.0, 0.0, 1.0]], dtype=np.float32), (8, 1))
ISLAND_COLOURS = ((0.9, 0.1, 0.05), (0.05, 0.8, 0.2))


def sized(name: str, **fields: object) -> object:
    """One descriptor or info record, stamped with its current ABI size."""
    value = getattr(cybertexel.capi, name)()
    value.size = getattr(cybertexel.capi, f"{name.upper()}_CURRENT_SIZE")
    for field, item in fields.items():
        setattr(value, field, item)
    return value


def doubles(values: object) -> object:
    return (cybertexel.capi.c_double * len(values))(*(float(value) for value in values))


def vec4s(rows: object) -> object:
    return (cybertexel.capi.ctex_vec4f * len(rows))(
        *(cybertexel.capi.ctex_vec4f(*row) for row in rows))


def rows_of(pixels: object) -> np.ndarray:
    return np.array([[pixel.x, pixel.y, pixel.z, pixel.w] for pixel in pixels])


def channel(semantic: bytes, components: int, pixels: object) -> object:
    return cybertexel.capi.ctex_paint_tool_channel_descriptor(
        cybertexel.capi.CTEX_PAINT_TOOL_CHANNEL_DESCRIPTOR_CURRENT_SIZE,
        cybertexel.capi.String(semantic), components, pixels, TEXELS)


def surface_map(capi: object, mesh: object) -> dict[str, object]:
    """Rasterize the tile's surface map once, then serve the copy from the cache."""
    cache = capi.POINTER(capi.ctex_paint_surface_map_cache)()
    assert capi.ctex_paint_surface_map_cache_create(capi.byref(cache)) == SUCCESS
    request = sized("ctex_paint_surface_map_request",
                    uv_set=capi.String(b"uv0"), width=WIDTH, height=HEIGHT)
    info = sized("ctex_paint_surface_map_info")
    lookup = (cache, mesh, capi.byref(request), capi.byref(info))
    assert capi.ctex_paint_surface_map_cache_lookup(*lookup, None) == SUCCESS
    assert info.cache_hit == 0 and info.required_texel_count == TEXELS
    texels, islands = (capi.ctex_paint_surface_texel * TEXELS)(), (capi.c_uint32 * TEXELS)()
    coverage, triangles = (capi.c_ubyte * TEXELS)(), (capi.c_uint32 * TEXELS)()
    set_id = capi.create_string_buffer(info.required_texture_set_id_size)
    uv_set = capi.create_string_buffer(info.required_uv_set_size)
    buffers = sized(
        "ctex_paint_surface_map_buffers",
        texture_set_id=capi.String(set_id), texture_set_id_size=len(set_id),
        uv_set=capi.String(uv_set), uv_set_size=len(uv_set),
        surface_texels=texels, surface_texel_capacity=TEXELS,
        coverage=coverage, coverage_capacity=TEXELS,
        triangle_identity=triangles, triangle_identity_capacity=TEXELS,
        uv_island_identity=islands, uv_island_identity_capacity=TEXELS)
    assert capi.ctex_paint_surface_map_cache_lookup(*lookup, capi.byref(buffers)) == SUCCESS
    assert info.cache_hit == 1, "the second lookup must be served from the cache"
    assert set_id.value == b"material/5:panel/uv/3:uv0" and uv_set.value == b"uv0"
    stats = sized("ctex_paint_surface_map_statistics")
    assert capi.ctex_paint_surface_map_cache_get_statistics(cache, capi.byref(stats)) == SUCCESS
    assert (stats.entries, stats.hits, stats.misses) == (1, 1, 1)
    covered = np.ctypeslib.as_array(coverage).astype(bool)
    island_ids = np.ctypeslib.as_array(islands).copy()
    triangle_ids = np.ctypeslib.as_array(triangles).copy()
    assert covered.sum() == 8, "the two UV islands leave a two-column gutter"
    assert np.array_equal(covered, triangle_ids != capi.CTEX_NO_SURFACE_TRIANGLE)
    assert len(distinct := sorted(set(island_ids[covered].tolist()))) == 2, "two UV islands"
    return {"cache": cache, "texels": texels, "coverage": coverage, "triangles": triangles,
            "islands": islands, "covered": covered, "island_ids": island_ids, "distinct": distinct,
            "triangle_ids": triangle_ids, "texture_set_id": set_id.value.decode(),
            "positions": np.array([[t.position.x, t.position.y, t.position.z] for t in texels])}


def rejected_coverage(capi: object, mesh: object, surface: dict) -> int:
    """Reject the stamp where the viewer only sees the panel's back face."""
    stamp = capi.ctex_resolved_stamp()
    stamp.position = capi.ctex_vec3d(-0.6, 0.0, 0.0)
    stamp.frame = capi.ctex_stroke_frame(
        capi.ctex_vec3d(1.0, 0.0, 0.0), capi.ctex_vec3d(0.0, 1.0, 0.0),
        capi.ctex_vec3d(0.0, 0.0, 1.0))
    stamp.radius, stamp.opacity, stamp.hardness, stamp.elongation, stamp.flow = 0.75, 1, 1, 1, 1
    stamp.tip_resource_identity = capi.String(b"builtin.circle")
    stroke = capi.ctex_resolved_stroke_descriptor(
        capi.CTEX_RESOLVED_STROKE_DESCRIPTOR_CURRENT_SIZE, 1,
        capi.CTEX_STROKE_TIP_DISCRETE_ALPHA, 1, capi.pointer(stamp), 1, None, 0)
    tile = capi.ctex_paint_tile_coverage_descriptor(
        capi.CTEX_PAINT_TILE_COVERAGE_DESCRIPTOR_CURRENT_SIZE,
        capi.String(b"uv0"), WIDTH, HEIGHT, capi.ctex_vec2d(0.0, 0.0))
    rejection = capi.ctex_paint_rejection_descriptor()
    assert capi.ctex_paint_rejection_init(capi.byref(rejection)) == SUCCESS
    assert (rejection.depth_bias, rejection.minimum_normal_dot) == (1.0e-4, 0.5)
    away = (capi.ctex_vec3d * TEXELS)(*(capi.ctex_vec3d(0.0, 0.0, -1.0) for _ in range(TEXELS)))
    rejection.depth_enabled, rejection.backface_enabled = 0, 1
    rejection.view_directions, rejection.view_direction_count = away, TEXELS
    info, values, count = (sized("ctex_paint_rejection_info"),
                           (capi.c_double * TEXELS)(), capi.c_size_t())
    evaluate = capi.ctex_paint_evaluate_rejected_coverage
    call = (mesh, capi.byref(tile), capi.byref(stroke), capi.byref(rejection), capi.byref(info))
    assert evaluate(*call, values, TEXELS, capi.byref(count)) == SUCCESS
    assert count.value == TEXELS and info.backface_rejected_texels > 0, "the view faces away"
    assert np.ctypeslib.as_array(values).max() == 0.0, "a back-facing view paints nothing"
    rejection.backface_enabled, rejection.view_direction_count = 0, 0
    assert evaluate(*call, values, TEXELS, capi.byref(count)) == SUCCESS
    accepted = np.ctypeslib.as_array(values).copy()
    assert info.backface_rejected_texels == 0 and accepted.max() > 0.0
    assert accepted[surface["island_ids"] == surface["distinct"][1]].max() == 0.0
    return int((accepted > 0.0).sum())


def pick_and_select(capi: object, surface: dict) -> dict[str, object]:
    """Read the channels under the cursor, then grow a colour-ID selection from them."""
    rank = {identity: position for position, identity in enumerate(surface["distinct"])}
    colours = np.array([[*ISLAND_COLOURS[rank[identity]], 1.0] if identity in rank
                        else [0.0, 0.0, 0.0, 1.0] for identity in surface["island_ids"].tolist()])
    pixels = vec4s(colours.tolist())
    enabled = (capi.ctex_paint_tool_channel_descriptor * 1)(
        channel(b"pbr.base_color", 3, pixels))
    view = sized("ctex_paint_picker_texture_view_descriptor",
                 texture_set_id=capi.String(surface["texture_set_id"].encode()),
                 width=WIDTH, height=HEIGHT, enabled_channels=enabled, enabled_channel_count=1)
    picker = sized("ctex_paint_picker_descriptor",
                   hit_texture_set_id=capi.String(surface["texture_set_id"].encode()),
                   texture_views=capi.pointer(view), texture_view_count=1)
    picker.hit.has_hit, picker.hit.uv = 1, capi.ctex_vec2f(0.125, 0.375)
    info = sized("ctex_paint_picker_info")
    call = (capi.byref(picker), capi.byref(info))
    assert capi.ctex_paint_pick_enabled_channels(*call, None, 0, None, 0) == SUCCESS
    assert info.required_channel_count == 1 and info.has_material_identity == 0
    picked = (capi.ctex_paint_picker_channel_value * 1)()
    text = capi.create_string_buffer(info.required_string_size)
    assert capi.ctex_paint_pick_enabled_channels(*call, picked, 0, text, len(text)) == TOO_SMALL
    assert picked[0].component_count == 0, "an undersized pick must write nothing"
    assert capi.ctex_paint_pick_enabled_channels(*call, picked, 1, text, len(text)) == SUCCESS
    u, v = picker.hit.uv.x, picker.hit.uv.y
    texel = (HEIGHT - 1 - int(v * HEIGHT)) * WIDTH + int(u * WIDTH)
    assert int(info.texel) == texel == 8, "the pick flips v: (0.125, 0.375) is the third row"
    assert picked[0].component_count == 3 and abs(picked[0].value.x - colours[texel][0]) < 1.0e-6
    start = int(picked[0].semantic_id_offset)
    assert text.raw[start : start + picked[0].semantic_id_size - 1] == b"pbr.base_color"
    colour = sized("ctex_paint_colour_id_descriptor", width=WIDTH, height=HEIGHT, pixels=pixels,
                   pixel_count=TEXELS, picked_colour=picked[0].value, tolerance=0.01)
    colour_info = sized("ctex_paint_colour_id_info")
    values = (capi.c_double * TEXELS)()
    assert capi.ctex_paint_select_colour_id(
        capi.byref(colour), capi.byref(colour_info), values, TEXELS) == SUCCESS
    expected = (surface["island_ids"] == surface["island_ids"][texel]).astype(float)
    assert colour_info.status == capi.CTEX_PAINT_COLOUR_ID_SELECTION_MATCHED
    assert np.array_equal(np.ctypeslib.as_array(values), expected)
    return {"texel": texel, "pixels": pixels, "colour": expected}


def screen_and_polygon(capi: object, index: object, surface: dict, revision: int,
                       picked: dict) -> dict[str, np.ndarray]:
    """Narrow the region with a viewport rectangle and with polygon growth."""
    descriptor = sized(
        "ctex_paint_selection_surface_descriptor", width=WIDTH, height=HEIGHT,
        texture_set_id=capi.String(surface["texture_set_id"].encode()),
        uv_set=capi.String(b"uv0"), mesh_revision=revision,
        surface_texels=surface["texels"], surface_texel_count=TEXELS,
        coverage=surface["coverage"], coverage_count=TEXELS,
        triangle_identity=surface["triangles"], triangle_identity_count=TEXELS,
        uv_island_identity=surface["islands"], uv_island_identity_count=TEXELS)
    screen = sized("ctex_paint_screen_selection_descriptor",
                   kind=capi.CTEX_PAINT_SELECTION_SCREEN_RECTANGLE,
                   surface=capi.pointer(descriptor),
                   minimum=capi.ctex_vec2f(5.0, 40.0), maximum=capi.ctex_vec2f(30.0, 75.0))
    screen.view.size = capi.CTEX_PICK_SCREEN_VIEW_DESCRIPTOR_CURRENT_SIZE
    screen.view.viewport_width = screen.view.viewport_height = VIEWPORT
    for axis in range(4):
        screen.view.view[axis * 5] = screen.view.projection[axis * 5] = 1.0
    info = sized("ctex_paint_selection_info")
    call = (index, capi.byref(screen), capi.byref(info))
    assert capi.ctex_paint_select_screen(*call, None) == SUCCESS and info.visited_nodes > 0
    assert (info.required_value_count, info.selected_triangle_count) == (TEXELS, 2)
    values = (capi.c_double * info.required_value_count)()
    ids = (capi.c_uint32 * info.selected_triangle_count)()
    outputs = sized("ctex_paint_selection_outputs", values=values, value_capacity=len(values),
                    selected_triangle_ids=ids, selected_triangle_capacity=len(ids))
    assert capi.ctex_paint_select_screen(*call, capi.byref(outputs)) == SUCCESS
    screen_values = np.ctypeslib.as_array(values).copy()
    screen_x = (surface["positions"][:, 0] + 1.0) * 0.5 * VIEWPORT
    screen_y = (1.0 - surface["positions"][:, 1]) * 0.5 * VIEWPORT
    inside = surface["covered"] & (screen_x >= 5.0) & (screen_x <= 30.0)
    inside &= (screen_y >= 40.0) & (screen_y <= 75.0)
    assert np.array_equal(screen_values, inside.astype(float))
    assert info.selected_texel_count == int(inside.sum()) == 3
    polygon = sized("ctex_paint_polygon_selection_descriptor",
                    kind=capi.CTEX_PAINT_SELECTION_POLYGON_UV_ISLAND,
                    surface=capi.pointer(descriptor), picked_texel=picked["texel"],
                    maximum_angle_degrees=45.0)
    grow = (capi.byref(polygon), capi.byref(info), capi.byref(outputs))
    assert capi.ctex_paint_select_polygon(*grow) == SUCCESS
    island_values = np.ctypeslib.as_array(values).copy()
    assert np.array_equal(island_values, picked["colour"])
    polygon.kind = capi.CTEX_PAINT_SELECTION_POLYGON_TRIANGLE
    assert capi.ctex_paint_select_polygon(*grow) == SUCCESS
    triangle_values = np.ctypeslib.as_array(values).copy()
    picked_triangle = surface["triangle_ids"][picked["texel"]]
    assert np.array_equal(triangle_values,
                          (surface["triangle_ids"] == picked_triangle).astype(float))
    assert info.selected_triangle_count == 1 and ids[0] == picked_triangle
    return {"screen": screen_values, "triangle": triangle_values, "island": island_values}


def stencil_and_masks(capi: object, surface: dict, parts: dict) -> tuple[np.ndarray, object]:
    """Resolve the stencil, then intersect every mask class before deposition."""
    positions = (capi.ctex_vec2d * TEXELS)(
        *(capi.ctex_vec2d(float(row[0]), float(row[1])) for row in surface["positions"][:, :2]))
    stencil = sized("ctex_paint_stencil_descriptor", width=WIDTH, height=HEIGHT,
                    screen_positions=positions, screen_position_count=TEXELS,
                    image_width=2, image_height=2,
                    image_opacity=doubles([0.0, 1.0, 1.0, 0.0]), image_opacity_count=4,
                    position=capi.ctex_vec2d(0.0, 0.0), scale=capi.ctex_vec2d(2.0, 2.0))
    info, values = sized("ctex_paint_stencil_info"), (capi.c_double * TEXELS)()
    call = (capi.byref(stencil), capi.byref(info))
    assert capi.ctex_paint_resolve_stencil_mask(*call, None, 0) == SUCCESS
    assert info.required_mask_value_count == TEXELS
    assert capi.ctex_paint_resolve_stencil_mask(*call, values, TEXELS) == SUCCESS
    resolved = np.ctypeslib.as_array(values).copy()
    quadrant = (surface["positions"][:, 0] < 0.0) == (surface["positions"][:, 1] >= 0.0)
    assert np.array_equal(resolved, np.where(quadrant, 0.0, 1.0))
    parts["stencil"] = resolved
    arrays = {name: doubles(part) for name, part in parts.items()}
    layer = (capi.ctex_paint_mask_view * 1)(capi.ctex_paint_mask_view(arrays["stencil"], TEXELS))
    views = {name: capi.ctex_paint_mask_view(array, TEXELS) for name, array in arrays.items()}
    masks = sized("ctex_paint_mask_inputs_descriptor",
                  active_layer_masks=layer, active_layer_mask_count=1,
                  colour_id_selection=capi.pointer(views["colour"]),
                  geometry_selection=capi.pointer(views["triangle"]),
                  screen_selection=capi.pointer(views["screen"]),
                  uv_island_selection=capi.pointer(views["island"]))
    mask_info, combined = sized("ctex_paint_mask_info"), (capi.c_double * TEXELS)()
    count = capi.c_size_t()
    merge = (WIDTH, HEIGHT, capi.byref(masks), capi.byref(mask_info))
    assert capi.ctex_paint_combine_masks(*merge, combined, TEXELS, capi.byref(count)) == SUCCESS
    assert count.value == TEXELS and mask_info.active_input_count == 5, "five restrictions"
    result = np.ctypeslib.as_array(combined).copy()
    assert np.allclose(result, np.prod(np.stack(list(parts.values())), axis=0)), "a product"
    return result, combined


def fill_blend_erase(capi: object, surface: dict, picked: dict, combined: np.ndarray,
                     selection: object) -> dict[str, object]:
    """Fill through the combined mask, blend over the snapshot, then erase the mask."""
    layer = (capi.ctex_paint_tool_channel_descriptor * 1)(
        channel(b"pbr.base_color", 3, vec4s([[0.0, 0.0, 0.0, 1.0]] * TEXELS)))
    material = (capi.ctex_paint_tool_channel_descriptor * 1)(
        channel(b"pbr.base_color", 3, picked["pixels"]))
    shaded, scope_values = (capi.ctex_vec4f * TEXELS)(), (capi.c_double * TEXELS)()
    descriptor = sized(
        "ctex_paint_fill_descriptor", width=WIDTH, height=HEIGHT,
        scope=capi.CTEX_PAINT_FILL_SELECTION, has_picked_texel=1, picked_texel=picked["texel"],
        maximum_angle_degrees=45.0, surface_texels=surface["texels"], surface_texel_count=TEXELS,
        coverage=surface["coverage"], coverage_count=TEXELS,
        triangle_identity=surface["triangles"], triangle_identity_count=TEXELS,
        uv_island_identity=surface["islands"], uv_island_identity_count=TEXELS,
        selection=selection, selection_count=TEXELS,
        enabled_layer_snapshot=layer, enabled_layer_channel_count=1,
        material=material, material_channel_count=1, blend_mode=capi.String(b"normal"))
    info = sized("ctex_paint_fill_info")
    outputs = sized(
        "ctex_paint_fill_outputs", scope_values=scope_values, scope_value_capacity=TEXELS,
        selected_triangle_ids=(capi.c_uint32 * 8)(), selected_triangle_capacity=8,
        channels=(capi.ctex_paint_tool_channel_output * 1)(
            capi.ctex_paint_tool_channel_output(
                capi.CTEX_PAINT_TOOL_CHANNEL_OUTPUT_CURRENT_SIZE, shaded, TEXELS)),
        channel_count=1)
    fill = (capi.byref(descriptor), capi.byref(info))
    assert capi.ctex_paint_apply_fill(*fill, capi.byref(outputs)) == SUCCESS
    scope, painted = np.ctypeslib.as_array(scope_values).copy(), rows_of(shaded)
    assert np.allclose(scope, combined), "the selection scope carries the combined mask"
    assert np.allclose(painted[:, 0], rows_of(picked["pixels"])[:, 0] * scope, atol=1.0e-6)
    deposition = (capi.ctex_paint_deposition_sample * TEXELS)(
        *(capi.ctex_paint_deposition_sample(value, 0.0, value, value, int(value > 0.0))
          for value in combined.tolist()))
    snapshot = vec4s([[0.2, 0.2, 0.2, 1.0]] * TEXELS)
    blend = capi.ctex_paint_blend_descriptor(
        capi.CTEX_PAINT_BLEND_DESCRIPTOR_CURRENT_SIZE, WIDTH, HEIGHT, capi.String(b"normal"),
        snapshot, shaded, deposition, TEXELS)
    blended, count = (capi.ctex_vec4f * TEXELS)(), capi.c_size_t()
    assert capi.ctex_paint_blend_snapshot(
        capi.byref(blend), blended, TEXELS, capi.byref(count)) == SUCCESS
    strength, base = combined.reshape(-1, 1), rows_of(snapshot)
    expected = np.where(strength > 0.0, base * (1.0 - strength) + painted * strength, base)
    assert count.value == TEXELS and np.allclose(rows_of(blended), expected, atol=1.0e-6)
    eraser = sized("ctex_paint_eraser_descriptor", width=WIDTH, height=HEIGHT,
                   target=capi.CTEX_PAINT_ERASER_TARGET_MASK,
                   stroke_start_values=doubles(combined), value_count=TEXELS,
                   deposition=deposition, deposition_count=TEXELS)
    eraser_info, erased = sized("ctex_paint_eraser_info"), (capi.c_double * TEXELS)()
    assert capi.ctex_paint_apply_eraser(
        capi.byref(eraser), capi.byref(eraser_info), erased, TEXELS) == SUCCESS
    assert np.allclose(np.ctypeslib.as_array(erased), combined * (1.0 - combined))
    dilation = capi.ctex_paint_seam_dilation_descriptor()
    assert capi.ctex_paint_seam_dilation_init(capi.byref(dilation)) == SUCCESS
    assert dilation.radius == 2, "the published seam dilation default"
    dilation.width, dilation.height, dilation.component_count = WIDTH, HEIGHT, 1
    dilation.pixels, dilation.pixel_count = doubles(painted[:, 0]), TEXELS
    dilation.coverage, dilation.coverage_count = surface["coverage"], TEXELS
    seam, healed = sized("ctex_paint_seam_dilation_info"), (capi.c_double * TEXELS)()
    assert capi.ctex_paint_dilate_uv_seams(
        capi.byref(dilation), capi.byref(seam), healed, TEXELS, capi.byref(count)) == SUCCESS
    covered, values = surface["covered"], np.ctypeslib.as_array(healed).copy()
    assert count.value == TEXELS and seam.dilated_texel_count == int((~covered).sum())
    assert np.allclose(values[covered], painted[covered, 0]), "covered texels are untouched"
    assert values[~covered].max() > 0.0, "the gutter now carries the island's colour"
    return {"painted_texels": int((scope > 0.0).sum()),
            "dilated_texels": int(seam.dilated_texel_count)}


def plan_tiles(capi: object) -> list[list[int]]:
    """Plan the storage tiles the stamp footprint plus its gutter can reach."""
    footprint = capi.ctex_paint_stamp_footprint(0, 60, 60, 70, 70)
    work = capi.ctex_paint_work_descriptor()
    assert capi.ctex_paint_work_init(capi.byref(work)) == SUCCESS
    assert (work.tile_size, work.dilation_radius) == (64, 2), "the published tiling defaults"
    work.canvas_width = work.canvas_height = 256
    work.stamp_footprints, work.stamp_footprint_count = capi.pointer(footprint), 1
    info, count = sized("ctex_paint_work_info"), capi.c_size_t()
    plan = (work, capi.byref(info))
    assert capi.ctex_paint_plan_work(*plan, None, 0, capi.byref(count)) == SUCCESS
    assert count.value == info.processed_tile_count == 4 and info.canvas_tile_count == 16
    tiles = (capi.ctex_paint_tile_coordinate * count.value)()
    assert capi.ctex_paint_plan_work(*plan, tiles, len(tiles), capi.byref(count)) == SUCCESS
    planned = sorted([int(entry.x), int(entry.y)] for entry in tiles)
    assert planned == [[0, 0], [0, 1], [1, 0], [1, 1]], "the footprint straddles both 64px seams"
    return planned


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--executor", default="cpu")
    arguments = parser.parse_args()
    capi = cybertexel.capi
    with cybertexel.Mesh(POSITIONS, TRIANGLES, normals=NORMALS, uv=UV,
                         partition_key="panel", partition_name="Panel") as mesh:
        pointer = ctypes.cast(mesh._require_open(), ctypes.POINTER(capi.ctex_mesh))
        mesh_info = sized("ctex_mesh_info")
        assert capi.ctex_mesh_get_info(pointer, capi.byref(mesh_info)) == SUCCESS
        index = capi.POINTER(capi.ctex_pick_index)()
        assert capi.ctex_pick_index_create(pointer, capi.byref(index)) == SUCCESS
        surface = surface_map(capi, pointer)
        accepted = rejected_coverage(capi, pointer, surface)
        picked = pick_and_select(capi, surface)
        parts = screen_and_polygon(capi, index, surface, mesh_info.revision, picked)
        parts["colour"] = picked["colour"]
        combined, selection = stencil_and_masks(capi, surface, parts)
        report = fill_blend_erase(capi, surface, picked, combined, selection)
        assert capi.ctex_paint_surface_map_cache_clear(surface["cache"]) == SUCCESS
        statistics = sized("ctex_paint_surface_map_statistics")
        assert capi.ctex_paint_surface_map_cache_get_statistics(
            surface["cache"], capi.byref(statistics)) == SUCCESS
        assert (statistics.entries, statistics.hits, statistics.misses) == (0, 0, 0)
        capi.ctex_paint_surface_map_cache_destroy(surface["cache"])
        capi.ctex_pick_index_destroy(index)
    arguments.output.mkdir(parents=True, exist_ok=True)
    summary = {"capabilities": list(CAPABILITIES),
               "accepted_texels": accepted,
               "combined_mask_texels": int((combined > 0.0).sum()),
               "planned_tiles": plan_tiles(capi),
               "texture_set_id": surface["texture_set_id"], **report}
    (arguments.output / "summary.json").write_text(
        json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
