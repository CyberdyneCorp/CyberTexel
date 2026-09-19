#!/usr/bin/env python3
"""Run the deterministic project-container libFuzzer CI gate."""

from __future__ import annotations

import argparse
import struct
import subprocess
import tempfile
from pathlib import Path


def valid_empty_container(version: tuple[int, int, int]) -> bytes:
    sections = b"".join(struct.pack("<IIQI", kind, 1, 4, 0) for kind in (1, 2, 3))
    header = b"CTEXPRJ\0" + struct.pack("<IIIIIIQ", 40, *version, 3, 0, len(sections))
    return header + sections


def write_corpus(directory: Path, version: tuple[int, int, int]) -> None:
    valid = valid_empty_container(version)
    seeds = {
        "empty": b"",
        "magic": b"CTEXPRJ\0",
        "header": valid[:40],
        "valid-empty": valid,
        "truncated-section": valid[:-1],
        "oversized-count": valid[:24] + b"\xff\xff\xff\xff" + valid[28:],
    }
    for name, contents in seeds.items():
        (directory / name).write_bytes(contents)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("fuzzer", type=Path)
    parser.add_argument("--runs", type=int, default=20_000)
    args = parser.parse_args()
    version = tuple(int(part) for part in Path("VERSION").read_text().strip().split("."))
    with tempfile.TemporaryDirectory(prefix="ctex-project-fuzz-") as temporary:
        corpus = Path(temporary)
        write_corpus(corpus, version)
        command = [
            str(args.fuzzer.resolve()),
            str(corpus),
            f"-runs={args.runs}",
            "-seed=1337",
            "-max_len=1048576",
            "-rss_limit_mb=512",
            "-timeout=5",
            "-print_final_stats=1",
        ]
        return subprocess.run(command, check=False).returncode


if __name__ == "__main__":
    raise SystemExit(main())
