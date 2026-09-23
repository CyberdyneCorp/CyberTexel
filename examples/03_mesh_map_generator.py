#!/usr/bin/env python3
"""Bind a mesh map to a texture set and evaluate a deterministic mask.

Capabilities: mesh-and-texture-sets, mesh-maps, texture-document.
The script uses the checked UV quad and ambient-occlusion fixtures, asserts the
document and generator results, and publishes the generated mask as a PNG.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path

import cybertexel
import numpy as np


ROOT = Path(__file__).resolve().parent
CAPABILITIES = ("mesh-and-texture-sets", "mesh-maps", "texture-document")


def fixture_mesh() -> tuple[np.ndarray, np.ndarray, np.ndarray, np.ndarray]:
    positions: list[list[float]] = []
    uv: list[list[float]] = []
    triangles: list[list[int]] = []
    path = ROOT / "fixtures" / "meshes" / "quad_uv.obj"
    for line in path.read_text(encoding="utf-8").splitlines():
        fields = line.split()
        if fields[:1] == ["v"]:
            positions.append([float(value) for value in fields[1:4]])
        elif fields[:1] == ["vt"]:
            uv.append([float(value) for value in fields[1:3]])
        elif fields[:1] == ["f"]:
            triangles.append([int(value.split("/")[0]) - 1 for value in fields[1:4]])
    normals = [[0.0, 0.0, 1.0]] * len(positions)
    return (
        np.asarray(positions, dtype=np.float32),
        np.asarray(triangles, dtype=np.uint32),
        np.asarray(normals, dtype=np.float32),
        np.asarray(uv, dtype=np.float32),
    )


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--executor", default="cpu")
    arguments = parser.parse_args()

    positions, triangles, normals, uv = fixture_mesh()
    ao_path = ROOT / "fixtures" / "maps" / "ambient_occlusion.png"
    ao = cybertexel.decode_image(
        ao_path.read_bytes(),
        source_name=ao_path.name,
        intended_channel=cybertexel.ChannelSemantic.OCCLUSION,
    ).pixels[:, :, 0]

    with (
        cybertexel.Document() as document,
        cybertexel.Mesh(positions, triangles, normals=normals, uv=uv) as mesh,
    ):
        texture_set = document.create_texture_set(
            "Fixture Quad", partition_key="body", width=32, height=32
        )
        document.set_channel_enabled(texture_set, "pbr.base_color")
        base_color = document.read_channel(texture_set, "pbr.base_color")
        assert document.texture_set_ids() == [texture_set.identifier]
        assert base_color.shape == (32, 32, 3)
        np.testing.assert_array_equal(base_color[0, 0], [128, 128, 128])
        assert np.count_nonzero(base_color != 128) == 0
        with cybertexel.MeshMapSet(document, texture_set, mesh) as maps:
            report = maps.import_map(cybertexel.MeshMapKind.AMBIENT_OCCLUSION, ao)
            mask = maps.generate_mask(
                cybertexel.MeshMapGeneratorKind.AMBIENT_OCCLUSION, 32, 32
            )

    assert report.resolution_mismatch
    assert (report.map_width, report.map_height) == (8, 8)
    assert mask.shape == (32, 32) and mask.dtype == np.float32
    np.testing.assert_allclose(
        [mask[0, 0], mask[16, 16], mask[31, 31], mask.mean()],
        [0.7997080, 0.3949755, 0.01286765, 0.4078235],
        rtol=0.0,
        atol=1e-7,
    )

    preview = np.rint(mask * 255.0).astype(np.uint8)
    encoded = cybertexel.encode_image(
        preview, color_space=cybertexel.ColorSpace.LINEAR_REC709
    )
    arguments.output.mkdir(parents=True, exist_ok=True)
    (arguments.output / "ambient_occlusion_mask.png").write_bytes(encoded)
    summary = {
        "capabilities": list(CAPABILITIES),
        "import_resolution_mismatch": report.resolution_mismatch,
        "map_shape": list(ao.shape),
        "mask_maximum": float(mask.max()),
        "mask_mean": float(mask.mean()),
        "mask_minimum": float(mask.min()),
        "mask_shape": list(mask.shape),
        "mesh_triangles": int(triangles.shape[0]),
        "mesh_vertices": int(positions.shape[0]),
    }
    (arguments.output / "summary.json").write_text(
        json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )


if __name__ == "__main__":
    main()
