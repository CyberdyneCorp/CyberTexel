#!/usr/bin/env python3
"""Retouch a scuffed UV panel with the clone, blur and smear paint tools.

Capabilities: paint-tools, paint-engine
An artist repairs a scratched rail panel that occupies one 32x8 UV tile. Clone
copies clean plate from sixteen texels to the left through the aligned mapping,
never sampling outside the tile and refusing a cross-set source outright. Blur
then softens the patch boundary with explicit surface-aware neighborhoods that
cross the tile's horizontal UV seam, and smear drags the far edge back into the
untouched plate. Each tool filters only the immutable stroke-start snapshot the
previous tool published, every buffer is sized from the count the library itself
reported in a NULL query call, and every published raster is checked against an
independent NumPy model of the documented mapping.
"""

from __future__ import annotations

import argparse
import ctypes
import json
import os
from pathlib import Path

import cybertexel
import numpy as np


CAPABILITIES = ("paint-tools", "paint-engine")

capi = cybertexel.capi
byref, pointer, text = capi.byref, capi.pointer, capi.String
OK, TOO_SMALL = capi.CTEX_RESULT_SUCCESS, capi.CTEX_RESULT_BUFFER_TOO_SMALL
BAD_ARGUMENT, BAD_TOOL = capi.CTEX_RESULT_INVALID_ARGUMENT, capi.CTEX_DIAGNOSTIC_INVALID_PAINT_TOOL
WIDTH, HEIGHT = 32, 8
TEXELS = WIDTH * HEIGHT
CLONE_SHIFT, SCUFF = 16, slice(16, 28)
RAIL, TRIM = b"set:rail", b"set:trim"
BASE_COLOR, ROUGHNESS, NORMAL = b"pbr.base_color", b"pbr.roughness", b"normal"
NO_SAMPLE = capi.CTEX_PAINT_NO_CLONE_SAMPLE
SAMPLE = capi.ctex_paint_surface_filter_sample
FRAME = capi.ctex_stroke_frame(
    capi.ctex_vec3d(1.0, 0.0, 0.0), capi.ctex_vec3d(0.0, 1.0, 0.0), capi.ctex_vec3d(0.0, 0.0, 1.0)
)


def to_pixels(values: np.ndarray) -> object:
    buffer = (capi.ctex_vec4f * TEXELS)()
    source = np.ascontiguousarray(values, dtype=np.float32)
    ctypes.memmove(buffer, source.ctypes.data, ctypes.sizeof(buffer))
    return buffer


def from_pixels(buffer: object) -> np.ndarray:
    return np.ctypeslib.as_array(buffer).view(np.float32).reshape(TEXELS, 4).copy()


def snapshot_channels(rasters: tuple[np.ndarray, np.ndarray]) -> tuple[object, list]:
    """One base-colour and one roughness channel over the same tile."""
    held = [to_pixels(raster) for raster in rasters]
    array = (capi.ctex_paint_tool_channel_descriptor * 2)(
        *(
            capi.ctex_paint_tool_channel_descriptor(
                capi.CTEX_PAINT_TOOL_CHANNEL_DESCRIPTOR_CURRENT_SIZE, text(semantic), components,
                pixels, TEXELS)
            for (semantic, components), pixels in zip(((BASE_COLOR, 3), (ROUGHNESS, 1)), held)
        )
    )
    return array, held


def output_channels(count: int, capacity: int) -> tuple[object, list]:
    """Sentinel-filled outputs, so a refused call is visibly a no-op."""
    sentinel = capi.ctex_vec4f(2.0, 2.0, 2.0, 2.0)
    held = [(capi.ctex_vec4f * TEXELS)(*([sentinel] * TEXELS)) for _ in range(count)]
    array = (capi.ctex_paint_tool_channel_output * count)(
        *(
            capi.ctex_paint_tool_channel_output(
                capi.CTEX_PAINT_TOOL_CHANNEL_OUTPUT_CURRENT_SIZE, buffer, capacity)
            for buffer in held
        )
    )
    return array, held


def deposition_array(write: np.ndarray, strength: np.ndarray) -> object:
    samples = (capi.ctex_paint_deposition_sample * TEXELS)()
    flat_write, flat_strength = write.reshape(-1), strength.reshape(-1)
    for index in range(TEXELS):
        samples[index] = capi.ctex_paint_deposition_sample(
            0.0, 0.0, float(flat_strength[index]), 0.0, int(flat_write[index]))
    return samples


def deposited_strength(write: np.ndarray, strength: np.ndarray) -> np.ndarray:
    """The library keeps a sample's strength only when its write flag is set."""
    return np.where(write.reshape(-1) != 0, strength.reshape(-1), 0.0)


def blend_normal(base: np.ndarray, paint: np.ndarray, amount: np.ndarray) -> np.ndarray:
    factor = amount.astype(np.float32).reshape(-1, 1)
    mixed = base + (paint - base) * factor
    return np.clip(mixed, np.float32(0.0), np.float32(1.0)).astype(np.float32)


def build_plate(seed: int) -> tuple[np.ndarray, np.ndarray]:
    generator = np.random.default_rng(seed)
    grain = generator.integers(96, 160, size=(HEIGHT, WIDTH)).astype(np.float32) / np.float32(255)
    color = np.stack([grain, grain * np.float32(0.75), grain * np.float32(0.5),
                      np.ones_like(grain)], axis=-1)
    rough = np.float32(0.25) + grain * np.float32(0.5)
    color[:, SCUFF, :3] *= np.float32(0.35)
    rough[:, SCUFF] = np.float32(0.9)
    roughness = np.stack([rough, rough, rough, np.ones_like(rough)], axis=-1)
    return color.reshape(TEXELS, 4), roughness.reshape(TEXELS, 4)


def build_surface() -> tuple[object, object, np.ndarray]:
    """A welded tile apart from a two-by-two unmapped corner."""
    covered = np.ones((HEIGHT, WIDTH), dtype=bool)
    covered[0:2, 30:32] = False
    texels = (capi.ctex_paint_surface_texel * TEXELS)()
    coverage = (capi.uint8_t * TEXELS)()
    up = capi.ctex_vec3d(0.0, 0.0, 1.0)
    for row in range(HEIGHT):
        for col in range(WIDTH):
            index = row * WIDTH + col
            uv = capi.ctex_vec2d((col + 0.5) / WIDTH, (HEIGHT - 1 - row + 0.5) / HEIGHT)
            texels[index] = capi.ctex_paint_surface_texel(
                capi.ctex_vec3d(float(col), float(row), 0.0), up, up, uv,
                0 if covered[row, col] else 0xFFFFFFFF)
            coverage[index] = 1 if covered[row, col] else 0
    return texels, coverage, covered.reshape(-1)


def expected_clone_samples(covered: np.ndarray, mode: int) -> np.ndarray:
    """Aligned adds one constant UV offset; fixed samples the source anchor texel."""
    indices = np.full(TEXELS, NO_SAMPLE, dtype=np.uint64)
    anchor = (HEIGHT - 1 - int(0.5 * HEIGHT)) * WIDTH
    for index in range(TEXELS):
        if not covered[index]:
            continue
        if mode == capi.CTEX_PAINT_CLONE_FIXED:
            indices[index] = anchor
        elif index % WIDTH >= CLONE_SHIFT:
            indices[index] = index - CLONE_SHIFT
    return indices


def clone_stage(plate: tuple[np.ndarray, np.ndarray], surface: object, coverage: object,
                covered: np.ndarray) -> tuple[tuple[np.ndarray, np.ndarray], int, str]:
    write = np.zeros((HEIGHT, WIDTH), dtype=np.uint32)
    strength = np.zeros((HEIGHT, WIDTH), dtype=np.float64)
    write[1:7, 12:30] = 1
    strength[1:7, 12:30] = 0.5
    strength[1:7, 16:28] = 1.0
    write[3, 20] = 0  # a discarded sample keeps its full strength but must deposit nothing
    deposition = deposition_array(write, strength)
    layer, layer_held = snapshot_channels(plate)
    plate_snapshot, plate_held = snapshot_channels(plate)
    source = capi.ctex_paint_clone_source_descriptor(
        capi.CTEX_PAINT_CLONE_SOURCE_DESCRIPTOR_CURRENT_SIZE, text(RAIL),
        capi.ctex_vec2d(0.0, 0.5))
    descriptor = capi.ctex_paint_clone_descriptor(
        capi.CTEX_PAINT_CLONE_DESCRIPTOR_CURRENT_SIZE, WIDTH, HEIGHT,
        capi.CTEX_PAINT_CLONE_ALIGNED, text(RAIL), capi.ctex_vec2d(0.0, 0.0),
        capi.ctex_vec2d(0.5, 0.5), pointer(source),
        surface, TEXELS, coverage, TEXELS, layer, 2, plate_snapshot, 2, deposition, TEXELS,
        text(NORMAL))
    info = capi.ctex_paint_clone_info()
    info.size = capi.CTEX_PAINT_CLONE_INFO_CURRENT_SIZE

    assert capi.ctex_paint_apply_clone(byref(descriptor), byref(info), None) == OK
    samples, pixels = int(info.required_source_sample_count), int(info.required_pixels_per_channel)
    count = int(info.applied_channel_count)
    assert (samples, pixels, count) == (TEXELS, TEXELS, 2)
    assert (info.source_anchor_uv.x, info.source_anchor_uv.y) == (0.0, 0.5)
    assert (info.destination_anchor_uv.x, info.destination_anchor_uv.y) == (0.5, 0.5)

    indices = (capi.c_size_t * samples)(*([7] * samples))
    channels, held = output_channels(count, pixels)
    outputs = capi.ctex_paint_clone_outputs(
        capi.CTEX_PAINT_CLONE_OUTPUTS_CURRENT_SIZE, indices, samples - 1, channels, count)
    assert capi.ctex_paint_apply_clone(byref(descriptor), byref(info), byref(outputs)) == TOO_SMALL
    assert indices[0] == 7 and held[0][0].x == 2.0

    outputs.source_sample_capacity = samples
    assert capi.ctex_paint_apply_clone(byref(descriptor), byref(info), byref(outputs)) == OK
    published = np.ctypeslib.as_array(indices).astype(np.uint64)
    np.testing.assert_array_equal(
        published, expected_clone_samples(covered, capi.CTEX_PAINT_CLONE_ALIGNED))

    sampleable = published != NO_SAMPLE
    amount = deposited_strength(write, strength) * sampleable
    rasters = []
    for channel, plate_raster in enumerate(plate):
        sampled = np.zeros((TEXELS, 4), dtype=np.float32)
        sampled[sampleable] = plate_raster[published[sampleable].astype(np.intp)]
        actual = from_pixels(held[channel])
        np.testing.assert_allclose(actual, blend_normal(plate_raster, sampled, amount),
                                   rtol=0.0, atol=1e-6)
        rasters.append(actual)
    np.testing.assert_array_equal(rasters[0][3 * WIDTH + 20], plate[0][3 * WIDTH + 20])

    descriptor.mode = capi.CTEX_PAINT_CLONE_FIXED
    assert capi.ctex_paint_apply_clone(byref(descriptor), byref(info), byref(outputs)) == OK
    assert int(info.mode) == capi.CTEX_PAINT_CLONE_FIXED
    np.testing.assert_array_equal(
        np.ctypeslib.as_array(indices).astype(np.uint64),
        expected_clone_samples(covered, capi.CTEX_PAINT_CLONE_FIXED))

    descriptor.mode = capi.CTEX_PAINT_CLONE_ALIGNED
    source.texture_set_id = text(TRIM)
    refused = capi.ctex_paint_apply_clone(byref(descriptor), byref(info), byref(outputs))
    assert refused == BAD_ARGUMENT
    assert capi.ctex_get_last_diagnostic_code() == BAD_TOOL
    diagnostic = capi.ctex_get_last_diagnostic().decode("utf-8")
    assert TRIM.decode() in diagnostic and RAIL.decode() in diagnostic
    del layer_held, plate_held
    return (rasters[0], rasters[1]), samples, diagnostic


def blur_neighborhoods() -> tuple[object, list]:
    """Wrap horizontally across the tile's UV seam; clamp at the top and bottom."""
    held: list = []
    array = (capi.ctex_paint_blur_neighborhood_descriptor * TEXELS)()
    for row in range(HEIGHT):
        for col in range(WIDTH):
            index = row * WIDTH + col
            horizontal = (SAMPLE * 3)(
                SAMPLE(row * WIDTH + (col - 1) % WIDTH, FRAME, -1, 0, 1.0),
                SAMPLE(index, FRAME, 0, 0, 1.0),
                SAMPLE(row * WIDTH + (col + 1) % WIDTH, FRAME, 1, 0, 1.0))
            rows = [other for other in (row - 1, row, row + 1) if 0 <= other < HEIGHT]
            vertical = (SAMPLE * len(rows))(
                *(SAMPLE(other * WIDTH + col, FRAME, 0, other - row, 1.0) for other in rows))
            held.append((horizontal, vertical))
            array[index] = capi.ctex_paint_blur_neighborhood_descriptor(
                capi.CTEX_PAINT_BLUR_NEIGHBORHOOD_DESCRIPTOR_CURRENT_SIZE, FRAME,
                horizontal, 3, vertical, len(rows))
    return array, held


def reference_blur(raster: np.ndarray) -> np.ndarray:
    """A horizontal pass over the snapshot, then a vertical pass over its result."""
    grid = raster.astype(np.float64).reshape(HEIGHT, WIDTH, 4)
    horizontal = (np.roll(grid, 1, axis=1) + grid + np.roll(grid, -1, axis=1)) / 3.0
    vertical = np.empty_like(horizontal)
    for row in range(HEIGHT):
        rows = [other for other in (row - 1, row, row + 1) if 0 <= other < HEIGHT]
        vertical[row] = sum(horizontal[other] for other in rows) / float(len(rows))
    return vertical.reshape(TEXELS, 4).astype(np.float32)


def blur_stage(plate: tuple[np.ndarray, np.ndarray]) -> tuple[np.ndarray, np.ndarray]:
    write = np.zeros((HEIGHT, WIDTH), dtype=np.uint32)
    strength = np.zeros((HEIGHT, WIDTH), dtype=np.float64)
    write[:, 14:18] = 1
    strength[:, 14:16] = 1.0
    strength[:, 16:18] = 0.5
    write[:, 0], write[:, WIDTH - 1] = 1, 1  # the same pass runs along the wrap seam
    strength[:, 0], strength[:, WIDTH - 1] = 1.0, 1.0
    deposition = deposition_array(write, strength)
    snapshot, snapshot_held = snapshot_channels(plate)
    neighborhoods, neighborhood_held = blur_neighborhoods()
    descriptor = capi.ctex_paint_blur_descriptor(
        capi.CTEX_PAINT_BLUR_DESCRIPTOR_CURRENT_SIZE, WIDTH, HEIGHT, 1, snapshot, 2,
        deposition, TEXELS, text(NORMAL), neighborhoods, TEXELS)
    info = capi.ctex_paint_blur_info()
    info.size = capi.CTEX_PAINT_BLUR_INFO_CURRENT_SIZE

    assert capi.ctex_paint_apply_blur(byref(descriptor), byref(info), None, 0) == OK
    count, pixels = int(info.applied_channel_count), int(info.required_pixels_per_channel)
    assert (count, pixels) == (2, TEXELS)
    assert (int(info.resolved_radius), int(info.radius_clamped)) == (1, 0)
    for requested, resolved in ((0, 1), (5000, 4096)):
        descriptor.radius = requested
        assert capi.ctex_paint_apply_blur(byref(descriptor), byref(info), None, 0) == OK
        assert (int(info.resolved_radius), int(info.radius_clamped)) == (resolved, 1)

    descriptor.radius = 1
    channels, held = output_channels(count, pixels - 1)
    assert capi.ctex_paint_apply_blur(byref(descriptor), byref(info), channels, count) == TOO_SMALL
    assert held[0][0].x == 2.0
    channels, held = output_channels(count, pixels)
    assert capi.ctex_paint_apply_blur(byref(descriptor), byref(info), channels, count) == OK

    amount = deposited_strength(write, strength)
    rasters = []
    for channel, raster in enumerate(plate):
        actual = from_pixels(held[channel])
        np.testing.assert_allclose(actual, blend_normal(raster, reference_blur(raster), amount),
                                   rtol=0.0, atol=1e-6)
        rasters.append(actual)
    del snapshot_held, neighborhood_held
    return rasters[0], rasters[1]


def smear_mappings() -> object:
    """One upstream mapping per texel: drag the seam-wrapped left neighbour rightwards."""
    array = (capi.ctex_paint_smear_mapping_descriptor * TEXELS)()
    for row in range(HEIGHT):
        for col in range(WIDTH):
            array[row * WIDTH + col] = capi.ctex_paint_smear_mapping_descriptor(
                capi.CTEX_PAINT_SMEAR_MAPPING_DESCRIPTOR_CURRENT_SIZE, FRAME,
                SAMPLE(row * WIDTH + (col - 1) % WIDTH, FRAME, -1, 0, 1.0))
    return array


def smear_stage(plate: tuple[np.ndarray, np.ndarray]) -> tuple[np.ndarray, np.ndarray]:
    write = np.zeros((HEIGHT, WIDTH), dtype=np.uint32)
    strength = np.zeros((HEIGHT, WIDTH), dtype=np.float64)
    write[:, 26:30] = 1
    strength[:, 26:30] = 1.0
    write[:, 0], strength[:, 0] = 1, 1.0  # column zero drags the far side of the seam
    deposition = deposition_array(write, strength)
    snapshot, snapshot_held = snapshot_channels(plate)
    mappings = smear_mappings()
    descriptor = capi.ctex_paint_smear_descriptor(
        capi.CTEX_PAINT_SMEAR_DESCRIPTOR_CURRENT_SIZE, WIDTH, HEIGHT, 0.5, 1, 1, snapshot, 2,
        deposition, TEXELS, text(NORMAL), mappings, TEXELS)
    info = capi.ctex_paint_smear_info()
    info.size = capi.CTEX_PAINT_SMEAR_INFO_CURRENT_SIZE

    assert capi.ctex_paint_apply_smear(byref(descriptor), byref(info), None, 0) == OK
    count, pixels = int(info.applied_channel_count), int(info.required_pixels_per_channel)
    assert (count, pixels) == (2, TEXELS)
    assert (info.resolved_strength, int(info.strength_clamped)) == (0.5, 0)
    assert (int(info.resolved_footprint_radius_x), int(info.footprint_radius_x_clamped)) == (1, 0)
    assert (int(info.resolved_footprint_radius_y), int(info.footprint_radius_y_clamped)) == (1, 0)

    descriptor.strength = 1.7
    assert capi.ctex_paint_apply_smear(byref(descriptor), byref(info), None, 0) == OK
    assert (info.resolved_strength, int(info.strength_clamped)) == (1.0, 1)
    descriptor.strength, descriptor.footprint_radius_x, descriptor.footprint_radius_y = 0.5, 0, 0
    assert capi.ctex_paint_apply_smear(byref(descriptor), byref(info), None, 0) == BAD_ARGUMENT
    assert capi.ctex_get_last_diagnostic_code() == BAD_TOOL
    descriptor.footprint_radius_x, descriptor.footprint_radius_y = 1, 1

    channels, held = output_channels(count, pixels)
    assert capi.ctex_paint_apply_smear(byref(descriptor), byref(info), channels, count) == OK
    amount = deposited_strength(write, strength) * 0.5
    rasters = []
    for channel, raster in enumerate(plate):
        dragged = np.roll(raster.reshape(HEIGHT, WIDTH, 4), 1, axis=1).reshape(TEXELS, 4)
        actual = from_pixels(held[channel])
        np.testing.assert_allclose(actual, blend_normal(raster, dragged, amount),
                                   rtol=0.0, atol=1e-6)
        rasters.append(actual)
    del snapshot_held
    return rasters[0], rasters[1]


def to_rgb(raster: np.ndarray) -> np.ndarray:
    values = np.clip(raster[:, :3], 0.0, 1.0).reshape(HEIGHT, WIDTH, 3)
    return np.rint(values * 255.0).astype(np.uint8)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--executor", default="cpu")
    arguments = parser.parse_args()

    plate = build_plate(int(os.environ.get("CTEX_EXAMPLE_SEED", "1729")))
    surface, coverage, covered = build_surface()
    cloned, sample_count, diagnostic = clone_stage(plate, surface, coverage, covered)
    blurred = blur_stage(cloned)
    smeared = smear_stage(blurred)

    before = float(plate[0][:, 0].reshape(HEIGHT, WIDTH)[:, SCUFF].mean())
    after = float(smeared[0][:, 0].reshape(HEIGHT, WIDTH)[:, SCUFF].mean())
    assert after > before * 1.5

    arguments.output.mkdir(parents=True, exist_ok=True)
    stages = np.concatenate([to_rgb(raster[0]) for raster in (plate, cloned, blurred, smeared)])
    (arguments.output / "panel_retouch_stages.png").write_bytes(cybertexel.encode_image(stages))
    summary = {
        "blurred_texels": int((blurred[0] != cloned[0]).any(axis=1).sum()),
        "capabilities": list(CAPABILITIES),
        "clone_cross_set_diagnostic": diagnostic,
        "clone_source_sample_count": sample_count,
        "cloned_texels": int((cloned[0] != plate[0]).any(axis=1).sum()),
        "scuff_red_after": round(after, 6),
        "scuff_red_before": round(before, 6),
        "smeared_texels": int((smeared[0] != blurred[0]).any(axis=1).sum()),
        "uncovered_texels": int((~covered).sum()),
    }
    (arguments.output / "summary.json").write_text(
        json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
