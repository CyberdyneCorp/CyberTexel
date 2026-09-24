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


def paint_strokes(surface: dict[str, np.ndarray], extent: int,
                  existing: np.ndarray | None = None) -> np.ndarray:
    """Deposit paint through the canonical pipeline and shade it over the base.

    The deposition stage decides coverage per texel; `ctex_paint_apply_brush`
    shades the enabled channel against the stroke-start snapshot. Nothing here
    paints by writing a NumPy array: the values come back from the library.
    """
    texels = extent * extent
    covered = surface["covered"]
    position, normal = surface["position"], surface["normal"]

    # The model's up axis is whichever one the surface normals actually spread
    # along most; picking it from the data keeps the demo honest about assets
    # authored Y-up or Z-up.
    axis = int(np.argmax(normal[covered].std(axis=0))) if covered.any() else 1
    up = normal[:, axis]
    height = position[:, axis]
    span = (np.percentile(height[covered], 5), np.percentile(height[covered], 95)) \
        if covered.any() else (0.0, 1.0)
    level = np.clip((height - span[0]) / max(span[1] - span[0], 1e-6), 0.0, 1.0)

    # Moss climbs the lower third and only on upward-facing surface.
    moss = covered & (up > 0.45) & (level < 0.34)
    # Lichen catches the bright side higher up, on faces turned away from moss.
    lichen = covered & (up < -0.35) & (level > 0.55)

    # The stroke-start snapshot is caller supplied by design, so a host paints
    # on top of the material the asset already ships rather than replacing it.
    if existing is not None:
        base = existing.reshape(texels, 4).astype(np.float64)
    else:
        base = np.zeros((texels, 4), dtype=np.float64)
        base[covered] = [0.34, 0.24, 0.17, 1.0]
    out = np.zeros((texels, 4), dtype=np.float64)

    for mask, colour, strength in ((moss, (0.19, 0.38, 0.13), 0.9),
                                   (lichen, (0.62, 0.66, 0.48), 0.6)):
        if not mask.any():
            continue
        deposition = (CAPI.ctex_paint_deposition_sample * texels)()
        for index in np.flatnonzero(mask):
            deposition[int(index)].write = 1
            deposition[int(index)].strength = strength
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
        base = np.array([[p.x, p.y, p.z, p.w] for p in output], dtype=np.float64)
        out = base
    return out if out.any() else base


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mesh", required=True, type=Path, help=".npz of pos/nrm/uv/tri arrays")
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--extent", type=int, default=1024)
    parser.add_argument("--base-colour", type=Path,
                        help="the material the asset already ships, painted over")
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
    painted = paint_strokes(surface, arguments.extent, existing)

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
    }
    (arguments.output / "summary.json").write_text(
        json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps(summary, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
