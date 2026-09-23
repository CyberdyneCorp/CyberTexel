from __future__ import annotations

import importlib.util
import unittest
from pathlib import Path
from unittest import mock


SCRIPT = Path(__file__).resolve().parents[2] / "tools" / "generate_rust_sys.py"
SPEC = importlib.util.spec_from_file_location("generate_rust_sys", SCRIPT)
assert SPEC and SPEC.loader
GENERATOR = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(GENERATOR)


class RustSysGenerationTests(unittest.TestCase):
    def test_missing_bindgen_has_an_actionable_error(self) -> None:
        with mock.patch.object(GENERATOR.shutil, "which", return_value=None):
            with self.assertRaisesRegex(RuntimeError, "install bindgen-cli 0.72.1"):
                GENERATOR.bindgen_path()

    def test_generated_surface_adds_the_compatibility_alias(self) -> None:
        completed = mock.Mock(stdout=GENERATOR.RESULT_TYPE + "\n")
        with (
            mock.patch.object(GENERATOR, "bindgen_path", return_value="bindgen"),
            mock.patch.object(GENERATOR.subprocess, "run", return_value=completed) as run,
        ):
            generated = GENERATOR.generate()
        self.assertIn(GENERATOR.RESULT_ALIAS, generated)
        self.assertIn("generated from `include/ctex/capi.h`", generated)
        self.assertIn("--allowlist-function", run.call_args.args[0])


if __name__ == "__main__":
    unittest.main()
