#!/usr/bin/env python3
"""Resolve, rasterize and apply one deterministic brush stroke.

Capabilities: stroke-model, paint-engine, paint-tools.
The example passes timestamped input through the native stroke reconstruction,
tile deposition and multi-channel brush APIs before publishing painted pixels.
"""

from __future__ import annotations

import argparse
import ctypes
import json
from pathlib import Path

import cybertexel
import numpy as np


CAPABILITIES = ("stroke-model", "paint-engine", "paint-tools")
WIDTH = 256
HEIGHT = 64
PIXEL_COUNT = WIDTH * HEIGHT


def input_sample(capi: object, x: float, y: float, timestamp: int) -> object:
    frame = capi.ctex_stroke_frame(
        capi.ctex_vec3d(1.0, 0.0, 0.0),
        capi.ctex_vec3d(0.0, 1.0, 0.0),
        capi.ctex_vec3d(0.0, 0.0, 1.0),
    )
    return capi.ctex_stroke_input_sample(
        capi.CTEX_STROKE_INPUT_SAMPLE_CURRENT_SIZE,
        capi.ctex_vec3d(x, y, 0.0),
        frame,
        timestamp,
        0,
        0.0,
        capi.ctex_vec2d(0.0, 0.0),
    )


def resolve_stroke(capi: object) -> tuple[object, object, object]:
    settings = capi.ctex_stroke_settings_descriptor()
    assert capi.ctex_stroke_settings_init(capi.byref(settings)) == capi.CTEX_RESULT_SUCCESS
    settings.radius = 0.32
    settings.spacing_fraction = 0.2
    settings.opacity = 0.92
    settings.flow = 0.8
    samples = (capi.ctex_stroke_input_sample * 3)(
        input_sample(capi, 1.0, 0.25, 0),
        input_sample(capi, 5.0, 0.75, 4_000_000),
        input_sample(capi, 9.0, 0.25, 8_000_000),
    )
    info = capi.ctex_resolved_stroke_info()
    info.size = capi.CTEX_RESOLVED_STROKE_INFO_CURRENT_SIZE
    stamp_count = capi.c_size_t()
    segment_count = capi.c_size_t()
    assert (
        capi.ctex_stroke_resolve(
            capi.byref(settings),
            samples,
            len(samples),
            capi.byref(info),
            None,
            0,
            capi.byref(stamp_count),
            None,
            0,
            capi.byref(segment_count),
        )
        == capi.CTEX_RESULT_SUCCESS
    )
    stamps = (capi.ctex_resolved_stamp * stamp_count.value)()
    segments = (capi.ctex_swept_segment * segment_count.value)()
    assert (
        capi.ctex_stroke_resolve(
            capi.byref(settings),
            samples,
            len(samples),
            capi.byref(info),
            stamps,
            len(stamps),
            capi.byref(stamp_count),
            segments,
            len(segments),
            capi.byref(segment_count),
        )
        == capi.CTEX_RESULT_SUCCESS
    )
    assert stamp_count.value > len(samples)
    assert segment_count.value == stamp_count.value - 1
    assert stamps[0].ordinal == 0
    assert stamps[-1].ordinal == stamp_count.value - 1
    np.testing.assert_allclose(
        [stamps[0].position.x, stamps[0].position.y], [1.0, 0.25]
    )
    np.testing.assert_allclose(
        [stamps[-1].position.x, stamps[-1].position.y], [9.0, 0.25]
    )
    descriptor = capi.ctex_resolved_stroke_descriptor(
        capi.CTEX_RESOLVED_STROKE_DESCRIPTOR_CURRENT_SIZE,
        info.reconstruction_version,
        info.tip_mode,
        info.symmetry_instance_count,
        stamps,
        stamp_count.value,
        segments,
        segment_count.value,
    )
    return descriptor, stamps, segments


def evaluate_deposition(capi: object, mesh: object, stroke: object) -> tuple[object, np.ndarray]:
    tile = capi.ctex_paint_tile_coverage_descriptor(
        capi.CTEX_PAINT_TILE_COVERAGE_DESCRIPTOR_CURRENT_SIZE,
        capi.String(b"uv0"),
        WIDTH,
        HEIGHT,
        capi.ctex_vec2d(0.0, 0.0),
    )
    coverage = (capi.c_double * PIXEL_COUNT)()
    coverage_count = capi.c_size_t()
    assert (
        capi.ctex_paint_evaluate_tile_coverage(
            mesh,
            capi.byref(tile),
            capi.byref(stroke),
            coverage,
            len(coverage),
            capi.byref(coverage_count),
        )
        == capi.CTEX_RESULT_SUCCESS
    )
    assert coverage_count.value == PIXEL_COUNT

    descriptor = capi.ctex_paint_deposition_descriptor()
    descriptor.size = capi.CTEX_PAINT_DEPOSITION_DESCRIPTOR_CURRENT_SIZE
    descriptor.mode = capi.CTEX_PAINT_DEPOSITION_NON_BUILDING
    descriptor.alpha_discard_format = capi.CTEX_ALPHA_DISCARD_UNORM8
    info = capi.ctex_paint_deposition_info()
    info.size = capi.CTEX_PAINT_DEPOSITION_INFO_CURRENT_SIZE
    deposition = (capi.ctex_paint_deposition_sample * PIXEL_COUNT)()
    deposition_count = capi.c_size_t()
    assert (
        capi.ctex_paint_evaluate_tile_deposition(
            mesh,
            capi.byref(tile),
            capi.byref(stroke),
            capi.byref(descriptor),
            capi.byref(info),
            deposition,
            len(deposition),
            capi.byref(deposition_count),
        )
        == capi.CTEX_RESULT_SUCCESS
    )
    coverage_values = np.ctypeslib.as_array(coverage).reshape(HEIGHT, WIDTH)
    assert deposition_count.value == PIXEL_COUNT
    assert info.applied_stamp_count == stroke.stamp_count
    assert float(coverage_values.max()) > 0.9
    assert np.count_nonzero(coverage_values) > WIDTH
    return deposition, coverage_values


def apply_brush(capi: object, deposition: object) -> np.ndarray:
    vector_array = capi.ctex_vec4f * PIXEL_COUNT
    layer_pixels = vector_array(
        *(capi.ctex_vec4f(0.035, 0.055, 0.09, 1.0) for _ in range(PIXEL_COUNT))
    )
    material_pixels = vector_array(
        *(capi.ctex_vec4f(0.95, 0.20, 0.075, 1.0) for _ in range(PIXEL_COUNT))
    )
    output_pixels = vector_array()
    channel_array = capi.ctex_paint_tool_channel_descriptor * 1
    layer = channel_array(
        capi.ctex_paint_tool_channel_descriptor(
            capi.CTEX_PAINT_TOOL_CHANNEL_DESCRIPTOR_CURRENT_SIZE,
            capi.String(b"pbr.base_color"),
            3,
            layer_pixels,
            PIXEL_COUNT,
        )
    )
    material = channel_array(
        capi.ctex_paint_tool_channel_descriptor(
            capi.CTEX_PAINT_TOOL_CHANNEL_DESCRIPTOR_CURRENT_SIZE,
            capi.String(b"pbr.base_color"),
            3,
            material_pixels,
            PIXEL_COUNT,
        )
    )
    descriptor = capi.ctex_paint_brush_descriptor(
        capi.CTEX_PAINT_BRUSH_DESCRIPTOR_CURRENT_SIZE,
        WIDTH,
        HEIGHT,
        layer,
        len(layer),
        material,
        len(material),
        deposition,
        PIXEL_COUNT,
        capi.String(b"normal"),
    )
    info = capi.ctex_paint_brush_info()
    info.size = capi.CTEX_PAINT_BRUSH_INFO_CURRENT_SIZE
    output = capi.ctex_paint_tool_channel_output(
        capi.CTEX_PAINT_TOOL_CHANNEL_OUTPUT_CURRENT_SIZE,
        output_pixels,
        PIXEL_COUNT,
    )
    assert (
        capi.ctex_paint_apply_brush(
            capi.byref(descriptor), capi.byref(info), capi.byref(output), 1
        )
        == capi.CTEX_RESULT_SUCCESS
    )
    assert (info.applied_channel_count, info.required_pixels_per_channel) == (1, PIXEL_COUNT)
    values = np.ctypeslib.as_array(output_pixels).view(np.float32).reshape(HEIGHT, WIDTH, 4)
    rgb = np.rint(np.clip(values[:, :, :3], 0.0, 1.0) * 255.0).astype(np.uint8)
    assert tuple(rgb[0, 0]) == (9, 14, 23)
    assert int(rgb[:, :, 0].max()) > 20
    return rgb


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--executor", default="cpu")
    arguments = parser.parse_args()
    assert arguments.executor == "cpu", "paint coverage uses the CPU reference executor"

    capi = cybertexel.capi
    positions = np.array(
        [[0, 0, 0], [10, 0, 0], [10, 1, 0], [0, 1, 0]], dtype=np.float32
    )
    triangles = np.array([[0, 1, 2], [0, 2, 3]], dtype=np.uint32)
    normals = np.array([[0, 0, 1]] * 4, dtype=np.float32)
    uv = np.array([[0, 0], [1, 0], [1, 1], [0, 1]], dtype=np.float32)
    stroke, stamps, segments = resolve_stroke(capi)
    with cybertexel.Mesh(positions, triangles, normals=normals, uv=uv) as mesh:
        mesh_pointer = ctypes.cast(
            mesh._require_open(), ctypes.POINTER(capi.ctex_mesh)
        )
        deposition, coverage = evaluate_deposition(capi, mesh_pointer, stroke)
        preview = apply_brush(capi, deposition)

    written = sum(int(sample.write != 0) for sample in deposition)
    assert 0 < written < PIXEL_COUNT
    arguments.output.mkdir(parents=True, exist_ok=True)
    (arguments.output / "brush_stroke.png").write_bytes(cybertexel.encode_image(preview))
    summary = {
        "capabilities": list(CAPABILITIES),
        "coverage_maximum": round(float(coverage.max()), 6),
        "coverage_mean": round(float(coverage.mean()), 6),
        "painted_pixels": written,
        "stamp_count": len(stamps),
        "swept_segment_count": len(segments),
    }
    (arguments.output / "summary.json").write_text(
        json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )


if __name__ == "__main__":
    main()
