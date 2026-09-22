#!/usr/bin/env python3
"""Run the deterministic in-memory image decoder libFuzzer CI gate."""

from __future__ import annotations

import argparse
import struct
import subprocess
import tempfile
import zlib
from pathlib import Path


def png_seed() -> bytes:
    def chunk(kind: bytes, payload: bytes) -> bytes:
        body = kind + payload
        return struct.pack(">I", len(payload)) + body + struct.pack(">I", zlib.crc32(body))

    header = struct.pack(">IIBBBBB", 1, 1, 8, 2, 0, 0, 0)
    return b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", header) + chunk(
        b"IDAT", zlib.compress(b"\x00\x00\x00\x00")
    ) + chunk(b"IEND", b"")


def write_corpus(directory: Path) -> None:
    tga_header = struct.pack("<BBBHHBHHHHBB", 0, 0, 2, 0, 0, 0, 0, 0, 1, 1, 24, 0)
    seeds = {
        "empty": b"",
        "png-valid": png_seed(),
        "jpeg-signature": b"\xff\xd8\xff\xe0\x00\x10JFIF\x00\xff\xd9",
        "bmp-signature": b"BM" + bytes(52),
        "tiff-little": b"II*\x00" + bytes(12),
        "tiff-big": b"MM\x00*" + bytes(12),
        "openexr-signature": b"\x76\x2f\x31\x01" + bytes(16),
        "radiance-signature": b"#?RADIANCE\nFORMAT=32-bit_rle_rgbe\n\n-Y 1 +X 1\n",
        "psd-signature": b"8BPS\x00\x01" + bytes(20),
        "tga-valid": tga_header + bytes(3),
    }
    for name, contents in seeds.items():
        (directory / name).write_bytes(contents)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("fuzzer", type=Path)
    parser.add_argument("--runs", type=int, default=20_000)
    parser.add_argument(
        "--artifact-dir",
        type=Path,
        default=Path("build/fuzz-artifacts/image-decode"),
    )
    args = parser.parse_args()
    args.artifact_dir.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="ctex-image-fuzz-") as temporary:
        corpus = Path(temporary)
        write_corpus(corpus)
        command = [
            str(args.fuzzer.resolve()),
            str(corpus),
            f"-runs={args.runs}",
            "-seed=1337",
            "-max_len=1048576",
            "-rss_limit_mb=512",
            "-timeout=5",
            "-print_final_stats=1",
            f"-artifact_prefix={args.artifact_dir.resolve()}/",
        ]
        return subprocess.run(command, check=False).returncode


if __name__ == "__main__":
    raise SystemExit(main())
