#!/usr/bin/env python3
"""Paint a real model with CyberTexel and render what came out.

This is a DEMO, not a numbered example. `examples` requires every example to run
on assets committed to the repository; this one takes a model path, so it lives
here instead of under examples/ and is not part of the gated suite.

It rasterizes the model's surface map once, picks surface texels in UV space,
drives them through the canonical deposition and brush pipeline, and writes the
painted channel out as a PNG that can be mapped straight back onto the model.
"""

from __future__ import annotations

import argparse
import ctypes
import json
from pathlib import Path

import cybertexel
import numpy as np

CAPI = cybertexel.capi
OK = CAPI.CTEX_RESULT_SUCCESS
BASE = b"pbr.base_color"


def sized(name: str, **fields: object) -> object:
    value = getattr(CAPI, f"ctex_{name}")()
    value.size = getattr(CAPI, f"CTEX_{name.upper()}_CURRENT_SIZE")
    for field, supplied in fields.items():
        setattr(value, field, supplied)
    return value


def ok(result: int, what: str) -> None:
    if result != OK:
        raise SystemExit(f"{what} failed: result {result} ({cybertexel.capi.ctex_get_last_diagnostic()})")


def surface_map(mesh_ptr: object, extent: int) -> dict[str, np.ndarray]:
    """Rasterize the model's UV surface map: which texels are on the surface."""
    texels = extent * extent
    cache = CAPI.POINTER(CAPI.ctex_paint_surface_map_cache)()
    ok(CAPI.ctex_paint_surface_map_cache_create(CAPI.byref(cache)), "surface cache create")
    request = sized("paint_surface_map_request", uv_set=CAPI.String(b"uv0"),
                    width=extent, height=extent)
    info = sized("paint_surface_map_info")
    lookup = (cache, mesh_ptr, CAPI.byref(request), CAPI.byref(info))
    ok(CAPI.ctex_paint_surface_map_cache_lookup(*lookup, None), "surface map sizing")
    surface = (CAPI.ctex_paint_surface_texel * texels)()
    coverage = (CAPI.c_ubyte * texels)()
    triangles = (CAPI.c_uint32 * texels)()
    islands = (CAPI.c_uint32 * texels)()
    set_id = CAPI.create_string_buffer(info.required_texture_set_id_size)
    uv_set = CAPI.create_string_buffer(info.required_uv_set_size)
    buffers = sized("paint_surface_map_buffers",
                    texture_set_id=CAPI.String(set_id), texture_set_id_size=len(set_id),
                    uv_set=CAPI.String(uv_set), uv_set_size=len(uv_set),
                    surface_texels=surface, surface_texel_capacity=texels,
                    coverage=coverage, coverage_capacity=texels,
                    triangle_identity=triangles, triangle_identity_capacity=texels,
                    uv_island_identity=islands, uv_island_identity_capacity=texels)
    ok(CAPI.ctex_paint_surface_map_cache_lookup(*lookup, CAPI.byref(buffers)), "surface map read")
    positions = np.array([[t.position.x, t.position.y, t.position.z] for t in surface],
                         dtype=np.float64)
    normals = np.array([[t.normal.x, t.normal.y, t.normal.z] for t in surface], dtype=np.float64)
    CAPI.ctex_paint_surface_map_cache_destroy(cache)
    return {
        "covered": np.ctypeslib.as_array(coverage).astype(bool).copy(),
        "island": np.ctypeslib.as_array(islands).copy(),
        "triangle": np.ctypeslib.as_array(triangles).copy(),
        "position": positions,
        "normal": normals,
        "texture_set_id": set_id.value.decode(),
    }


def trunk_paths(positions: np.ndarray, axis: int, samples: int = 14) -> list[np.ndarray]:
    """Trace a path up each trunk, from the model's own geometry.

    Vertices in the lowest part of the model are the trunks. Splitting them at
    their widest horizontal gap separates one trunk from the next, and the mean
    horizontal position of each cluster at successive heights is a line running
    up the middle of that trunk.
    """
    lateral = [a for a in (0, 1, 2) if a != axis]
    height = positions[:, axis]
    low, high = height.min(), height.max()
    base = positions[height < low + 0.30 * (high - low)]
    if len(base) < 8:
        return []
    spread = lateral[int(np.argmax(np.ptp(base[:, lateral], axis=0)))]
    ordered = np.sort(base[:, spread])
    split = ordered[int(np.argmax(np.diff(ordered)))] if len(ordered) > 1 else 0.0
    paths = []
    for cluster in (base[base[:, spread] <= split], base[base[:, spread] > split]):
        if len(cluster) < 8:
            continue
        centre = cluster.mean(axis=0)
        top = low + 0.42 * (high - low)
        path = np.zeros((samples, 3))
        for step in range(samples):
            t = step / (samples - 1)
            path[step] = centre
            path[step, axis] = low + t * (top - low)
        paths.append(path)
    return paths


def resolve_stroke(path: np.ndarray, radius: float,
                   hardness: float = 0.35) -> tuple[object, list]:
    """Turn a 3D path into the library's canonical resolved stroke."""
    settings = CAPI.ctex_stroke_settings_descriptor()
    ok(CAPI.ctex_stroke_settings_init(CAPI.byref(settings)), "stroke settings")
    settings.radius = radius
    settings.spacing_fraction = 0.12
    settings.opacity = 1.0
    settings.flow = 0.9
    # Hardness is what makes a stroke a gradient. At the default 1.0 every
    # covered texel deposits the same strength and the stroke lands as a flat
    # band; at 0.35 the same sweep produces dozens of distinct strengths across
    # its width, which is the falloff a painted edge needs.
    settings.hardness = hardness
    settings.taper.enabled = 1
    settings.taper.entry_fraction = 0.18
    settings.taper.exit_fraction = 0.35
    frame = CAPI.ctex_stroke_frame(CAPI.ctex_vec3d(1.0, 0.0, 0.0),
                                   CAPI.ctex_vec3d(0.0, 1.0, 0.0),
                                   CAPI.ctex_vec3d(0.0, 0.0, 1.0))
    samples = (CAPI.ctex_stroke_input_sample * len(path))(*[
        CAPI.ctex_stroke_input_sample(
            CAPI.CTEX_STROKE_INPUT_SAMPLE_CURRENT_SIZE,
            CAPI.ctex_vec3d(*point), frame, index * 8_000_000, 0, 0.0,
            CAPI.ctex_vec2d(0.0, 0.0))
        for index, point in enumerate(path)])
    info = sized("resolved_stroke_info")
    stamp_count, segment_count = CAPI.c_size_t(), CAPI.c_size_t()
    probe = (CAPI.byref(settings), samples, len(samples), CAPI.byref(info))
    ok(CAPI.ctex_stroke_resolve(*probe, None, 0, CAPI.byref(stamp_count),
                                None, 0, CAPI.byref(segment_count)), "stroke sizing")
    stamps = (CAPI.ctex_resolved_stamp * stamp_count.value)()
    segments = (CAPI.ctex_swept_segment * segment_count.value)()
    ok(CAPI.ctex_stroke_resolve(*probe, stamps, len(stamps), CAPI.byref(stamp_count),
                                segments, len(segments), CAPI.byref(segment_count)),
       "stroke resolve")
    stroke = CAPI.ctex_resolved_stroke_descriptor(
        CAPI.CTEX_RESOLVED_STROKE_DESCRIPTOR_CURRENT_SIZE, info.reconstruction_version,
        info.tip_mode, info.symmetry_instance_count, stamps, stamp_count.value,
        segments, segment_count.value)
    return stroke, [stamps, segments, samples, settings]


def deposit_stroke(mesh_ptr: object, stroke: object, extent: int) -> np.ndarray:
    """Rasterize the swept stroke into per-texel deposition over the whole canvas."""
    texels = extent * extent
    tile = CAPI.ctex_paint_tile_coverage_descriptor(
        CAPI.CTEX_PAINT_TILE_COVERAGE_DESCRIPTOR_CURRENT_SIZE,
        CAPI.String(b"uv0"), extent, extent, CAPI.ctex_vec2d(0.0, 0.0))
    descriptor = sized("paint_deposition_descriptor",
                       mode=CAPI.CTEX_PAINT_DEPOSITION_NON_BUILDING,
                       alpha_discard_format=CAPI.CTEX_ALPHA_DISCARD_UNORM8)
    info = sized("paint_deposition_info")
    deposition = (CAPI.ctex_paint_deposition_sample * texels)()
    produced = CAPI.c_size_t()
    ok(CAPI.ctex_paint_evaluate_tile_deposition(
        mesh_ptr, CAPI.byref(tile), CAPI.byref(stroke), CAPI.byref(descriptor),
        CAPI.byref(info), deposition, texels, CAPI.byref(produced)), "tile deposition")
    return deposition, int(info.applied_stamp_count)


def paint_strokes(surface: dict[str, np.ndarray], extent: int, mesh_ptr: object,
                  positions: np.ndarray, existing: np.ndarray | None = None,
                  axis: int = 2) -> tuple[np.ndarray, dict[str, object]]:
    """Paint the model with real strokes, then shade them over what it ships.

    A height threshold is not painting: on a coarse mesh it snaps to whole
    triangles, because one triangle spans a narrow height range. A stroke is
    per texel by construction — `ctex_stroke_resolve` turns timestamped 3D
    samples into stamps and swept segments, and the deposition stage rasterizes
    that sweep against the surface.
    """
    texels = extent * extent
    covered = surface["covered"]
    normal = surface["normal"]

    # The stroke-start snapshot is caller supplied by design, so a host paints
    # on top of the material the asset already ships rather than replacing it.
    if existing is not None:
        base = existing.reshape(texels, 4).astype(np.float64)
    else:
        base = np.zeros((texels, 4), dtype=np.float64)
        base[covered] = [0.34, 0.24, 0.17, 1.0]

    # --- moss: strokes swept up each trunk --------------------------------
    paths = trunk_paths(positions, axis)
    strength = np.zeros(texels)
    stamps = segments = 0
    retained = []
    for path in paths:
        stroke, keep = resolve_stroke(path, radius=0.32)
        retained.append(keep)
        stamps += int(stroke.stamp_count)
        segments += int(stroke.swept_segment_count)
        deposition, _ = deposit_stroke(mesh_ptr, stroke, extent)
        swept = np.fromiter((sample.strength if sample.write else 0.0
                             for sample in deposition), dtype=np.float64, count=texels)
        strength = np.maximum(strength, swept)
    moss = covered & (strength > 0.01)

    # --- sun bleach: per-texel, following how much each texel faces up ------
    up = np.clip(normal[:, axis], 0.0, 1.0)
    bleach = covered & (up > 0.05)
    bleach_strength = 0.55 * up

    out = base
    for mask, colour, strengths in ((moss, (0.16, 0.30, 0.10), strength),
                                    (bleach, (0.78, 0.80, 0.62), bleach_strength)):
        active = mask & (strengths > 0.01)
        if not active.any():
            continue
        deposition = (CAPI.ctex_paint_deposition_sample * texels)()
        for index in np.flatnonzero(active):
            deposition[int(index)].write = 1
            deposition[int(index)].strength = float(strengths[index])
        snapshot = (CAPI.ctex_vec4f * texels)(*[CAPI.ctex_vec4f(*row) for row in base])
        material = (CAPI.ctex_vec4f * texels)(*[CAPI.ctex_vec4f(*colour, 1.0)] * texels)
        output = (CAPI.ctex_vec4f * texels)()
        layer = (CAPI.ctex_paint_tool_channel_descriptor * 1)(
            sized("paint_tool_channel_descriptor", semantic_id=CAPI.String(BASE),
                  component_count=4, pixels=snapshot, pixel_count=texels))
        mat = (CAPI.ctex_paint_tool_channel_descriptor * 1)(
            sized("paint_tool_channel_descriptor", semantic_id=CAPI.String(BASE),
                  component_count=4, pixels=material, pixel_count=texels))
        descriptor = sized("paint_brush_descriptor", width=extent, height=extent,
                           enabled_layer_snapshot=layer, enabled_layer_channel_count=1,
                           material=mat, material_channel_count=1,
                           deposition=deposition, deposition_count=texels,
                           blend_mode=CAPI.String(b"normal"))
        info = sized("paint_brush_info")
        outputs = (CAPI.ctex_paint_tool_channel_output * 1)(
            sized("paint_tool_channel_output", pixels=output, pixel_capacity=texels))
        ok(CAPI.ctex_paint_apply_brush(CAPI.byref(descriptor), CAPI.byref(info), outputs, 1),
           "apply brush")
        base = np.array([[q.x, q.y, q.z, q.w] for q in output], dtype=np.float64)
        out = base
    report = {"strokes": len(paths), "resolved_stamps": stamps,
              "swept_segments": segments,
              "moss_texels": int(moss.sum()), "bleach_texels": int(bleach.sum())}
    return out, report



def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mesh", required=True, type=Path, help=".npz of pos/nrm/uv/tri arrays")
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--extent", type=int, default=1024)
    parser.add_argument("--base-colour", type=Path,
                        help="the material the asset already ships, painted over")
    parser.add_argument("--up-axis", choices=("x", "y", "z"), default="z",
                        help="the asset's up axis; Blender writes Z-up")
    arguments = parser.parse_args()

    data = np.load(arguments.mesh, allow_pickle=True)
    pos, nrm, uv, tri = data["pos0"], data["nrm0"], data["uv0"], data["tri0"]
    mesh = cybertexel.Mesh(pos, tri, normals=nrm, uv=uv, uv_name="uv0", partition_key="model")
    document = cybertexel.Document()
    texture_set = document.create_texture_set(
        "Model", partition_key="model", width=arguments.extent, height=arguments.extent)
    document.set_channel_enabled(texture_set, BASE.decode())

    mesh_ptr = ctypes.cast(mesh._require_open(), ctypes.POINTER(CAPI.ctex_mesh))
    surface = surface_map(mesh_ptr, arguments.extent)
    existing = None
    if arguments.base_colour:
        decoded = cybertexel.decode_image(arguments.base_colour.read_bytes(),
                                          source_name=arguments.base_colour.name)
        pixels = decoded.pixels[:, :, :3].astype(np.float64) / 255.0
        step = pixels.shape[0] // arguments.extent
        if step > 1:
            pixels = pixels[::step, ::step]
        pixels = pixels[:arguments.extent, :arguments.extent]
        existing = np.concatenate(
            [pixels, np.ones((*pixels.shape[:2], 1))], axis=-1)
    painted, strokes = paint_strokes(surface, arguments.extent, mesh_ptr, pos, existing,
                                     axis="xyz".index(arguments.up_axis))

    rgba = np.clip(painted.reshape(arguments.extent, arguments.extent, 4), 0.0, 1.0)
    image = np.rint(rgba[:, :, :3] * 255.0).astype(np.uint8)
    arguments.output.mkdir(parents=True, exist_ok=True)
    (arguments.output / "painted_base_color.png").write_bytes(cybertexel.encode_image(image))

    covered = surface["covered"]
    summary = {
        "mesh": {"vertices": int(len(pos)), "triangles": int(len(tri))},
        "texture_set": surface["texture_set_id"],
        "extent": arguments.extent,
        "surface_texels": int(covered.sum()),
        "surface_coverage_percent": round(100.0 * covered.mean(), 2),
        "uv_islands": int(len(np.unique(surface["island"][covered]))),
        "painted_texels": int((rgba[:, :, 3].reshape(-1) > 0).sum()),
        **strokes,
    }
    (arguments.output / "summary.json").write_text(
        json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps(summary, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
