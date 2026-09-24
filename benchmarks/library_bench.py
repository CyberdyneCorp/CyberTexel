#!/usr/bin/env python3
"""Measure the library-side reference-device budgets through the Python binding.

`device-gate` decides only figures from an exact named reference device, so this
writes a schema-1 measurement run naming the device, date, commit and command
rather than printing timings. A case whose declared configuration this runner
cannot build is left out and named in `not_measured`, which the gate reports as
unmeasured — never as a pass.

Host-owned figures — input-to-visible and residency traffic — are not measurable
from a binding. They come from the reference hosts under `hosts/`.

The run also carries the stamp-scaling case, which the gate decides on its own:
one 64-pixel-radius stamp applied to the tiles it touches on a 2048-square and a
16384-square canvas, to show that a stamp costs what it touches rather than what
the canvas holds.
"""

from __future__ import annotations

import argparse
import ctypes
import json
import subprocess
import time
from datetime import date, timezone, datetime
from pathlib import Path

import cybertexel
import numpy as np


ROOT = Path(__file__).resolve().parents[1]

INTERACTIVE = "interactive-4k"
BATCH = "batch-4k"

CHANNELS = ("pbr.base_color", "pbr.roughness", "pbr.metallic", "pbr.normal")
LAYER_COUNT = 8
SIZE = 4096
MESH_TRIANGLES = 250_000

SCALING = "stamp-scaling"
# The declared scaling configuration: one 64-pixel-radius stamp and the identical
# touched tiles on a 2048-square and a 16384-square texture set.
TILE_SIZE = 64
STAMP_RADIUS = 64
# Tile-aligned and far enough inside both canvases that neither clips the stamp,
# so both resolutions plan exactly the same touched tiles.
STAMP_CENTRE = 1024
SCALING_SIZES = (2048, 16384)


def percentile(values: list[float], fraction: float) -> float:
    ordered = sorted(values)
    if not ordered:
        raise ValueError("no timings were recorded")
    index = min(len(ordered) - 1, max(0, round(fraction * (len(ordered) - 1))))
    return ordered[index]


def repeat(operation, *, warmup: int = 3, samples: int = 30, setup=None) -> list[float]:
    """Time an operation repeatedly, in milliseconds.

    `setup` runs outside the timed region. `device-gate` refuses a batch that
    stands in for the operation it names, so anything that is not the operation
    under test belongs in setup rather than inside the measurement.
    """

    for _ in range(warmup):
        operation(setup() if setup else None)
    timings = []
    for _ in range(samples):
        prepared = setup() if setup else None
        start = time.perf_counter()
        operation(prepared)
        timings.append((time.perf_counter() - start) * 1000.0)
    return timings


def grid_mesh(triangles: int) -> tuple[np.ndarray, np.ndarray, np.ndarray, np.ndarray]:
    """A UV-unwrapped grid carrying at least the requested triangle count."""

    side = int(np.ceil(np.sqrt(triangles / 2.0)))
    axis = np.linspace(0.0, 1.0, side + 1, dtype=np.float32)
    u, v = np.meshgrid(axis, axis, indexing="xy")
    positions = np.stack([u, v, np.zeros_like(u)], axis=-1).reshape(-1, 3)
    normals = np.tile(np.array([0.0, 0.0, 1.0], dtype=np.float32), (positions.shape[0], 1))
    uvs = np.stack([u, v], axis=-1).reshape(-1, 2)
    corners = np.arange((side + 1) * (side + 1), dtype=np.uint32).reshape(side + 1, side + 1)
    a = corners[:-1, :-1].reshape(-1)
    b = corners[:-1, 1:].reshape(-1)
    c = corners[1:, 1:].reshape(-1)
    d = corners[1:, :-1].reshape(-1)
    triangle_indices = np.empty((a.size * 2, 3), dtype=np.uint32)
    triangle_indices[0::2] = np.stack([a, b, c], axis=-1)
    triangle_indices[1::2] = np.stack([a, c, d], axis=-1)
    return positions, normals, uvs, triangle_indices


def build_document(size: int) -> tuple[cybertexel.Document, cybertexel.TextureSet]:
    """The declared configuration: one texture set, four channels, eight layers."""

    document = cybertexel.Document()
    texture_set = document.create_texture_set(
        "Bench", partition_key="bench", width=size, height=size
    )
    for semantic in CHANNELS:
        document.set_channel_enabled(texture_set, semantic)
    document.append_layers(
        texture_set,
        [
            cybertexel.LayerEntry(
                identifier=f"paint.{index}",
                display_name=f"Layer {index}",
                kind=cybertexel.LayerKind.PAINT,
            )
            for index in range(LAYER_COUNT)
        ],
    )
    return document, texture_set


def document_resident_bytes(document: cybertexel.Document) -> int:
    capi = cybertexel.capi
    info = capi.ctex_document_memory_info()
    info.size = capi.CTEX_DOCUMENT_MEMORY_INFO_CURRENT_SIZE
    handle = ctypes.cast(
        document._require_open(), ctypes.POINTER(capi.ctex_document)  # noqa: SLF001
    )
    assert (
        capi.ctex_document_get_memory_report(handle, capi.byref(info), None, 0, None, 0)
        == capi.CTEX_RESULT_SUCCESS
    )
    return int(info.total_resident_bytes)


def measure_delta_query(samples: int) -> list[float]:
    document, texture_set = build_document(SIZE)
    pool = cybertexel.SnapshotPool(budget_bytes=256 << 20)
    document.write_channel_pixel(texture_set, CHANNELS[0], 1, 1, b"\x11\x22\x33")
    timings = repeat(
        lambda _: pool.current_cursor(document, texture_set, CHANNELS[0]), samples=samples
    )
    pool.close()
    document.close()
    return timings


def measure_tile_readback(samples: int) -> list[float]:
    document, texture_set = build_document(SIZE)
    pool = cybertexel.SnapshotPool(budget_bytes=256 << 20)
    state = {"x": 0}

    def prepare():
        """Dirty one tile and pin it. None of this is the readback."""

        before = pool.current_cursor(document, texture_set, CHANNELS[0])
        state["x"] = (state["x"] + 1) % SIZE
        document.write_channel_pixel(
            texture_set, CHANNELS[0], state["x"], 7, bytes([state["x"] % 251, 9, 9])
        )
        return pool.snapshot(document, texture_set, CHANNELS[0], since=before)

    def readback(snapshot) -> None:
        handle = snapshot.begin_host_readback()
        handle.complete([bytes(size) for size in handle.tile_byte_sizes])
        handle.close()
        snapshot.close()

    timings = repeat(readback, samples=samples, setup=prepare)
    pool.close()
    document.close()
    return timings


def measure_emission(samples: int) -> list[float]:
    counter = {"n": 0}

    def emit(_prepared=None) -> None:
        # A distinct identity each run so the emission cache cannot serve the
        # measurement instead of the code generator it is meant to time.
        counter["n"] += 1
        cybertexel.emit_default_host_material(
            stable_identity=f"bench/emission/{counter['n']}",
            output_identity="material-output",
            width=SIZE,
            height=SIZE,
        )

    return repeat(emit, samples=samples)


def measure_generator(samples: int) -> tuple[list[float], int]:
    positions, normals, uvs, triangles = grid_mesh(MESH_TRIANGLES)
    mesh = cybertexel.Mesh(
        positions, triangles, normals=normals, uv=uvs, partition_key="bench"
    )
    document, texture_set = build_document(SIZE)
    maps = cybertexel.MeshMapSet(document, texture_set, mesh)
    # The position-gradient generator declares position as a required input, so
    # the run imports one rather than choosing a generator that needs nothing.
    axis = np.linspace(0.0, 1.0, SIZE, dtype=np.float32)
    u, v = np.meshgrid(axis, axis, indexing="xy")
    position = np.stack([u, v, np.zeros_like(u)], axis=-1).astype(np.float32)
    # 3 is position-xyz in the channel-meaning vocabulary.
    maps.import_map(cybertexel.MeshMapKind.POSITION, position, channel_meaning=3)
    timings = repeat(
        lambda _: maps.generate_mask(
            cybertexel.MeshMapGeneratorKind.POSITION_GRADIENT, SIZE, SIZE
        ),
        warmup=1,
        samples=max(3, samples // 6),
    )
    count = int(triangles.shape[0])
    maps.close()
    document.close()
    mesh.close()
    return timings, count


def sized(name: str) -> object:
    """A capi structure carrying its current version size."""

    capi = cybertexel.capi
    value = getattr(capi, name)()
    value.size = getattr(capi, f"{name.upper()}_CURRENT_SIZE")
    return value


def stamp_canvas(size: int) -> cybertexel.Mesh:
    """A flat canvas whose position space is the texture set's texel space.

    `ctex_paint_evaluate_tile_deposition` rasterizes one storage tile as a unit
    of UV, so a UV span of `size / TILE_SIZE` makes one UV unit one 64-texel
    tile and one position unit one texel. The stamp below can then be placed in
    texel coordinates, identically on both canvases.
    """

    extent = float(size)
    span = extent / TILE_SIZE
    positions = np.array(
        [[0, 0, 0], [extent, 0, 0], [extent, extent, 0], [0, extent, 0]], dtype=np.float32
    )
    triangles = np.array([[0, 1, 2], [0, 2, 3]], dtype=np.uint32)
    normals = np.array([[0.0, 0.0, 1.0]] * 4, dtype=np.float32)
    uv = np.array([[0, 0], [span, 0], [span, span], [0, span]], dtype=np.float32)
    return cybertexel.Mesh(
        positions, triangles, normals=normals, uv=uv, partition_key="bench"
    )


def resolve_single_stamp() -> tuple[object, object, object]:
    """One stamp of `STAMP_RADIUS` texels at the canvas-independent centre."""

    capi = cybertexel.capi
    settings = capi.ctex_stroke_settings_descriptor()
    assert capi.ctex_stroke_settings_init(capi.byref(settings)) == capi.CTEX_RESULT_SUCCESS
    settings.radius = float(STAMP_RADIUS)
    settings.opacity = 0.9
    settings.flow = 0.8
    frame = capi.ctex_stroke_frame(
        capi.ctex_vec3d(1.0, 0.0, 0.0),
        capi.ctex_vec3d(0.0, 1.0, 0.0),
        capi.ctex_vec3d(0.0, 0.0, 1.0),
    )
    samples = (capi.ctex_stroke_input_sample * 1)(
        capi.ctex_stroke_input_sample(
            capi.CTEX_STROKE_INPUT_SAMPLE_CURRENT_SIZE,
            capi.ctex_vec3d(float(STAMP_CENTRE), float(STAMP_CENTRE), 0.0),
            frame,
            0,
            0,
            0.0,
            capi.ctex_vec2d(0.0, 0.0),
        )
    )
    info = sized("ctex_resolved_stroke_info")
    stamps_needed, segments_needed = capi.c_size_t(), capi.c_size_t()
    arguments = (capi.byref(settings), samples, 1, capi.byref(info))
    assert (
        capi.ctex_stroke_resolve(
            *arguments, None, 0, capi.byref(stamps_needed), None, 0, capi.byref(segments_needed)
        )
        == capi.CTEX_RESULT_SUCCESS
    )
    stamps = (capi.ctex_resolved_stamp * stamps_needed.value)()
    segments = (capi.ctex_swept_segment * max(1, segments_needed.value))()
    assert (
        capi.ctex_stroke_resolve(
            *arguments,
            stamps,
            len(stamps),
            capi.byref(stamps_needed),
            segments,
            segments_needed.value,
            capi.byref(segments_needed),
        )
        == capi.CTEX_RESULT_SUCCESS
    )
    assert stamps_needed.value == 1, "the scaling case applies exactly one stamp"
    stroke = capi.ctex_resolved_stroke_descriptor(
        capi.CTEX_RESOLVED_STROKE_DESCRIPTOR_CURRENT_SIZE,
        info.reconstruction_version,
        info.tip_mode,
        info.symmetry_instance_count,
        stamps,
        stamps_needed.value,
        segments,
        segments_needed.value,
    )
    return stroke, stamps, segments


class BoundedStamp:
    """One stamp applied to exactly the storage tiles its footprint reaches.

    Everything the stamp does not do — the canvas, the stroke resolution, the
    tile buffers and the channel descriptors — is built here, in setup. `apply`
    plans the touched tiles from the canvas and then deposits and blends the
    stamp on each of them, and is the only thing the run times.
    """

    TEXELS = TILE_SIZE * TILE_SIZE

    def __init__(self, size: int) -> None:
        capi = cybertexel.capi
        self.capi = capi
        self.size = size
        self.mesh = stamp_canvas(size)
        self.mesh_pointer = ctypes.cast(
            self.mesh._require_open(), ctypes.POINTER(capi.ctex_mesh)  # noqa: SLF001
        )
        self.stroke, self._stamps, self._segments = resolve_single_stamp()
        self._build_work_plan()
        self._build_tile_buffers()

    def _build_work_plan(self) -> None:
        capi = self.capi
        # `ctex_paint_plan_work` takes an exact half-open texel footprint.
        self.footprint = capi.ctex_paint_stamp_footprint(
            0,
            STAMP_CENTRE - STAMP_RADIUS,
            STAMP_CENTRE - STAMP_RADIUS,
            STAMP_CENTRE + STAMP_RADIUS,
            STAMP_CENTRE + STAMP_RADIUS,
        )
        self.work = capi.ctex_paint_work_descriptor()
        assert capi.ctex_paint_work_init(capi.byref(self.work)) == capi.CTEX_RESULT_SUCCESS
        assert self.work.tile_size == TILE_SIZE, "the published storage-tile size changed"
        self.work.canvas_width = self.work.canvas_height = self.size
        self.work.stamp_footprints = capi.pointer(self.footprint)
        self.work.stamp_footprint_count = 1
        self.work_info = sized("ctex_paint_work_info")
        self.tile_count = capi.c_size_t()
        assert (
            capi.ctex_paint_plan_work(
                capi.byref(self.work), capi.byref(self.work_info), None, 0,
                capi.byref(self.tile_count),
            )
            == capi.CTEX_RESULT_SUCCESS
        )
        self.tiles = (capi.ctex_paint_tile_coordinate * self.tile_count.value)()

    def _build_tile_buffers(self) -> None:
        capi = self.capi
        texels = self.TEXELS
        self.tile = capi.ctex_paint_tile_coverage_descriptor(
            capi.CTEX_PAINT_TILE_COVERAGE_DESCRIPTOR_CURRENT_SIZE,
            capi.String(b"uv0"),
            TILE_SIZE,
            TILE_SIZE,
            capi.ctex_vec2d(0.0, 0.0),
        )
        self.deposition_descriptor = sized("ctex_paint_deposition_descriptor")
        self.deposition_descriptor.mode = capi.CTEX_PAINT_DEPOSITION_NON_BUILDING
        self.deposition_descriptor.alpha_discard_format = capi.CTEX_ALPHA_DISCARD_UNORM8
        self.deposition_info = sized("ctex_paint_deposition_info")
        self.deposition = (capi.ctex_paint_deposition_sample * texels)()
        self.deposition_count = capi.c_size_t()
        vectors = capi.ctex_vec4f * texels
        self._layer_pixels = [
            vectors(*(capi.ctex_vec4f(0.04, 0.06, 0.09, 1.0) for _ in range(texels)))
            for _ in CHANNELS
        ]
        self._material_pixels = [
            vectors(*(capi.ctex_vec4f(0.90, 0.20, 0.08, 1.0) for _ in range(texels)))
            for _ in CHANNELS
        ]
        self._output_pixels = [vectors() for _ in CHANNELS]
        self.layers = self._channels(self._layer_pixels)
        self.material = self._channels(self._material_pixels)
        outputs = capi.ctex_paint_tool_channel_output * len(CHANNELS)
        self.outputs = outputs(
            *(
                capi.ctex_paint_tool_channel_output(
                    capi.CTEX_PAINT_TOOL_CHANNEL_OUTPUT_CURRENT_SIZE, pixels, texels
                )
                for pixels in self._output_pixels
            )
        )
        self.brush = capi.ctex_paint_brush_descriptor(
            capi.CTEX_PAINT_BRUSH_DESCRIPTOR_CURRENT_SIZE,
            TILE_SIZE,
            TILE_SIZE,
            self.layers,
            len(CHANNELS),
            self.material,
            len(CHANNELS),
            self.deposition,
            texels,
            capi.String(b"normal"),
        )
        self.brush_info = sized("ctex_paint_brush_info")

    def _channels(self, pixels: list[object]) -> object:
        capi = self.capi
        array = capi.ctex_paint_tool_channel_descriptor * len(CHANNELS)
        return array(
            *(
                capi.ctex_paint_tool_channel_descriptor(
                    capi.CTEX_PAINT_TOOL_CHANNEL_DESCRIPTOR_CURRENT_SIZE,
                    capi.String(semantic.encode("utf-8")),
                    3,
                    pixels[index],
                    self.TEXELS,
                )
                for index, semantic in enumerate(CHANNELS)
            )
        )

    def _stamp_tile(self, x: int, y: int) -> None:
        capi = self.capi
        self.tile.tile_origin = capi.ctex_vec2d(float(x), float(y))
        assert (
            capi.ctex_paint_evaluate_tile_deposition(
                self.mesh_pointer,
                capi.byref(self.tile),
                capi.byref(self.stroke),
                capi.byref(self.deposition_descriptor),
                capi.byref(self.deposition_info),
                self.deposition,
                self.TEXELS,
                capi.byref(self.deposition_count),
            )
            == capi.CTEX_RESULT_SUCCESS
        )
        assert (
            capi.ctex_paint_apply_brush(
                capi.byref(self.brush), capi.byref(self.brush_info), self.outputs, len(CHANNELS)
            )
            == capi.CTEX_RESULT_SUCCESS
        )

    def apply(self, _prepared: object = None) -> None:
        """The operation under test: plan the touched tiles and stamp them."""

        capi = self.capi
        assert (
            capi.ctex_paint_plan_work(
                capi.byref(self.work),
                capi.byref(self.work_info),
                self.tiles,
                len(self.tiles),
                capi.byref(self.tile_count),
            )
            == capi.CTEX_RESULT_SUCCESS
        )
        for index in range(self.tile_count.value):
            self._stamp_tile(self.tiles[index].x, self.tiles[index].y)

    def touched_tiles(self) -> list[tuple[int, int]]:
        self.apply()
        return [(int(tile.x), int(tile.y)) for tile in self.tiles]

    def written_texels(self) -> int:
        """Texels this stamp deposits over the whole touched area."""

        written = 0
        for index in range(self.tile_count.value):
            self._stamp_tile(self.tiles[index].x, self.tiles[index].y)
            written += sum(int(sample.write != 0) for sample in self.deposition)
        return written

    def close(self) -> None:
        self.mesh.close()


def measure_stamp_scaling(samples: int) -> dict[str, object]:
    """Time the identical stamp on a 2048-square and a 16384-square canvas."""

    timings: dict[int, list[float]] = {}
    planned: dict[int, list[tuple[int, int]]] = {}
    written: dict[int, int] = {}
    canvas_tiles: dict[int, int] = {}
    for size in SCALING_SIZES:
        stamp = BoundedStamp(size)
        planned[size] = stamp.touched_tiles()
        written[size] = stamp.written_texels()
        canvas_tiles[size] = int(stamp.work_info.canvas_tile_count)
        timings[size] = repeat(stamp.apply, samples=samples)
        stamp.close()
    small, large = SCALING_SIZES
    assert planned[small] == planned[large], "the two canvases must touch identical tiles"
    assert written[small] == written[large] > 0, "the stamp must deposit the same texels"
    return {
        "configuration": SCALING,
        "metric": "median_ms",
        "samples": samples,
        "stamp_radius_texels": STAMP_RADIUS,
        "tile_size": TILE_SIZE,
        "touched_tile_count": len(planned[small]),
        "written_texel_count": written[small],
        "canvas_tile_counts": {str(size): canvas_tiles[size] for size in SCALING_SIZES},
        f"resolution_{small}_ms": percentile(timings[small], 0.5),
        f"resolution_{large}_ms": percentile(timings[large], 0.5),
    }


def measurement(
    budget_id: str, value: float, unit: str, configuration: str
) -> dict[str, object]:
    return {
        "budget_id": budget_id,
        "value": value,
        "unit": unit,
        "configuration": configuration,
        "batch_size": 1,
    }


def git_commit() -> str:
    return subprocess.run(
        ["git", "rev-parse", "HEAD"], cwd=ROOT, capture_output=True, text=True, check=True
    ).stdout.strip()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--device", required=True, help="reference device id")
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--samples", type=int, default=30)
    arguments = parser.parse_args()

    measurements: list[dict[str, object]] = []
    not_measured: list[str] = []

    measurements.append(
        measurement(
            "desktop-delta-query",
            percentile(measure_delta_query(arguments.samples), 0.95),
            "ms",
            INTERACTIVE,
        )
    )
    measurements.append(
        measurement(
            "desktop-tile-readback",
            percentile(measure_tile_readback(arguments.samples), 0.95),
            "ms",
            INTERACTIVE,
        )
    )
    measurements.append(
        measurement(
            "desktop-emission",
            percentile(measure_emission(arguments.samples), 0.95),
            "ms",
            BATCH,
        )
    )
    generator, triangle_count = measure_generator(arguments.samples)
    measurements.append(
        measurement("desktop-generator", percentile(generator, 0.95), "ms", BATCH)
    )

    scaling = measure_stamp_scaling(arguments.samples)

    not_measured.extend(
        [
            "desktop-stamp-median, desktop-stamp-p95 and desktop-stroke are declared at"
            " interactive-4k, whose 250k-triangle mesh cannot be honestly timed through the"
            " binding: ctex_paint_evaluate_tile_deposition rasterizes the whole mesh for every"
            " storage tile and takes no cached surface map, so a per-stamp figure would report"
            " that rasterization rather than the stamp; the stamp-scaling case avoids the"
            " question by using a flat canvas, identical on both resolutions",
            "desktop-composite, desktop-smart-material and desktop-export need their"
            " surfaces wrapped (task 16.12)",
            "every peak_working_bytes budget needs per-operation working-set"
            " accounting rather than document residency (task 16.12)",
            "desktop-visible-* and the synchronous readback budgets are host figures"
            " produced by hosts/ (tasks 17.12, 17.14), already measured on the M3 Pro",
            "every tablet budget is a figure from hosts/ios-probe on the iPad Air"
            " 13-inch (M3), not from this process",
        ]
    )

    run = {
        "schema": 1,
        "device_id": arguments.device,
        "date": date.today().isoformat(),
        "commit": git_commit(),
        "command": "just bench",
        "recorded_utc": datetime.now(timezone.utc).isoformat(timespec="seconds"),
        "mesh_triangles": triangle_count,
        "layers": LAYER_COUNT,
        "channels": len(CHANNELS),
        "measurements": measurements,
        "stamp_scaling": scaling,
        "not_measured": not_measured,
    }
    arguments.output.parent.mkdir(parents=True, exist_ok=True)
    arguments.output.write_text(json.dumps(run, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"ok: wrote {len(measurements)} measurements to {arguments.output}")
    small, large = SCALING_SIZES
    print(
        f"stamp scaling: {scaling[f'resolution_{small}_ms']:.4f} ms at {small} and"
        f" {scaling[f'resolution_{large}_ms']:.4f} ms at {large} for"
        f" {scaling['touched_tile_count']} identical touched tiles"
    )
    for note in not_measured:
        print(f"not measured: {note}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
