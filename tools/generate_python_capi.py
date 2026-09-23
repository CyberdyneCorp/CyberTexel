#!/usr/bin/env python3
"""Regenerate the committed Python ctypes surface from the public C header."""

from __future__ import annotations

import hashlib
import importlib.util
import re
import shutil
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "include" / "ctex" / "capi.h"
OUTPUT = ROOT / "python" / "src" / "cybertexel" / "capi.py"


def load_abi_checker():
    path = ROOT / "tools" / "check_abi.py"
    spec = importlib.util.spec_from_file_location("ctex_generate_python_abi", path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"could not load ABI checker from {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def uv_path() -> str:
    executable = shutil.which("uv")
    if executable is None:
        raise RuntimeError("uv is required to run the pinned ctypesgen generator")
    return executable


def generated_operations(source: str) -> set[str]:
    return set(re.findall(r"^\s*(ctex_[A-Za-z0-9_]+)\.argtypes\s*=", source, re.MULTILINE))


def generate() -> str:
    command = [
        uv_path(),
        "tool",
        "run",
        "--from",
        "ctypesgen==1.1.1",
        "ctypesgen",
        str(HEADER),
        "-I",
        str(ROOT / "include"),
        "-l",
        "cybertexel_c",
        "-i",
        "^ctex_.*",
        "-i",
        "^CTEX_.*",
        "--no-macro-warnings",
    ]
    generated = subprocess.run(
        command, cwd=ROOT, check=True, capture_output=True, text=True
    ).stdout
    expected = set(load_abi_checker().extract_symbols(HEADER.read_text(encoding="utf-8")))
    actual = generated_operations(generated)
    if actual != expected:
        missing = ", ".join(sorted(expected - actual))
        unknown = ", ".join(sorted(actual - expected))
        raise RuntimeError(
            f"ctypesgen operation mismatch; missing=[{missing}], unknown=[{unknown}]"
        )
    generated = re.sub(
        r'^r"""Wrapper for capi\.h.*?"""',
        '"""Generated raw ctypes declarations for the CyberTexel C ABI.\n\n'
        "Regenerate with ``just generate-python-capi``.\n"
        '"""',
        generated,
        count=1,
        flags=re.DOTALL,
    )
    search = 'add_library_search_dirs([])'
    replacement = (
        'add_library_search_dirs([os.path.join(os.path.dirname(__file__), "_native_lib")])'
    )
    if generated.count(search) != 1:
        raise RuntimeError("ctypesgen library search marker changed")
    generated = generated.replace(search, replacement, 1)
    load = '_libs["cybertexel_c"] = load_library("cybertexel_c")'
    configured_load = (
        '_libs["cybertexel_c"] = load_library('
        'os.environ.get("CYBERTEXEL_LIBRARY", "cybertexel_c"))'
    )
    if generated.count(load) != 1:
        raise RuntimeError("ctypesgen library load marker changed")
    generated = generated.replace(load, configured_load, 1)
    generated = re.sub(r"# /[^\n]+:\s*\d+\s*$", "", generated, flags=re.MULTILINE)
    digest = hashlib.sha256(HEADER.read_bytes()).hexdigest()
    return f"# C header SHA-256: {digest}\n{generated}"


def main() -> int:
    try:
        generated = generate()
    except (RuntimeError, subprocess.CalledProcessError) as error:
        print(f"Python binding generation failed: {error}", file=sys.stderr)
        return 1
    current = OUTPUT.read_text(encoding="utf-8") if OUTPUT.is_file() else ""
    if current != generated:
        OUTPUT.write_text(generated, encoding="utf-8")
        print(f"updated {OUTPUT.relative_to(ROOT)}")
    else:
        print(f"ok: {OUTPUT.relative_to(ROOT)} is current")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
