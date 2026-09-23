#!/usr/bin/env python3
"""Query deterministic surface hits in UV space and visualize the result.

Capabilities: picking.
The example asserts both triangles of a UV quad, barycentric interpolation and
the distinct miss outcome exposed by the Python binding.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path

import cybertexel
import numpy as np


CAPABILITIES = ("picking",)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--executor", default="cpu")
    arguments = parser.parse_args()
    assert arguments.executor == "cpu", "picking uses the deterministic CPU index"

    positions = np.array(
        [[-1, -1, 0], [1, -1, 0], [1, 1, 0], [-1, 1, 0]], dtype=np.float32
    )
    triangles = np.array([[0, 1, 2], [0, 2, 3]], dtype=np.uint32)
    normals = np.array([[0, 0, 1]] * 4, dtype=np.float32)
    uv = np.array([[0, 0], [1, 0], [1, 1], [0, 1]], dtype=np.float32)
    with cybertexel.Mesh(
        positions,
        triangles,
        normals=normals,
        uv=uv,
        partition_key="body",
        partition_name="Body",
    ) as mesh:
        lower = mesh.pick_uv(0.75, 0.25)
        upper = mesh.pick_uv(0.25, 0.75)
        miss = mesh.pick_uv(1.25, 0.50)

    assert lower is not None and upper is not None and miss is None
    assert (lower.triangle_index, upper.triangle_index) == (0, 1)
    np.testing.assert_allclose(lower.position, [0.5, -0.5, 0.0])
    np.testing.assert_allclose(upper.position, [-0.5, 0.5, 0.0])
    np.testing.assert_allclose(lower.barycentric, [0.25, 0.5, 0.25])
    np.testing.assert_allclose(upper.barycentric, [0.25, 0.25, 0.5])
    assert lower.texture_set_id == upper.texture_set_id == "material/4:body/uv/3:uv0"

    preview = np.full((128, 128, 3), [34, 45, 66], dtype=np.uint8)
    for coordinate, color in (((0.75, 0.25), [237, 79, 63]), ((0.25, 0.75), [70, 190, 170])):
        x = round(coordinate[0] * 127)
        y = round((1.0 - coordinate[1]) * 127)
        preview[max(0, y - 5) : y + 6, max(0, x - 5) : x + 6] = color
    diagonal = np.arange(128)
    preview[127 - diagonal, diagonal] = [230, 230, 230]

    arguments.output.mkdir(parents=True, exist_ok=True)
    (arguments.output / "uv_hits.png").write_bytes(cybertexel.encode_image(preview))
    summary = {
        "capabilities": list(CAPABILITIES),
        "hit_texture_set": lower.texture_set_id,
        "lower_hit_position": list(lower.position),
        "lower_triangle": lower.triangle_index,
        "miss": None,
        "upper_hit_position": list(upper.position),
        "upper_triangle": upper.triangle_index,
    }
    (arguments.output / "summary.json").write_text(
        json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )


if __name__ == "__main__":
    main()
