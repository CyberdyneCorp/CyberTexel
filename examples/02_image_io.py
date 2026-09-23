#!/usr/bin/env python3
"""Decode, inspect, resize and encode a fixture image through Python.

Capabilities: image-io, language-bindings.
The enlarged checker is a visual output while the assertions pin decoded pixel
values, channel statistics and the encoded round trip.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path

import cybertexel
import numpy as np


ROOT = Path(__file__).resolve().parent
CAPABILITIES = ("image-io", "language-bindings")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--executor", default="cpu")
    arguments = parser.parse_args()

    source = ROOT / "fixtures" / "images" / "checker.png"
    decoded = cybertexel.decode_image(source.read_bytes(), source_name=source.name)
    assert decoded.pixels.shape == (8, 8, 3)
    assert decoded.pixels.dtype == np.uint8
    np.testing.assert_array_equal(decoded.pixels[0, 0], [232, 76, 61])
    np.testing.assert_array_equal(decoded.pixels[0, 2], [28, 42, 64])
    np.testing.assert_allclose(decoded.pixels.mean(axis=(0, 1)), [130.0, 59.0, 62.5])

    preview = np.repeat(np.repeat(decoded.pixels, 16, axis=0), 16, axis=1)
    encoded = cybertexel.encode_image(preview)
    round_trip = cybertexel.decode_image(encoded, source_name="checker_preview.png")
    np.testing.assert_array_equal(round_trip.pixels, preview)

    arguments.output.mkdir(parents=True, exist_ok=True)
    (arguments.output / "checker_preview.png").write_bytes(encoded)
    summary = {
        "capabilities": list(CAPABILITIES),
        "channel_mean": [130.0, 59.0, 62.5],
        "decoded_shape": list(decoded.pixels.shape),
        "preview_shape": list(preview.shape),
    }
    (arguments.output / "summary.json").write_text(
        json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )


if __name__ == "__main__":
    main()
