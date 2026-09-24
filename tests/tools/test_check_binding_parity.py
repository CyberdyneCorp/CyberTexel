from __future__ import annotations

import hashlib
import importlib.util
import io
import tempfile
import unittest
from pathlib import Path
from unittest import mock


SCRIPT = Path(__file__).resolve().parents[2] / "tools" / "check_binding_parity.py"
SPEC = importlib.util.spec_from_file_location("check_binding_parity", SCRIPT)
assert SPEC and SPEC.loader
CHECK = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(CHECK)


class BindingParityTests(unittest.TestCase):
    def test_main_reports_stale_generated_header_digest(self) -> None:
        output = io.StringIO()
        with (
            mock.patch.object(CHECK, "check", return_value=["rust binding has a stale header digest"]),
            mock.patch("sys.stderr", output),
        ):
            self.assertEqual(CHECK.main(), 1)
        self.assertIn("stale header digest", output.getvalue())

    def test_check_names_missing_and_unknown_operations_per_binding(self) -> None:
        operations = {
            "c": {"ctex_one", "ctex_two"},
            "python": {"ctex_one"},
            "swift": {"ctex_one", "ctex_two"},
            "rust": {"ctex_one", "ctex_unknown"},
        }
        with (
            mock.patch.object(CHECK, "binding_operations", return_value=operations),
            mock.patch.object(CHECK, "rust_surface_matches_header", return_value=True),
            mock.patch.object(CHECK, "python_surface_matches_header", return_value=True),
        ):
            self.assertEqual(
                CHECK.check(Path("repo")),
                [
                    "python binding is missing C operation: ctex_two",
                    "rust binding is missing C operation: ctex_two",
                    "rust binding declares unknown C operation: ctex_unknown",
                ],
            )

    def test_python_extracts_registered_ctypes_signatures(self) -> None:
        source = '''
_signature(library, "ctex_one", [], ctypes.c_uint32)
_signature(
    library,
    "ctex_two",
    [ctypes.c_void_p],
    None,
)
library.ctex_untyped()
'''
        self.assertEqual(CHECK.python_operations(source), {"ctex_one", "ctex_two"})

    def test_python_generated_surface_requires_typed_functions(self) -> None:
        source = '''
ctex_one = _lib.get("ctex_one", "cdecl")
ctex_one.argtypes = []
ctex_one.restype = c_uint
ctex_untyped = _lib.get("ctex_untyped", "cdecl")
'''
        self.assertEqual(CHECK.generated_python_operations(source), {"ctex_one"})

    def test_rust_extracts_only_public_extern_declarations(self) -> None:
        source = '''
extern "C" {
    pub fn ctex_one() -> ctex_result;
    fn ctex_private();
}
unsafe extern "C" {
    pub fn ctex_two();
}
pub fn ctex_wrapper() {}
'''
        self.assertEqual(CHECK.rust_operations(source), {"ctex_one", "ctex_two"})

    def test_swift_requires_the_system_module_to_import_the_public_header(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            target = root / "swift" / "Sources" / "CyberTexelC"
            target.mkdir(parents=True)
            (target / "module.modulemap").write_text(
                'module CyberTexelC { header "shim.h" export * }', encoding="utf-8"
            )
            (target / "shim.h").write_text('#include <ctex/capi.h>\n', encoding="utf-8")
            self.assertTrue(CHECK.swift_imports_complete_c_header(root))
            (target / "shim.h").write_text('#include <ctex/version.h>\n', encoding="utf-8")
            self.assertFalse(CHECK.swift_imports_complete_c_header(root))

    def test_rust_generated_surface_is_pinned_to_the_header(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            header = root / "include" / "ctex" / "capi.h"
            source = root / "rust" / "cybertexel-sys" / "src" / "lib.rs"
            header.parent.mkdir(parents=True)
            source.parent.mkdir(parents=True)
            header.write_text("void ctex_one(void);\n", encoding="utf-8")
            digest = hashlib.sha256(header.read_bytes()).hexdigest()
            source.write_text(f"//! C header SHA-256: {digest}\n", encoding="utf-8")
            self.assertTrue(CHECK.rust_surface_matches_header(root))
            header.write_text("void ctex_two(void);\n", encoding="utf-8")
            self.assertFalse(CHECK.rust_surface_matches_header(root))

    def test_python_generated_surface_is_pinned_to_the_header(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            header = root / "include" / "ctex" / "capi.h"
            source = root / "python" / "src" / "cybertexel" / "capi.py"
            header.parent.mkdir(parents=True)
            source.parent.mkdir(parents=True)
            header.write_text("void ctex_one(void);\n", encoding="utf-8")
            digest = hashlib.sha256(header.read_bytes()).hexdigest()
            source.write_text(f"# C header SHA-256: {digest}\n", encoding="utf-8")
            self.assertTrue(CHECK.python_surface_matches_header(root))
            source.write_text("# stale\n", encoding="utf-8")
            self.assertFalse(CHECK.python_surface_matches_header(root))


if __name__ == "__main__":
    unittest.main()
