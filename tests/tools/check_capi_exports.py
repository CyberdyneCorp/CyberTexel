#!/usr/bin/env python3

from __future__ import annotations

import subprocess
import sys
from pathlib import Path


EXPECTED = {
    "ctex_accumulate_height",
    "ctex_channel_get_color_policy",
    "ctex_channel_get_bit_depth_warning",
    "ctex_color_convert",
    "ctex_color_input_to_working",
    "ctex_color_space_get_name",
    "ctex_cube_lut_apply_preview",
    "ctex_cube_lut_create",
    "ctex_cube_lut_destroy",
    "ctex_document_create",
    "ctex_document_create_texture_set",
    "ctex_document_destroy",
    "ctex_document_get_texture_set_ids",
    "ctex_get_abi_version",
    "ctex_get_last_diagnostic",
    "ctex_get_last_result",
    "ctex_get_version",
    "ctex_get_working_color_space",
    "ctex_quantize_unorm8",
    "ctex_resolve_input_color_space",
    "ctex_texture_set_get_channel_ids",
    "ctex_texture_set_get_channel_info",
    "ctex_texture_set_get_memory_report",
    "ctex_texture_set_register_channel",
    "ctex_texture_set_set_channel_enabled",
}


def main() -> int:
    if len(sys.argv) != 3:
        print("usage: check_capi_exports.py LIBRARY NM", file=sys.stderr)
        return 2
    library = Path(sys.argv[1])
    completed = subprocess.run(
        [sys.argv[2], "-D", "--defined-only", "-g", str(library)],
        check=True,
        capture_output=True,
        text=True,
    )
    exports = {
        line.split()[-1].split("@")[0]
        for line in completed.stdout.splitlines()
        if line.split()
    }
    unexpected = sorted(name for name in exports if not name.startswith("ctex_"))
    missing = sorted(EXPECTED - exports)
    if unexpected:
        print(f"non-ctex exports: {', '.join(unexpected)}")
    if missing:
        print(f"missing C ABI exports: {', '.join(missing)}")
    if unexpected or missing:
        return 1
    print(f"ok: {len(exports)} shared-library exports, all use the ctex_ prefix")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
