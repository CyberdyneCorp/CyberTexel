from __future__ import annotations

import importlib.util
import json
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


class DeterminismCategoryTests(unittest.TestCase):
    def manifest(self, root: Path) -> Path:
        path = root / "cases.json"
        path.write_text(
            json.dumps(
                {
                    "schema": 1,
                    "categories": [
                        {"name": "ready", "task": "1", "cases": []},
                        {"name": "future", "task": "2", "cases": []},
                    ],
                }
            ),
            encoding="utf-8",
        )
        return path

    def test_selected_category_does_not_run_other_empty_categories(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            failures = CHECK_DETERMINISM.check(root, self.manifest(root), {"ready"})
        self.assertEqual(
            failures, ["ready: no determinism cases registered; delivered by task 1"]
        )

    def test_unknown_selected_category_is_reported(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            failures = CHECK_DETERMINISM.check(root, self.manifest(root), {"missing"})
        self.assertEqual(failures, ["unknown determinism category: missing"])


if __name__ == "__main__":
    unittest.main()
