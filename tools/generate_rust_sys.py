#!/usr/bin/env python3
"""Regenerate the committed Rust -sys surface from the public C header."""

from __future__ import annotations

import hashlib
import os
import shutil
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "include" / "ctex" / "capi.h"
OUTPUT = ROOT / "rust" / "cybertexel-sys" / "src" / "lib.rs"
RESULT_TYPE = "pub type ctex_result = ::std::os::raw::c_uint;"
RESULT_ALIAS = (
    RESULT_TYPE
    + "\n"
    + "pub const CTEX_RESULT_SUCCESS: ctex_result = ctex_result_CTEX_RESULT_SUCCESS;"
)


def bindgen_path() -> str:
    executable = shutil.which("bindgen")
    if executable is None:
        raise RuntimeError(
            "bindgen CLI is required to regenerate Rust declarations; "
            "install bindgen-cli 0.72.1"
        )
    return executable


def generate() -> str:
    environment = os.environ.copy()
    environment.setdefault("LIBCLANG_PATH", "/Library/Developer/CommandLineTools/usr/lib")
    command = [
        bindgen_path(),
        str(HEADER),
        "--allowlist-function",
        "^ctex_.*",
        "--allowlist-type",
        "^ctex_.*",
        "--allowlist-var",
        "^CTEX_.*",
        "--with-derive-default",
        "--no-layout-tests",
        "--disable-header-comment",
        "--formatter",
        "rustfmt",
        "--",
        f"-I{ROOT / 'include'}",
    ]
    generated = subprocess.run(
        command, check=True, capture_output=True, text=True, env=environment
    ).stdout
    if RESULT_TYPE not in generated:
        raise RuntimeError("generated bindings do not declare ctex_result as expected")
    generated = generated.replace(RESULT_TYPE, RESULT_ALIAS, 1)
    header_digest = hashlib.sha256(HEADER.read_bytes()).hexdigest()
    return (
        "//! Raw declarations generated from `include/ctex/capi.h`.\n"
        "//! Regenerate with `python3 tools/generate_rust_sys.py`.\n\n"
        f"//! C header SHA-256: {header_digest}\n\n"
        "#![allow(non_camel_case_types, non_snake_case, non_upper_case_globals)]\n\n"
        + generated
    )


def main() -> int:
    try:
        generated = generate()
    except (RuntimeError, subprocess.CalledProcessError) as error:
        print(f"Rust binding generation failed: {error}", file=sys.stderr)
        return 1
    if OUTPUT.read_text(encoding="utf-8") != generated:
        OUTPUT.write_text(generated, encoding="utf-8")
        print(f"updated {OUTPUT.relative_to(ROOT)}")
    else:
        print(f"ok: {OUTPUT.relative_to(ROOT)} is current")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
