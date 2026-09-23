#!/usr/bin/env python3
"""Build a validated layer stack and edit it as an ordered document.

Capabilities: texture-document.
Entries declare their kind explicitly — paint, group, mask — and a batch is
validated against the resulting stack before any of it is published. Channel
participation and opacity are per entry, and removing an entry other entries
reference is the caller's explicit choice rather than a hidden rule.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path

import cybertexel


CAPABILITIES = ("texture-document",)

BASE = "pbr.base_color"
ROUGHNESS = "pbr.roughness"


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--executor", default="cpu")
    arguments = parser.parse_args()

    with cybertexel.Document() as document:
        texture_set = document.create_texture_set(
            "Body", partition_key="body", width=64, height=64
        )
        for semantic in (BASE, ROUGHNESS):
            document.set_channel_enabled(texture_set, semantic)

        # A custom semantic uses the same descriptor as the built-in preset.
        document.register_channel(
            texture_set,
            cybertexel.TextureChannel(
                semantic_id="studio.wear_mask",
                component_count=1,
                preferred_bit_depth=8,
                default_value=(0.0,),
                export_mapping="R",
            ),
        )
        channels = document.channel_ids(texture_set)
        assert BASE in channels and "studio.wear_mask" in channels, channels

        # One ordered batch: a group, a paint entry inside it, and a mask
        # attached to that entry.
        document.append_layers(
            texture_set,
            [
                cybertexel.LayerEntry(
                    identifier="group.metal",
                    display_name="Metal",
                    kind=cybertexel.LayerKind.GROUP,
                    opacity=0.75,
                ),
                cybertexel.LayerEntry(
                    identifier="paint.rust",
                    display_name="Rust",
                    kind=cybertexel.LayerKind.PAINT,
                    parent_identifier="group.metal",
                    opacity=0.5,
                    blend_mode="multiply",
                    channels=(
                        cybertexel.LayerChannel(BASE, enabled=True, opacity=1.0),
                        cybertexel.LayerChannel(ROUGHNESS, enabled=False),
                    ),
                ),
                cybertexel.LayerEntry(
                    identifier="mask.edges",
                    display_name="Edges",
                    kind=cybertexel.LayerKind.MASK,
                    target_identifier="paint.rust",
                ),
            ],
        )

        stack = document.inspect_layers(texture_set)
        entries = {entry["id"]: entry for entry in stack["entries"]}
        assert set(entries) == {"group.metal", "paint.rust", "mask.edges"}, sorted(entries)
        assert entries["paint.rust"]["parent"] == "group.metal"
        assert entries["mask.edges"]["target"] == "paint.rust"
        revision = stack["revision"]

        # An invalid batch changes nothing: the parent does not exist.
        try:
            document.append_layers(
                texture_set,
                [
                    cybertexel.LayerEntry(
                        identifier="paint.orphan",
                        display_name="Orphan",
                        parent_identifier="group.absent",
                    )
                ],
            )
        except cybertexel.CyberTexelError as error:
            refusal = str(error)
        else:  # pragma: no cover - the contract forbids reaching this
            raise AssertionError("an entry naming an absent parent must be refused")
        assert document.inspect_layers(texture_set)["revision"] == revision

        document.set_layer_state(
            texture_set,
            "paint.rust",
            display_name="Rust and grime",
            enabled=False,
            opacity=0.25,
            blend_mode="overlay",
        )
        edited = {
            entry["id"]: entry
            for entry in document.inspect_layers(texture_set)["entries"]
        }["paint.rust"]
        assert edited["display_name"] == "Rust and grime"
        assert edited["enabled"] is False
        assert edited["blend_mode"] == "overlay"

        document.remove_layers(texture_set, ["mask.edges"])
        remaining = [
            entry["id"] for entry in document.inspect_layers(texture_set)["entries"]
        ]
        assert "mask.edges" not in remaining, remaining

    arguments.output.mkdir(parents=True, exist_ok=True)
    (arguments.output / "layer_stack.json").write_text(
        json.dumps(
            {
                "capabilities": list(CAPABILITIES),
                "channels": channels,
                "entries_after_append": sorted(entries),
                "invalid_batch_refused": bool(refusal),
                "edited_blend_mode": edited["blend_mode"],
                "entries_after_removal": remaining,
            },
            indent=2,
            sort_keys=True,
        )
        + "\n",
        encoding="utf-8",
    )


if __name__ == "__main__":
    main()
