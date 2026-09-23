#!/usr/bin/env python3
"""Answer every viewport picking question about a two-panel prop from one index.

Capabilities: picking, image-io.
A front panel and a back panel share the same UV square. The script builds one
acceleration structure over both and asks it what is under the cursor, what is
behind that, what a snapped point lands on, what a rubber band encloses, and
what a whole grid of rays reached, then asks the same question in UV space. The
grid answer is published through the raw image expansion and resampling calls.
"""

from __future__ import annotations

import argparse
import ctypes
import json
from pathlib import Path

import cybertexel
import numpy as np


CAPABILITIES = ("picking", "image-io")
CAPI = cybertexel.capi
OK = CAPI.CTEX_RESULT_SUCCESS
UNORM8 = CAPI.CTEX_SCALAR_REPRESENTATION_UNSIGNED_NORMALIZED
UV_SET = b"uv0"
TEXTURE_SET_ID = b"material/4:mesh/uv/3:uv0"
IDENTITY_SIZE = len(TEXTURE_SET_ID) + 1
GRID, PREVIEW = 16, 128
SUMMARY: dict[str, object] = {
    "capabilities": list(CAPABILITIES),
    "texture_set_id": TEXTURE_SET_ID.decode(),
}


def call(name: str, *arguments: object, expect: int = OK) -> None:
    assert getattr(CAPI, name)(*arguments) == expect


def sized(name: str, **fields: object) -> object:
    """Allocate a versioned ABI struct with its CURRENT_SIZE and named fields."""
    value = getattr(CAPI, name)()
    value.size = getattr(CAPI, f"{name.upper()}_CURRENT_SIZE")
    for field, item in fields.items():
        setattr(value, field, item)
    return value


def panels() -> tuple[np.ndarray, np.ndarray, np.ndarray, np.ndarray]:
    """A front panel at z=0 and a back panel at z=-1, two triangles each."""
    corners = [(0.0, 0.0), (1.0, 0.0), (1.0, 1.0), (0.0, 1.0)]
    positions = [[x, y, z] for z in (0.0, -1.0) for x, y in corners]
    return (
        np.asarray(positions, dtype=np.float32),
        np.asarray([[0, 1, 2], [0, 2, 3], [4, 5, 6], [4, 6, 7]], dtype=np.uint32),
        np.asarray([[0.0, 0.0, 1.0]] * 8, dtype=np.float32),
        np.asarray(corners * 2, dtype=np.float32),
    )


def binding() -> object:
    return sized("ctex_pick_texture_set_binding_descriptor", partition_index=0,
                 uv_set=CAPI.String(UV_SET))


def ray(origin: tuple[float, ...], direction: tuple[float, ...]) -> object:
    return CAPI.ctex_pick_ray(CAPI.ctex_vec3f(*origin), CAPI.ctex_vec3f(*direction))


def ray_query(index: object, cast: object, occlusion: int, backface: int) -> list:
    """Size the answer with NULL buffers, then collect it, as the ABI requires."""
    options = sized("ctex_pick_options_descriptor", maximum_distance=5.0,
                    occlusion_policy=occlusion, backface_policy=backface)
    texture_set, info = binding(), sized("ctex_pick_query_info")
    fixed = (index, cast, CAPI.byref(options), CAPI.byref(texture_set), 1)
    call("ctex_pick_ray_query", *fixed, None, 0, None, 0, CAPI.byref(info))
    assert info.required_texture_set_id_size == info.result_count * IDENTITY_SIZE
    assert info.visited_nodes >= 1
    if info.result_count == 0:
        return []
    hits = (CAPI.ctex_pick_hit * info.result_count)()
    names = CAPI.create_string_buffer(info.required_texture_set_id_size)
    call("ctex_pick_ray_query", *fixed, hits, len(hits), names, len(names), CAPI.byref(info))
    raw = bytes(names)
    for hit in hits:
        start = hit.texture_set_id_offset
        assert hit.has_hit == 1
        assert raw[start : start + hit.texture_set_id_size - 1] == TEXTURE_SET_ID
    return list(hits)


def surface_under_the_cursor(index: object) -> None:
    """The nearest surface, everything behind it, and the two distinct misses."""
    downward = ray((0.6, 0.25, 1.0), (0.0, 0.0, -1.0))
    nearest = ray_query(index, downward, CAPI.CTEX_PICK_OCCLUSION_NEAREST, 0)
    assert len(nearest) == 1 and nearest[0].triangle_index == 0
    assert abs(nearest[0].distance - 1.0) < 1e-6
    np.testing.assert_allclose([nearest[0].uv.x, nearest[0].uv.y], [0.6, 0.25], atol=1e-6)
    position = [nearest[0].position.x, nearest[0].position.y, nearest[0].position.z]
    np.testing.assert_allclose(position, [0.6, 0.25, 0.0], atol=1e-6)
    everything = ray_query(index, downward, CAPI.CTEX_PICK_OCCLUSION_ALL_HITS, 0)
    assert [hit.triangle_index for hit in everything] == [0, 2]
    assert [round(hit.distance, 6) for hit in everything] == [1.0, 2.0]
    upward = ray((0.6, 0.25, -2.0), (0.0, 0.0, 1.0))
    assert ray_query(index, upward, 0, CAPI.CTEX_PICK_BACKFACE_REJECT) == []
    behind = ray_query(index, upward, 0, CAPI.CTEX_PICK_BACKFACE_ACCEPT)
    assert [hit.triangle_index for hit in behind] == [2]
    assert ray_query(index, ray((4.0, 4.0, 1.0), (0.0, 0.0, -1.0)), 0, 0) == []
    SUMMARY.update(cursor_nearest_triangle=int(nearest[0].triangle_index),
                   cursor_position=[round(value, 6) for value in position],
                   cursor_occluded_triangles=[int(hit.triangle_index) for hit in everything])


def snap_a_dropped_point(index: object) -> None:
    """Snapping accepts a nearby point and clears the hit for a distant one."""
    texture_set, info = binding(), sized("ctex_pick_query_info")
    hit = CAPI.ctex_pick_hit()
    identifier = CAPI.create_string_buffer(IDENTITY_SIZE)

    def snap(point, radius) -> None:
        call("ctex_pick_snap_to_surface", index, CAPI.ctex_vec3f(*point), radius,
             CAPI.byref(texture_set), 1, CAPI.byref(hit), identifier, len(identifier),
             CAPI.byref(info))

    snap((0.6, 0.25, 0.2), 0.5)
    assert info.result_count == 1 and hit.has_hit == 1 and hit.triangle_index == 0
    assert abs(hit.distance - 0.2) < 1e-6 and identifier.value == TEXTURE_SET_ID
    hit.has_hit = 1
    snap((4.0, 4.0, 0.2), 0.1)
    assert info.result_count == 0 and hit.has_hit == 0
    SUMMARY["snap_distance"] = 0.2


def identity_view() -> object:
    view = sized("ctex_pick_screen_view_descriptor", viewport_width=100, viewport_height=100)
    for diagonal in range(4):
        view.view[diagonal * 5] = 1.0
        view.projection[diagonal * 5] = 1.0
    return view


def region(name: str, *arguments: object) -> list[int]:
    info = sized("ctex_pick_query_info")
    call(name, *arguments, None, 0, CAPI.byref(info))
    if info.result_count == 0:
        return []
    triangles = (CAPI.uint32_t * info.result_count)()
    call(name, *arguments, triangles, len(triangles), CAPI.byref(info))
    return [int(value) for value in triangles]


def screen_rays(view: object) -> None:
    """The centre pixel gives a perspective ray at the eye and an offset ortho one."""
    cast, centre = CAPI.ctex_pick_ray(), CAPI.ctex_vec2f(50.0, 50.0)

    def build(kind) -> None:
        call("ctex_pick_ray_from_screen", centre, CAPI.byref(view), kind, CAPI.byref(cast))

    build(CAPI.CTEX_PICK_PROJECTION_PERSPECTIVE)
    np.testing.assert_allclose(
        [cast.origin.x, cast.origin.y, cast.origin.z, cast.direction.z], [0.0, 0.0, 0.0, 1.0]
    )
    build(CAPI.CTEX_PICK_PROJECTION_ORTHOGRAPHIC)
    assert cast.origin.z == -1.0 and cast.direction.z == 1.0


def rubber_band_selection(index: object) -> None:
    """Screen rectangles and lassos, world spheres and boxes, and one refusal."""
    view = identity_view()
    screen_rays(view)
    rectangle, lasso = "ctex_pick_query_screen_rectangle", "ctex_pick_query_screen_lasso"

    def band(low, high) -> list[int]:
        return region(rectangle, index, CAPI.ctex_vec2f(*low), CAPI.ctex_vec2f(*high),
                      CAPI.byref(view))

    def outline(points) -> object:
        return (CAPI.ctex_vec2f * len(points))(*(CAPI.ctex_vec2f(*point) for point in points))

    corner, opposite = band((90.0, 50.0), (100.0, 60.0)), band((50.0, 0.0), (60.0, 10.0))
    assert corner == [0, 2] and opposite == [1, 3] and band((0.0, 0.0), (20.0, 20.0)) == []
    over = outline([(45.0, 45.0), (80.0, 45.0), (80.0, 20.0), (45.0, 20.0)])
    beside = outline([(0.0, 0.0), (5.0, 0.0), (5.0, 5.0), (0.0, 5.0)])
    assert region(lasso, index, over, 4, CAPI.byref(view)) == [0, 1, 2, 3]
    assert region(lasso, index, beside, 4, CAPI.byref(view)) == []
    sphere = "ctex_pick_query_world_sphere"
    tight = region(sphere, index, CAPI.ctex_vec3f(0.6, 0.25, 0.0), 0.1)
    wide = region(sphere, index, CAPI.ctex_vec3f(0.6, 0.25, -0.5), 0.75)
    assert tight == [0] and wide == [0, 1, 2, 3]
    assert region(sphere, index, CAPI.ctex_vec3f(5.0, 5.0, 5.0), 0.5) == []
    box = "ctex_pick_query_world_box"
    back = region(box, index, CAPI.ctex_vec3f(0.0, 0.0, -1.1), CAPI.ctex_vec3f(0.5, 0.5, -0.9))
    front = region(box, index, CAPI.ctex_vec3f(-0.1, -0.1, -0.1), CAPI.ctex_vec3f(1.1, 1.1, 0.1))
    assert back == [2, 3] and front == [0, 1]
    info, sentinel = sized("ctex_pick_query_info"), (CAPI.uint32_t * 1)(99)
    call(rectangle, index, CAPI.ctex_vec2f(45.0, 20.0), CAPI.ctex_vec2f(80.0, 55.0),
         CAPI.byref(view), sentinel, 1, CAPI.byref(info),
         expect=CAPI.CTEX_RESULT_BUFFER_TOO_SMALL)
    assert info.result_count == 4 and sentinel[0] == 99
    SUMMARY.update(region_corner_rectangle=corner, region_opposite_rectangle=opposite,
                   region_tight_sphere=tight, region_back_box=back, region_front_box=front)


def grid_rays() -> object:
    """A 16x16 sweep over a window one and a half times the panel."""
    rays = (CAPI.ctex_pick_ray * (GRID * GRID))()
    span = [-0.25 + (step + 0.5) * 1.5 / GRID for step in range(GRID)]
    for row in range(GRID):
        for column in range(GRID):
            rays[row * GRID + column] = ray((span[column], span[row], 1.0), (0.0, 0.0, -1.0))
    return rays


def sweep_the_panel(index: object) -> np.ndarray:
    """One batched sweep, then the same sweep refused by budget and cancellation."""
    progress: list[tuple[int, int]] = []
    cancelling = [0]

    @CAPI.ctex_pick_cancel_callback
    def is_cancelled(_user_data: object) -> int:
        return cancelling[0]

    @CAPI.ctex_pick_progress_callback
    def report_progress(completed: int, total: int, _user_data: object) -> None:
        progress.append((int(completed), int(total)))

    unlimited = ctypes.c_size_t(-1).value
    control = sized("ctex_pick_batch_control_descriptor", memory_ceiling_bytes=unlimited,
                    progress_interval=64, user_data=None, is_cancelled=is_cancelled,
                    report_progress=report_progress)
    texture_set, rays = binding(), grid_rays()
    info = sized("ctex_pick_batch_info")

    def sweep(hits=None, names=None, expect=OK) -> None:
        call("ctex_pick_nearest_batch", index, rays, len(rays), 5.0,
             CAPI.CTEX_PICK_BACKFACE_ACCEPT, CAPI.byref(texture_set), 1, CAPI.byref(control),
             hits, 0 if hits is None else len(hits), names, 0 if names is None else len(names),
             CAPI.byref(info), expect=expect)

    sweep()
    assert info.status == CAPI.CTEX_PICK_BATCH_COMPLETE
    assert info.processed_rays == GRID * GRID and info.required_hit_count == GRID * GRID
    assert info.required_texture_set_id_size == 100 * IDENTITY_SIZE
    assert info.visited_nodes == GRID * GRID and info.tested_leaf_triangles == 400
    assert progress == [(0, 256), (64, 256), (128, 256), (192, 256), (256, 256)]
    hits = (CAPI.ctex_pick_hit * info.required_hit_count)()
    names = CAPI.create_string_buffer(info.required_texture_set_id_size)
    sweep(hits, names)
    assert sum(hit.has_hit for hit in hits) == 100
    assert sorted({hit.triangle_index for hit in hits if hit.has_hit}) == [0, 1]
    control.memory_ceiling_bytes = 1
    sweep(expect=CAPI.CTEX_RESULT_OVER_BUDGET)
    assert info.status == CAPI.CTEX_PICK_BATCH_MEMORY_CEILING_EXCEEDED
    assert info.required_memory_bytes > 1
    control.memory_ceiling_bytes = unlimited
    cancelling[0] = 1
    sweep(expect=CAPI.CTEX_RESULT_CANCELLED)
    assert info.status == CAPI.CTEX_PICK_BATCH_CANCELLED and info.processed_rays == 0
    coverage = np.zeros(GRID * GRID, dtype=np.uint8)
    for ordinal, hit in enumerate(hits):
        if hit.has_hit:
            coverage[ordinal] = 255 if hit.triangle_index == 0 else 128
    coverage = coverage.reshape(GRID, GRID)
    assert sorted(np.unique(coverage).tolist()) == [0, 128, 255]
    assert int((coverage == 255).sum()) == 55 and int((coverage == 128).sum()) == 45
    SUMMARY.update(batch_rays=GRID * GRID, batch_reached=100,
                   batch_progress_reports=len(progress), batch_tested_leaf_triangles=400)
    return coverage


def uv_space_survey(mesh_pointer: object) -> None:
    """The same panel asked in UV space instead of world space."""
    index = ctypes.POINTER(CAPI.ctex_uv_pick_index)()
    call("ctex_uv_pick_index_create", mesh_pointer, CAPI.String(UV_SET), CAPI.byref(index))
    try:
        info = sized("ctex_pick_index_info")
        call("ctex_uv_pick_index_get_info", index, CAPI.byref(info))
        assert info.build_count == 1 and info.triangle_count == 4 and info.node_count >= 1
        texture_set, query = binding(), sized("ctex_pick_query_info")
        hit = CAPI.ctex_pick_hit()
        identifier = CAPI.create_string_buffer(IDENTITY_SIZE)

        def pick(u, v) -> None:
            call("ctex_pick_uv_query", index, CAPI.ctex_vec2f(u, v), CAPI.byref(texture_set),
                 CAPI.byref(hit), identifier, len(identifier), CAPI.byref(query))

        pick(0.6, 0.25)
        assert query.result_count == 1 and hit.triangle_index == 0
        assert identifier.value == TEXTURE_SET_ID
        np.testing.assert_allclose(
            [hit.position.x, hit.position.y, hit.position.z], [0.6, 0.25, 0.0], atol=1e-6
        )
        hit.has_hit = 1
        pick(2.0, 2.0)
        assert query.result_count == 0 and hit.has_hit == 0
        SUMMARY.update(uv_index_triangles=int(info.triangle_count), uv_miss_is_distinct=True)
    finally:
        CAPI.ctex_uv_pick_index_destroy(index)


def publish_coverage(coverage: np.ndarray) -> np.ndarray:
    """Grayscale coverage becomes a tinted RGB blow-up through the image pipeline."""
    source = np.ascontiguousarray(coverage)
    expansion = sized("ctex_image_channel_expansion_descriptor", width=GRID, height=GRID,
                      source_channel_count=1, scalar_representation=UNORM8, bit_depth=8,
                      source_row_stride_bytes=0, target_channel_count=3)
    report = sized("ctex_image_channel_expansion_info")
    origin = source.ctypes.data_as(ctypes.c_void_p)

    def expand(target, capacity, expect=OK) -> None:
        call("ctex_image_expand_channels", origin, source.nbytes, CAPI.byref(expansion),
             CAPI.byref(report), target, capacity, expect=expect)

    expand(None, 0)
    assert report.rule == CAPI.CTEX_IMAGE_CHANNEL_EXPANSION_GRAYSCALE_TO_RGB
    assert report.channel_count == 3 and report.bit_depth == 8
    assert report.required_pixel_buffer_size == GRID * GRID * 3
    sentinel = np.full(1, 0xA5, dtype=np.uint8)
    expand(sentinel.ctypes.data_as(ctypes.c_void_p), 1, CAPI.CTEX_RESULT_BUFFER_TOO_SMALL)
    assert int(sentinel[0]) == 0xA5
    rgb = np.zeros(report.required_pixel_buffer_size, dtype=np.uint8)
    expand(rgb.ctypes.data_as(ctypes.c_void_p), rgb.nbytes)
    np.testing.assert_array_equal(rgb.reshape(GRID, GRID, 3)[:, :, 0], coverage)
    tinted = rgb.reshape(GRID, GRID, 3).copy()
    for value, tint in ((255, (237, 79, 63)), (128, (70, 190, 170)), (0, (34, 45, 66))):
        tinted[coverage == value] = tint
    packed = np.ascontiguousarray(tinted).ravel()
    resample = sized("ctex_image_resample_descriptor", source_width=GRID, source_height=GRID,
                     channel_count=3, scalar_representation=UNORM8, bit_depth=8,
                     source_row_stride_bytes=0, output_width=PREVIEW, output_height=PREVIEW,
                     filter=CAPI.CTEX_IMAGE_RESAMPLE_FILTER_NEAREST, maximum_output_bytes=0)
    scaled = sized("ctex_image_resample_info")
    pointer = packed.ctypes.data_as(ctypes.c_void_p)

    def run(target, capacity, expect=OK) -> None:
        call("ctex_image_resample", pointer, packed.nbytes, CAPI.byref(resample),
             CAPI.byref(scaled), target, capacity, expect=expect)

    run(None, 0)
    assert scaled.filter == CAPI.CTEX_IMAGE_RESAMPLE_FILTER_NEAREST
    assert (scaled.width, scaled.height, scaled.channel_count) == (PREVIEW, PREVIEW, 3)
    assert scaled.required_pixel_buffer_size == PREVIEW * PREVIEW * 3
    preview = np.zeros(scaled.required_pixel_buffer_size, dtype=np.uint8)
    run(preview.ctypes.data_as(ctypes.c_void_p), preview.nbytes)
    assert set(np.unique(preview).tolist()) == set(np.unique(packed).tolist())
    resample.maximum_output_bytes = 1
    run(None, 0, CAPI.CTEX_RESULT_OVER_BUDGET)
    assert CAPI.ctex_get_last_diagnostic_code() == CAPI.CTEX_DIAGNOSTIC_IMAGE_LIMIT_EXCEEDED
    return preview.reshape(PREVIEW, PREVIEW, 3)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--executor", default="cpu")
    arguments = parser.parse_args()

    positions, triangles, normals, uv = panels()
    with cybertexel.Mesh(positions, triangles, normals=normals, uv=uv) as mesh:
        mesh_pointer = ctypes.cast(mesh._require_open(), ctypes.POINTER(CAPI.ctex_mesh))
        index = ctypes.POINTER(CAPI.ctex_pick_index)()
        call("ctex_pick_index_create", mesh_pointer, CAPI.byref(index))
        try:
            info = sized("ctex_pick_index_info")
            call("ctex_pick_index_get_info", index, CAPI.byref(info))
            assert info.build_count == 1 and info.triangle_count == 4
            assert info.node_count >= 1 and info.mesh_revision != 0
            SUMMARY.update(index_build_count=int(info.build_count),
                           index_triangles=int(info.triangle_count))
            surface_under_the_cursor(index)
            snap_a_dropped_point(index)
            rubber_band_selection(index)
            coverage = sweep_the_panel(index)
            uv_space_survey(mesh_pointer)
        finally:
            CAPI.ctex_pick_index_destroy(index)

    arguments.output.mkdir(parents=True, exist_ok=True)
    encoded = cybertexel.encode_image(publish_coverage(coverage))
    (arguments.output / "pick_coverage.png").write_bytes(encoded)
    (arguments.output / "summary.json").write_text(
        json.dumps(SUMMARY, indent=2, sort_keys=True) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
