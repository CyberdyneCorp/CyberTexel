#!/usr/bin/env python3
"""Require every numbered example and committed artifact in the gallery."""

from __future__ import annotations

import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def gallery_failures(root: Path = ROOT) -> list[str]:
    examples = root / "examples"
    gallery_path = root / "docs" / "gallery.md"
    if not gallery_path.is_file():
        return ["docs/gallery.md is missing"]
    gallery = gallery_path.read_text(encoding="utf-8")
    failures: list[str] = []
    for script in sorted(examples.glob("[0-9][0-9]_*.py")):
        if f"## `{script.stem}`" not in gallery:
            failures.append(f"gallery is missing example {script.stem}")
        output = examples / "outputs" / script.stem
        if not output.is_dir():
            failures.append(f"committed output is missing for {script.stem}")
            continue
        for artifact in sorted(path for path in output.rglob("*") if path.is_file()):
            relative = (Path("..") / artifact.relative_to(root)).as_posix()
            if relative not in gallery:
                failures.append(f"gallery is missing artifact {artifact.relative_to(root)}")
    return failures


def main() -> int:
    failures = gallery_failures()
    if failures:
        print("example gallery check failed:", file=sys.stderr)
        for failure in failures:
            print(f"- {failure}", file=sys.stderr)
        return 1
    print("example gallery check passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
