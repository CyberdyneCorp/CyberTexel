from __future__ import annotations

import importlib.util
import unittest
from pathlib import Path
from unittest import mock


SCRIPT = Path(__file__).resolve().parents[2] / "tools" / "generate_python_capi.py"
SPEC = importlib.util.spec_from_file_location("generate_python_capi", SCRIPT)
assert SPEC and SPEC.loader
GENERATOR = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(GENERATOR)


class PythonCapiGenerationTests(unittest.TestCase):
    def test_missing_uv_has_an_actionable_error(self) -> None:
        with mock.patch.object(GENERATOR.shutil, "which", return_value=None):
            with self.assertRaisesRegex(RuntimeError, "uv is required"):
                GENERATOR.uv_path()

    def test_generated_operation_extraction_requires_argtypes(self) -> None:
        source = '''
ctex_one = library.ctex_one
ctex_one.argtypes = []
ctex_one.restype = c_uint
ctex_untyped = library.ctex_untyped
'''
        self.assertEqual(GENERATOR.generated_operations(source), {"ctex_one"})


if __name__ == "__main__":
    unittest.main()
