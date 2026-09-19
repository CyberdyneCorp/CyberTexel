#!/usr/bin/env python3

import json
import pathlib
import sys


def main() -> int:
    report = json.loads(pathlib.Path(sys.argv[1]).read_text(encoding="utf-8"))
    assert report["preset"] == "test"
    assert report["dry_run"] is True
    assert report["cancelled"] is False
    assert len(report["outputs"]) == 1
    output = report["outputs"][0]
    required = {
        "path",
        "texture_sets",
        "udim",
        "atlas",
        "preset_entry",
        "width",
        "height",
        "format",
        "color_space",
        "bit_depth",
        "jpeg_quality",
        "estimated_size_bytes",
        "encoded_size_bytes",
        "state",
    }
    assert required <= output.keys()
    assert output["state"] == "planned"
    assert output["encoded_size_bytes"] is None
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
