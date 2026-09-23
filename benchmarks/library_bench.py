#!/usr/bin/env python3
"""Measure the library-side reference-device budgets through the Python binding.

`device-gate` decides only figures from an exact named reference device, so this
writes a schema-1 measurement run naming the device, date, commit and command
rather than printing timings. A case whose declared configuration this runner
cannot build is left out and named in `not_measured`, which the gate reports as
unmeasured — never as a pass.

Host-owned figures — input-to-visible and residency traffic — are not measurable
from a binding. They come from the reference hosts under `hosts/`.
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

    not_measured.extend(
        [
            "desktop-stamp-median, desktop-stamp-p95, desktop-stroke and stamp scaling"
            " need the bounded paint-work surface wrapped (task 16.12)",
            "desktop-composite, desktop-smart-material and desktop-export need their"
            " surfaces wrapped (task 16.12)",
            "every peak_working_bytes budget needs per-operation working-set"
            " accounting rather than document residency (task 16.12)",
            "desktop-visible-* and the synchronous readback budgets are host figures"
            " produced by hosts/ (tasks 17.12, 17.14)",
            "every tablet budget needs a run on the iPad Pro M4 (task 17.13)",
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
        "not_measured": not_measured,
    }
    arguments.output.parent.mkdir(parents=True, exist_ok=True)
    arguments.output.write_text(json.dumps(run, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"ok: wrote {len(measurements)} measurements to {arguments.output}")
    for note in not_measured:
        print(f"not measured: {note}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
