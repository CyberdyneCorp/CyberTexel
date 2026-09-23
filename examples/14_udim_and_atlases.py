#!/usr/bin/env python3
"""Address sparse UDIM tiles in absolute UV and group texture sets into an atlas.

Capabilities: mesh-and-texture-sets.
UDIM storage is sparse: a tile is logically occupied when declared and allocated
when something is written into it, and the two are reported separately. Reading
an undeclared tile returns the channel default without occupying it. A write
batch is addressed in absolute UV, so one batch can cross a tile border. An
atlas groups existing texture sets into validated, non-overlapping regions that
export as one deterministic output.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path

import cybertexel


CAPABILITIES = ("mesh-and-texture-sets",)

SEMANTIC = "pbr.base_color"
RED = b"\xc8\x20\x20"
BLUE = b"\x20\x40\xc8"


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--executor", default="cpu")
    arguments = parser.parse_args()

    with cybertexel.Document() as document:
        tiled = document.create_texture_set(
            "Character", partition_key="character", width=32, height=32, udim=True
        )
        document.set_channel_enabled(tiled, SEMANTIC)

        # 1001 is the first standard tile; 1002 is the one to its right.
        newly_allocated = document.ensure_udim_tiles(tiled, [1001, 1002])
        assert document.udim_tiles(tiled) == [1001, 1002]

        # Absolute UV: u below 1.0 lands in 1001, u above 1.0 lands in 1002.
        # Texel addressing is top-left origin, so v maps directly to y.
        report = document.write_udim_pixels(
            tiled,
            SEMANTIC,
            [(0.25, 0.25, RED), (1.75, 0.25, BLUE)],
        )
        assert report.changed_pixel_count == 2, report
        assert report.changed_tile_count >= 1, report

        first = document.read_udim_pixel(tiled, SEMANTIC, 1001, 8, 8)
        second = document.read_udim_pixel(tiled, SEMANTIC, 1002, 24, 8)
        assert first == RED, first
        assert second == BLUE, second

        # Sparse means an undeclared tile reads as the channel default and
        # does not become occupied by being read.
        undeclared = document.read_udim_pixel(tiled, SEMANTIC, 1099, 0, 0)
        assert undeclared == b"\x80\x80\x80", undeclared
        assert document.udim_tiles(tiled) == [1001, 1002]

        # An atlas groups existing sets into non-overlapping regions.
        left = document.create_texture_set(
            "Left", partition_key="left", width=64, height=64
        )
        right = document.create_texture_set(
            "Right", partition_key="right", width=64, height=64
        )
        document.create_atlas(
            "atlas.body",
            "Body atlas",
            width=128,
            height=64,
            regions=[
                cybertexel.AtlasRegionPlacement(left.identifier, 0, 0, 64, 64),
                cybertexel.AtlasRegionPlacement(right.identifier, 64, 0, 64, 64),
            ],
        )
        assert document.atlas_ids() == ["atlas.body"], document.atlas_ids()
        atlas = document.atlas("atlas.body")
        assert (atlas.width, atlas.height) == (128, 64)
        assert {region.texture_set_id for region in atlas.regions} == {
            left.identifier,
            right.identifier,
        }

        # Overlapping regions are refused before the atlas exists.
        try:
            document.create_atlas(
                "atlas.bad",
                "Overlapping",
                width=128,
                height=64,
                regions=[
                    cybertexel.AtlasRegionPlacement(left.identifier, 0, 0, 64, 64),
                    cybertexel.AtlasRegionPlacement(right.identifier, 32, 0, 64, 64),
                ],
            )
        except cybertexel.CyberTexelError as error:
            overlap_refusal = str(error)
        else:  # pragma: no cover - the contract forbids reaching this
            raise AssertionError("overlapping atlas regions must be refused")
        assert document.atlas_ids() == ["atlas.body"]

    arguments.output.mkdir(parents=True, exist_ok=True)
    (arguments.output / "udim_and_atlases.json").write_text(
        json.dumps(
            {
                "capabilities": list(CAPABILITIES),
                "newly_allocated_tiles": newly_allocated,
                "occupied_tiles": [1001, 1002],
                "changed_pixel_count": report.changed_pixel_count,
                "changed_tile_count": report.changed_tile_count,
                "tile_1001_texel": list(first),
                "tile_1002_texel": list(second),
                "undeclared_tile_reads_default": list(undeclared),
                "atlas": {
                    "display_name": atlas.display_name,
                    "width": atlas.width,
                    "height": atlas.height,
                    "region_count": len(atlas.regions),
                },
                "overlap_refused": bool(overlap_refusal),
            },
            indent=2,
            sort_keys=True,
        )
        + "\n",
        encoding="utf-8",
    )


if __name__ == "__main__":
    main()
