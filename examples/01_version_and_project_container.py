#!/usr/bin/env python3
"""Create a canonical project container through the C ABI.

Capabilities: build-packaging, c-abi, examples, language-bindings, project-io.
The script asserts the version/header contract and writes deterministic bytes
plus a machine-readable summary for the committed-output comparison.
"""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path

import cybertexel


CAPABILITIES = (
    "build-packaging",
    "c-abi",
    "examples",
    "language-bindings",
    "project-io",
)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--executor", default="cpu")
    arguments = parser.parse_args()
    assert arguments.executor == "cpu", "this storage example is executor-independent"

    capi = cybertexel.capi
    version = capi.ctex_get_version()
    required = capi.c_size_t()
    assert (
        capi.ctex_project_container_create_empty(None, 0, capi.byref(required))
        == capi.CTEX_RESULT_SUCCESS
    )
    encoded = capi.create_string_buffer(required.value)
    assert (
        capi.ctex_project_container_create_empty(encoded, len(encoded), capi.byref(required))
        == capi.CTEX_RESULT_SUCCESS
    )
    payload = encoded.raw[: required.value]
    assert payload[:8] == b"CTEXPRJ\0"
    assert int.from_bytes(payload[8:12], "little") == 40
    assert tuple(map(int, cybertexel.__version__.split("."))) == (
        version.major,
        version.minor,
        version.patch,
    )

    arguments.output.mkdir(parents=True, exist_ok=True)
    (arguments.output / "empty.ctex").write_bytes(payload)
    summary = {
        "capabilities": list(CAPABILITIES),
        "container_bytes": len(payload),
        "container_sha256": hashlib.sha256(payload).hexdigest(),
        "executor": arguments.executor,
        "version": cybertexel.__version__,
    }
    (arguments.output / "summary.json").write_text(
        json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )


if __name__ == "__main__":
    main()
