#!/usr/bin/env python3
"""Regenerate the deterministic PNG fixtures without external dependencies."""

from __future__ import annotations

import struct
import zlib
from pathlib import Path


ROOT = Path(__file__).resolve().parent


def chunk(kind: bytes, payload: bytes) -> bytes:
    return (
        struct.pack(">I", len(payload))
        + kind
        + payload
        + struct.pack(">I", zlib.crc32(kind + payload) & 0xFFFFFFFF)
    )


def write_png(path: Path, width: int, height: int, channels: int, pixels: bytes) -> None:
    if channels not in (1, 3) or len(pixels) != width * height * channels:
        raise ValueError("fixture pixel dimensions are inconsistent")
    rows = b"".join(
        b"\0" + pixels[row * width * channels : (row + 1) * width * channels]
        for row in range(height)
    )
    header = struct.pack(">IIBBBBB", width, height, 8, 0 if channels == 1 else 2, 0, 0, 0)
    encoded = (
        b"\x89PNG\r\n\x1a\n"
        + chunk(b"IHDR", header)
        + chunk(b"IDAT", zlib.compress(rows, level=9))
        + chunk(b"IEND", b"")
    )
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(encoded)


def rgb_pixels(width: int, height: int, sample) -> bytes:
    return bytes(component for y in range(height) for x in range(width) for component in sample(x, y))


def gray_pixels(width: int, height: int, sample) -> bytes:
    return bytes(sample(x, y) for y in range(height) for x in range(width))


def main() -> None:
    write_png(
        ROOT / "images" / "checker.png",
        8,
        8,
        3,
        rgb_pixels(
            8,
            8,
            lambda x, y: (232, 76, 61) if (x // 2 + y // 2) % 2 == 0 else (28, 42, 64),
        ),
    )
    write_png(
        ROOT / "alphas" / "soft_round.png",
        16,
        16,
        1,
        gray_pixels(
            16,
            16,
            lambda x, y: max(0, 255 - (((2 * x - 15) ** 2 + (2 * y - 15) ** 2) * 255 // 226)),
        ),
    )
    write_png(
        ROOT / "maps" / "ambient_occlusion.png",
        8,
        8,
        1,
        gray_pixels(8, 8, lambda x, y: 48 + ((x + y) * 207 // 14)),
    )
    write_png(
        ROOT / "maps" / "curvature.png",
        8,
        8,
        1,
        gray_pixels(8, 8, lambda x, y: 255 if x in (0, 7) or y in (0, 7) else 96),
    )
    write_png(
        ROOT / "maps" / "normal_opengl.png",
        8,
        8,
        3,
        rgb_pixels(8, 8, lambda x, y: (112 + 4 * x, 112 + 4 * y, 255)),
    )
    write_png(
        ROOT / "maps" / "position.png",
        8,
        8,
        3,
        rgb_pixels(8, 8, lambda x, y: (x * 255 // 7, y * 255 // 7, 128)),
    )


if __name__ == "__main__":
    main()
