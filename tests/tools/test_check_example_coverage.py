from __future__ import annotations

import importlib.util
import json
import tempfile
import unittest
from pathlib import Path


SCRIPT = Path(__file__).resolve().parents[2] / "tools" / "check_example_coverage.py"
SPEC = importlib.util.spec_from_file_location("check_example_coverage", SCRIPT)
assert SPEC and SPEC.loader
CHECK = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(CHECK)


class ExampleCoverageCheckTests(unittest.TestCase):
    def make_root(self, directory: str, capabilities: list[str]) -> Path:
        root = Path(directory)
        (root / "abi").mkdir()
        (root / "examples").mkdir()
        (root / "abi" / "capi-capabilities.json").write_text(
            json.dumps(
                {
                    "schema": 1,
                    "capabilities": [{"name": name} for name in capabilities],
                }
            ),
            encoding="utf-8",
        )
        return root

    def test_complete_coverage_passes(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = self.make_root(directory, ["image-io", "project-io"])
            (root / "examples" / "01_io.py").write_text(
                'CAPABILITIES = ("image-io", "project-io")\n', encoding="utf-8"
            )
            self.assertEqual(CHECK.coverage_failures(root), [])

    def test_missing_and_unknown_capabilities_are_named(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = self.make_root(directory, ["image-io", "project-io"])
            (root / "examples" / "01_io.py").write_text(
                'CAPABILITIES = ("image-io", "imaginary")\n', encoding="utf-8"
            )
            self.assertEqual(
                CHECK.coverage_failures(root),
                [
                    "01_io.py: unknown capabilities: imaginary",
                    "capabilities without numbered examples: project-io",
                ],
            )

    def test_declaration_must_be_unique_literal_and_nonempty(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = self.make_root(directory, ["image-io"])
            (root / "examples" / "01_io.py").write_text(
                'CAPABILITIES = ("image-io", "image-io")\n', encoding="utf-8"
            )
            self.assertEqual(
                CHECK.coverage_failures(root),
                [
                    "01_io.py: CAPABILITIES contains duplicates",
                    "capabilities without numbered examples: image-io",
                ],
            )


if __name__ == "__main__":
    unittest.main()
