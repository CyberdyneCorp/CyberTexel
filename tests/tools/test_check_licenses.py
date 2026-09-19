from __future__ import annotations

import importlib.util
import json
import tempfile
import unittest
from pathlib import Path


SCRIPT = Path(__file__).resolve().parents[2] / "tools" / "check_licenses.py"
SPEC = importlib.util.spec_from_file_location("check_licenses", SCRIPT)
assert SPEC and SPEC.loader
CHECK_LICENSES = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(CHECK_LICENSES)


class LicenceGateTests(unittest.TestCase):
    def make_root(self) -> Path:
        temporary_directory = tempfile.TemporaryDirectory()
        self.addCleanup(temporary_directory.cleanup)
        root = Path(temporary_directory.name)
        (root / "thirdparty").mkdir()
        (root / "thirdparty" / "dependencies.json").write_text(
            json.dumps({"schema": 1, "dependencies": []}), encoding="utf-8"
        )
        (root / "THIRD_PARTY_NOTICES.md").write_text("No dependencies.\n", encoding="utf-8")
        (root / "CMakeLists.txt").write_text("project(test)\n", encoding="utf-8")
        return root

    def test_untracked_vendored_tree_is_named(self) -> None:
        root = self.make_root()
        (root / "thirdparty" / "surprise").mkdir()

        failures = CHECK_LICENSES.audit(root)

        self.assertEqual(failures, ["untracked dependency: vendored tree thirdparty/surprise"])

    def test_copyleft_dependency_is_rejected(self) -> None:
        root = self.make_root()
        (root / "thirdparty" / "copyleft").mkdir()
        license_path = root / "thirdparty" / "copyleft" / "COPYING"
        license_path.write_text("G" * 80, encoding="utf-8")
        dependency = {
            "name": "copyleft",
            "discovery_name": "copyleft",
            "license": "GPL-3.0-only",
            "license_file": "thirdparty/copyleft/COPYING",
            "revision": "0123456789abcdef",
            "source": "https://example.com/copyleft",
        }
        (root / "thirdparty" / "dependencies.json").write_text(
            json.dumps({"schema": 1, "dependencies": [dependency]}), encoding="utf-8"
        )
        (root / "THIRD_PARTY_NOTICES.md").write_text(
            "copyleft GPL-3.0-only 0123456789abcdef\n", encoding="utf-8"
        )

        failures = CHECK_LICENSES.audit(root)

        self.assertIn("copyleft: licence 'GPL-3.0-only' is not permitted", failures)

    def test_platform_threads_are_not_reported_as_a_shipped_dependency(self) -> None:
        root = self.make_root()
        (root / "CMakeLists.txt").write_text(
            "find_package(Threads REQUIRED)\nfind_package(Surprise REQUIRED)\n",
            encoding="utf-8",
        )

        failures = CHECK_LICENSES.audit(root)

        self.assertEqual(
            failures,
            ["untracked dependency: find_package declaration CMakeLists.txt"],
        )


if __name__ == "__main__":
    unittest.main()
