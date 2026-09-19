from __future__ import annotations

import importlib.util
import tempfile
import unittest
from pathlib import Path


SCRIPT = Path(__file__).resolve().parents[2] / "tools" / "check_determinism.py"
SPEC = importlib.util.spec_from_file_location("check_determinism", SCRIPT)
assert SPEC and SPEC.loader
CHECK_DETERMINISM = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(CHECK_DETERMINISM)


class DeterminismGateTests(unittest.TestCase):
    def test_changed_output_is_named(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            root = Path(temporary_directory)
            first = root / "first"
            second = root / "second"
            first.mkdir()
            second.mkdir()
            (first / "result.bin").write_bytes(b"first")
            (second / "result.bin").write_bytes(b"second")

            failures = CHECK_DETERMINISM.compare_outputs(
                first, second, ["result.bin"], "save fixture"
            )

        self.assertEqual(
            failures,
            ["save fixture: output is not byte-identical: result.bin"],
        )

    def test_identical_output_passes(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            root = Path(temporary_directory)
            first = root / "first"
            second = root / "second"
            first.mkdir()
            second.mkdir()
            (first / "result.bin").write_bytes(b"same")
            (second / "result.bin").write_bytes(b"same")

            failures = CHECK_DETERMINISM.compare_outputs(
                first, second, ["result.bin"], "save fixture"
            )

        self.assertEqual(failures, [])


if __name__ == "__main__":
    unittest.main()
