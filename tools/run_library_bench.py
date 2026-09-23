#!/usr/bin/env python3
"""Run the library benchmark against the newest built wheel."""

from __future__ import annotations

import shutil
import subprocess
import sys
from datetime import date
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def main() -> int:
    uv = shutil.which("uv")
    if uv is None:
        print("uv is required to run the installed-wheel benchmark", file=sys.stderr)
        return 2
    wheels = sorted((ROOT / "build" / "python-dist").glob("cybertexel-*.whl"))
    if len(wheels) != 1:
        print(f"expected exactly one built wheel, found {len(wheels)}", file=sys.stderr)
        return 2
    arguments = sys.argv[1:]
    if "--output" not in arguments:
        device = arguments[arguments.index("--device") + 1] if "--device" in arguments else "run"
        destination = ROOT / "benchmarks" / "results" / f"{date.today().isoformat()}-{device}.json"
        arguments += ["--output", str(destination)]
    return subprocess.run(
        [
            uv, "run", "--no-project", "--with", str(wheels[0]), "--with", "numpy",
            "--", "python", str(ROOT / "benchmarks" / "library_bench.py"), *arguments,
        ],
        cwd=ROOT,
        check=False,
    ).returncode


if __name__ == "__main__":
    raise SystemExit(main())
