#!/usr/bin/env python3
"""Publish painted tiles to a host by revision, then read them back explicitly.

Capabilities: host-transport, paint-engine, texture-document, c-abi.
A host that owns the GPU never receives pixels implicitly. It asks what changed
since a revision cursor, pins the answer as a budgeted snapshot, reads the tile
memory layout it must upload into, and completes an asynchronous readback with
the exact tile payloads. This example does all of that with no device, and shows
that a pending readback publishes nothing until the host returns its tiles.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path

import cybertexel
import numpy as np


CAPABILITIES = ("host-transport", "paint-engine", "texture-document", "c-abi")

SEMANTIC = "pbr.base_color"
PIXELS = ((3, 5, b"\xd0\x40\x20"), (40, 12, b"\x20\xa0\xd0"))


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--executor", default="cpu")
    arguments = parser.parse_args()

    with cybertexel.Document() as document:
        texture_set = document.create_texture_set(
            "Body", partition_key="body", width=64, height=64
        )
        document.set_channel_enabled(texture_set, SEMANTIC)

        with cybertexel.SnapshotPool(budget_bytes=4 << 20) as pool:
            # A cursor taken before painting is what the host already holds.
            before = pool.current_cursor(document, texture_set, SEMANTIC)

            # Each write runs one isolated preview session: write, finalize
            # against its coverage, then commit as the published result.
            for x, y, colour in PIXELS:
                document.write_channel_pixel(texture_set, SEMANTIC, x, y, colour)

            after = pool.current_cursor(document, texture_set, SEMANTIC)
            assert after.revision > before.revision, (before, after)

            snapshot = pool.snapshot(document, texture_set, SEMANTIC, since=before)
            assert snapshot.cursor.epoch == after.epoch
            assert snapshot.cursor.revision == after.revision

            readback = snapshot.begin_host_readback()
            assert readback.status is cybertexel.ReadbackStatus.PENDING
            sizes = readback.tile_byte_sizes
            assert sizes and all(size > 0 for size in sizes)

            # A pending readback owns no caller-visible bytes yet.
            try:
                _ = readback.tiles
            except RuntimeError as error:
                pending_refusal = str(error)
            else:  # pragma: no cover - the contract forbids reaching this
                raise AssertionError("a pending readback must not publish tiles")

            # The host returns exactly the payloads the layout asked for.
            channel = document.read_channel(texture_set, SEMANTIC)
            readback.complete([bytes(size) for size in sizes])
            assert readback.status is cybertexel.ReadbackStatus.COMPLETE
            tiles = readback.tiles
            assert [len(tile) for tile in tiles] == list(sizes)
            readback.close()
            snapshot.close()

        # A failed call raises a typed exception carrying the native code and
        # the English diagnostic, rather than returning an error value.
        try:
            document.read_channel(texture_set, "no_such_channel")
        except cybertexel.CyberTexelError as error:
            diagnostic = {"code": error.diagnostic_code, "message": str(error)}
        else:  # pragma: no cover - the contract forbids reaching this
            raise AssertionError("an unknown channel must raise")
        assert diagnostic["message"]

        for x, y, colour in PIXELS:
            np.testing.assert_array_equal(channel[y, x], np.frombuffer(colour, np.uint8))

    arguments.output.mkdir(parents=True, exist_ok=True)
    (arguments.output / "host_transport.json").write_text(
        json.dumps(
            {
                "capabilities": list(CAPABILITIES),
                "cursor_before": [before.epoch, before.revision],
                "cursor_after": [after.epoch, after.revision],
                "tile_count": len(sizes),
                "tile_byte_sizes": list(sizes),
                "pending_readback_refused": bool(pending_refusal),
                "diagnostic_code": diagnostic["code"],
            },
            indent=2,
            sort_keys=True,
        )
        + "\n",
        encoding="utf-8",
    )


if __name__ == "__main__":
    main()
