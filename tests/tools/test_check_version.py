from __future__ import annotations

import importlib.util
import tempfile
import unittest
from pathlib import Path


SCRIPT = Path(__file__).resolve().parents[2] / "tools" / "check_version.py"
SPEC = importlib.util.spec_from_file_location("check_version", SCRIPT)
assert SPEC and SPEC.loader
CHECK_VERSION = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(CHECK_VERSION)


class VersionGateTests(unittest.TestCase):
    def test_drift_names_the_package(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            root = Path(temporary_directory)
            (root / "python").mkdir()
            (root / "rust").mkdir()
            (root / "swift").mkdir()
            (root / "python" / "pyproject.toml").write_text(
                '[project]\nversion = "9.9.9"\n', encoding="utf-8"
            )
            (root / "rust" / "Cargo.toml").write_text(
                '[package]\nversion = "0.1.0"\n', encoding="utf-8"
            )
            (root / "swift" / "Package.swift").write_text(
                'let cyberTexelVersion = "0.1.0"\n', encoding="utf-8"
            )

            failures = CHECK_VERSION.package_failures(root, "0.1.0")

        self.assertEqual(
            failures,
            ["Python package version 9.9.9 differs from VERSION 0.1.0"],
        )


if __name__ == "__main__":
    unittest.main()
