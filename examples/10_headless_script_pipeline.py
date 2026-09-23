#!/usr/bin/env python3
"""Run a document-editing script through the headless Python bridge.

Capabilities: cli-headless.
The example creates a project, invokes the same isolated script runner used by
the CLI `run` command, and asserts diagnostics routing and the persisted edit.
"""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile

import cybertexel
import numpy as np


CAPABILITIES = ("cli-headless",)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--executor", default="cpu")
    arguments = parser.parse_args()
    assert arguments.executor == "cpu", "the headless example uses the CPU reference"

    with cybertexel.Document() as document:
        texture_set = document.create_texture_set(
            "Script target", partition_key="body", width=4, height=4
        )
        document.set_channel_enabled(texture_set, "pbr.base_color", bit_depth=8)
        source_bytes = document.to_project_bytes()

    with tempfile.TemporaryDirectory(prefix="ctex-headless-example-") as temporary:
        workspace = Path(temporary)
        source = workspace / "source.ctex"
        script = workspace / "edit.py"
        staged = workspace / "edited.ctex"
        source.write_bytes(source_bytes)
        script.write_text(
            "def main(document):\n"
            "    print('headless-script-progress')\n"
            "    target = document.texture_set_ids()[0]\n"
            "    document.set_channel_enabled(target, 'pbr.roughness', bit_depth=8)\n",
            encoding="utf-8",
        )
        completed = subprocess.run(
            [
                sys.executable,
                "-m",
                "cybertexel._script_runner",
                str(source),
                str(script),
                str(staged),
            ],
            capture_output=True,
            text=True,
            check=False,
        )
        assert completed.returncode == 0, completed.stderr
        assert completed.stdout == ""
        assert "headless-script-progress" in completed.stderr
        edited_bytes = staged.read_bytes()

    assert source_bytes != edited_bytes
    with cybertexel.Document.from_project(edited_bytes) as edited:
        identifiers = edited.texture_set_ids()
        assert identifiers == [texture_set.identifier]
        pixels = edited.read_channel(
            cybertexel.TextureSet(identifiers[0], 4, 4), "pbr.roughness"
        )
    assert pixels.shape == (4, 4, 1)
    assert pixels.dtype == np.uint8
    assert np.unique(pixels).tolist() == [128]

    arguments.output.mkdir(parents=True, exist_ok=True)
    (arguments.output / "scripted.ctex").write_bytes(edited_bytes)
    summary = {
        "capabilities": list(CAPABILITIES),
        "diagnostic": "headless-script-progress",
        "output_sha256": hashlib.sha256(edited_bytes).hexdigest(),
        "roughness_shape": list(pixels.shape),
        "roughness_value": int(pixels[0, 0, 0]),
        "source_sha256": hashlib.sha256(source_bytes).hexdigest(),
        "texture_set": texture_set.identifier,
    }
    (arguments.output / "summary.json").write_text(
        json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )


if __name__ == "__main__":
    main()
