#!/usr/bin/env python3
"""Check that every public C operation is reachable from each official binding."""

from __future__ import annotations

import importlib.util
import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def load_abi_checker(root: Path):
    path = root / "tools" / "check_abi.py"
    spec = importlib.util.spec_from_file_location("ctex_check_abi", path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"could not load ABI checker from {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def python_operations(source: str) -> set[str]:
    return set(re.findall(r'_signature\(\s*library,\s*"(ctex_[A-Za-z0-9_]+)"', source))


def rust_operations(source: str) -> set[str]:
    extern_blocks = re.findall(
        r'(?:unsafe\s+)?extern\s+"C"\s*\{(.*?)\}', source, re.DOTALL
    )
    return {
        name
        for block in extern_blocks
        for name in re.findall(r"\bpub\s+fn\s+(ctex_[A-Za-z0-9_]+)\s*\(", block)
    }


def swift_imports_complete_c_header(root: Path) -> bool:
    modulemap = root / "swift" / "Sources" / "CyberTexelC" / "module.modulemap"
    shim = root / "swift" / "Sources" / "CyberTexelC" / "shim.h"
    if not modulemap.is_file() or not shim.is_file():
        return False
    module_text = modulemap.read_text(encoding="utf-8")
    shim_text = shim.read_text(encoding="utf-8")
    return (
        re.search(r'\bheader\s+"shim\.h"', module_text) is not None
        and re.search(r"#\s*include\s*[<\"]ctex/capi\.h[>\"]", shim_text) is not None
    )


def binding_operations(root: Path) -> dict[str, set[str]]:
    abi = load_abi_checker(root)
    header = (root / "include" / "ctex" / "capi.h").read_text(encoding="utf-8")
    c_operations = set(abi.extract_symbols(header))
    python_source = (root / "python" / "src" / "cybertexel" / "_native.py").read_text(
        encoding="utf-8"
    )
    rust_source = (root / "rust" / "cybertexel-sys" / "src" / "lib.rs").read_text(
        encoding="utf-8"
    )
    return {
        "c": c_operations,
        "python": python_operations(python_source),
        "rust": rust_operations(rust_source),
        "swift": c_operations if swift_imports_complete_c_header(root) else set(),
    }


def check(root: Path) -> list[str]:
    operations = binding_operations(root)
    expected = operations["c"]
    failures: list[str] = []
    for binding in ("python", "swift", "rust"):
        for name in sorted(expected - operations[binding]):
            failures.append(f"{binding} binding is missing C operation: {name}")
        for name in sorted(operations[binding] - expected):
            failures.append(f"{binding} binding declares unknown C operation: {name}")
    return failures


def main() -> int:
    failures = check(ROOT)
    if failures:
        operations = binding_operations(ROOT)
        expected = operations["c"]
        print("binding parity check failed:", file=sys.stderr)
        for binding in ("python", "swift", "rust"):
            missing = sorted(expected - operations[binding])
            if missing:
                print(f"  {binding}: {len(missing)} missing", file=sys.stderr)
                for name in missing:
                    print(f"    - {name}", file=sys.stderr)
        return 1
    print(f"ok: all {len(binding_operations(ROOT)['c'])} C operations reach every binding")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
