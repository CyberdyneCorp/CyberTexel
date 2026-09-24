#!/usr/bin/env python3
"""Read the delivery note for the shader backend CyberTexel actually ships.

Capabilities: shader-emission.
The library vendors ArmorPaint's Kongruent-derived compiler. A studio shipping a
binary has to be able to state which compiler produced its shaders, under which
licence and at which pinned revision, without reading the build tree. The note
is published through the same two-call sizing contract as every other report,
and its reported size is exact rather than an upper bound.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path

import cybertexel

CAPABILITIES = ("shader-emission",)
CAPI = cybertexel.capi
byref, buffer = CAPI.byref, CAPI.create_string_buffer
OK = CAPI.CTEX_RESULT_SUCCESS


def ok(result: int) -> None:
    assert result == OK, f"the C ABI refused with result {result}"


def fill(name: str, **fields: object) -> object:
    value = getattr(CAPI, f"ctex_{name}")()
    value.size = getattr(CAPI, f"CTEX_{name.upper()}_CURRENT_SIZE")
    for field, supplied in fields.items():
        setattr(value, field, supplied)
    return value


def buffers(*sizes: int) -> list[object]:
    return [buffer(size) for size in sizes]


def backend_attribution() -> dict[str, object]:
    """The delivery note records the shader backend the studio really ships."""
    info = fill("shader_backend_attribution_info")
    ok(CAPI.ctex_shader_get_backend_attribution(byref(info), None, 0))
    report, = buffers(info.report_size)
    ok(CAPI.ctex_shader_get_backend_attribution(byref(info), report, info.report_size))
    assert len(report.value) + 1 == info.report_size, "the published report size was not exact"
    attribution = json.loads(report.value)
    assert attribution["name"] == "Kongruent minikong" and attribution["license"] == "Zlib"
    assert attribution["license_file"] == "thirdparty/licenses/kongruent.txt"
    assert len(attribution["revision"]) == len(attribution["upstream_revision"]) == 40
    assert attribution["revision"] in attribution["source"], "source misses the revision"
    return attribution


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--executor", default="cpu")
    arguments = parser.parse_args()
    attribution = backend_attribution()
    arguments.output.mkdir(parents=True, exist_ok=True)
    (arguments.output / "backend_note.json").write_text(
        json.dumps({"capabilities": list(CAPABILITIES), "backend": attribution},
                   indent=2, sort_keys=True) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
