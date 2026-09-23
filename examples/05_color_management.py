#!/usr/bin/env python3
"""Inspect working-space policy and visualize deterministic colour conversion.

Capabilities: color-management.
The output compares an encoded sRGB ramp with its linear Rec. 709 values and a
deterministically dithered half-intensity strip.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path

import cybertexel
import numpy as np


CAPABILITIES = ("color-management",)


def convert_gray(value: float) -> float:
    capi = cybertexel.capi
    encoded = capi.ctex_rgb_color(
        value, value, value, capi.CTEX_COLOR_SPACE_SRGB_REC709
    )
    linear = capi.ctex_rgb_color()
    assert (
        capi.ctex_color_convert(
            capi.byref(encoded), capi.CTEX_COLOR_SPACE_LINEAR_REC709, capi.byref(linear)
        )
        == capi.CTEX_RESULT_SUCCESS
    )
    return float(linear.red)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--executor", default="cpu")
    arguments = parser.parse_args()
    assert arguments.executor == "cpu", "colour conversion is executor-independent"

    capi = cybertexel.capi
    linear = np.array([convert_gray(value / 255.0) for value in range(256)])
    assert capi.ctex_get_working_color_space() == capi.CTEX_COLOR_SPACE_LINEAR_REC709
    np.testing.assert_allclose(linear[[0, 128, 255]], [0.0, 0.2158605001, 1.0], atol=1e-10)

    normal_policy = capi.ctex_channel_color_policy()
    normal_policy.size = capi.CTEX_CHANNEL_COLOR_POLICY_CURRENT_SIZE
    assert (
        capi.ctex_channel_get_color_policy(
            capi.CTEX_CHANNEL_SEMANTIC_NORMAL, capi.byref(normal_policy)
        )
        == capi.CTEX_RESULT_SUCCESS
    )
    assert (normal_policy.color_valued, normal_policy.recommended_bit_depth) == (0, 16)

    contributions = (capi.c_double * 100)(*[0.001] * 100)
    accumulated = capi.c_double()
    assert (
        capi.ctex_accumulate_height(contributions, 100, 8, capi.byref(accumulated))
        == capi.CTEX_RESULT_SUCCESS
    )
    assert abs(accumulated.value - (26.0 / 255.0)) < 1e-12

    preview = np.empty((96, 256, 3), dtype=np.uint8)
    preview[:32, :, :] = np.arange(256, dtype=np.uint8)[None, :, None]
    preview[32:64, :, :] = np.rint(linear * 255.0).astype(np.uint8)[None, :, None]
    for y in range(64, 96):
        for x in range(256):
            quantized = capi.uint8_t()
            assert (
                capi.ctex_quantize_unorm8(0.5, x, y, 1, capi.byref(quantized))
                == capi.CTEX_RESULT_SUCCESS
            )
            preview[y, x, :] = quantized.value
    assert set(np.unique(preview[64:]).tolist()) == {127, 128}

    arguments.output.mkdir(parents=True, exist_ok=True)
    (arguments.output / "transfer_and_dither.png").write_bytes(
        cybertexel.encode_image(preview)
    )
    summary = {
        "accumulated_height": accumulated.value,
        "capabilities": list(CAPABILITIES),
        "dither_values": sorted(np.unique(preview[64:]).tolist()),
        "linear_midpoint": float(linear[128]),
        "normal_recommended_bit_depth": normal_policy.recommended_bit_depth,
        "working_color_space": "Linear Rec. 709",
    }
    (arguments.output / "summary.json").write_text(
        json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )


if __name__ == "__main__":
    main()
