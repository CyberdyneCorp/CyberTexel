from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SAFE_CRATE = ROOT / "rust" / "cybertexel" / "src"
BOUNDARY = SAFE_CRATE / "ffi.rs"
UNSAFE = re.compile(r"\bunsafe\b")


def main() -> int:
    violations: list[str] = []
    boundary_count = 0
    for path in sorted(SAFE_CRATE.rglob("*.rs")):
        text = path.read_text(encoding="utf-8")
        matches = list(UNSAFE.finditer(text))
        if path == BOUNDARY:
            boundary_count += len(matches)
        elif matches:
            violations.append(str(path.relative_to(ROOT)))
    if violations:
        raise RuntimeError(f"unsafe escaped the safe wrapper boundary: {violations}")
    if boundary_count == 0:
        raise RuntimeError("Rust FFI boundary contains no explicit unsafe operation")
    print(f"ok: {boundary_count} unsafe markers confined to rust/cybertexel/src/ffi.rs")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
