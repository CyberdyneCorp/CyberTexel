#!/usr/bin/env python3
"""Run examples against the newest wheel produced by the binding test."""

from __future__ import annotations

import shutil
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def main() -> int:
    uv = shutil.which("uv")
    if uv is None:
        print("uv is required to run the installed-wheel examples", file=sys.stderr)
        return 2
    wheels = sorted((ROOT / "build" / "python-dist").glob("cybertexel-*.whl"))
    if len(wheels) != 1:
        print(f"expected exactly one built CyberTexel wheel, found {len(wheels)}", file=sys.stderr)
        return 2
    arguments = sys.argv[1:] or ["compare"]
    return subprocess.run(
        [
            uv,
            "run",
            "--no-project",
            "--with",
            str(wheels[0]),
            "--",
            "python",
            str(ROOT / "examples" / "run_all.py"),
            *arguments,
        ],
        cwd=ROOT,
        check=False,
    ).returncode


if __name__ == "__main__":
    raise SystemExit(main())
