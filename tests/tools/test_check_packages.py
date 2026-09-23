from __future__ import annotations

import importlib.util
import json
import tempfile
import unittest
from pathlib import Path


SCRIPT = Path(__file__).resolve().parents[2] / "tools" / "check_packages.py"
SPEC = importlib.util.spec_from_file_location("check_packages", SCRIPT)
assert SPEC and SPEC.loader
CHECK = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(CHECK)

VERSION = "1.2.3"


class PackageGateTests(unittest.TestCase):
    def make_root(self, directory: str, *, deferred: dict[str, str] | None = None) -> Path:
        root = Path(directory) / "root"
        (root / "release").mkdir(parents=True)
        (root / "dist").mkdir(parents=True)
        (root / "VERSION").write_text(f"{VERSION}\n", encoding="utf-8")
        (root / "release" / "platforms.json").write_text(
            json.dumps(
                {
                    "schema": 1,
                    "in_scope": [
                        {
                            "preset": "macos-universal",
                            "kind": "desktop",
                            "recipe": "package-macos",
                            "smoke_test": "executed",
                        }
                    ],
                    "deferred": deferred if deferred is not None else {"windows-x64": "task 18.6"},
                }
            ),
            encoding="utf-8",
        )
        return root

    def install(self, root: Path, preset: str, *, smoke: str = "executed", version: str = VERSION) -> None:
        prefix = root / "build" / "packages" / preset / "root"
        (prefix / "include" / "ctex").mkdir(parents=True)
        (prefix / "share" / "cybertexel").mkdir(parents=True)
        (prefix / "include" / "ctex" / "capi.h").write_text("", encoding="utf-8")
        (prefix / "include" / "ctex" / "version.h").write_text("", encoding="utf-8")
        (prefix / "share" / "cybertexel" / "LICENSE").write_text("", encoding="utf-8")
        (prefix / "share" / "cybertexel" / "THIRD_PARTY_NOTICES.md").write_text("", encoding="utf-8")
        (prefix / "share" / "cybertexel" / "package.json").write_text(
            json.dumps({"platform": preset, "version": version, "smoke_test": smoke}),
            encoding="utf-8",
        )
        (root / "dist" / f"cybertexel-{VERSION}-{preset}.zip").write_bytes(b"")

    def test_complete_in_scope_package_passes(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = self.make_root(directory)
            self.install(root, "macos-universal")
            failures, decided = CHECK.package_failures(root)
            self.assertEqual(failures, [])
            self.assertEqual(len(decided), 1)

    def test_absent_package_is_named(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = self.make_root(directory)
            failures, _ = CHECK.package_failures(root)
            self.assertTrue(any("package-macos" in failure for failure in failures), failures)

    def test_unexercised_smoke_test_fails(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = self.make_root(directory)
            self.install(root, "macos-universal", smoke="linked")
            failures, _ = CHECK.package_failures(root)
            self.assertTrue(any("smoke test" in failure for failure in failures), failures)

    def test_unarchived_package_fails(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = self.make_root(directory)
            self.install(root, "macos-universal")
            (root / "dist" / f"cybertexel-{VERSION}-macos-universal.zip").unlink()
            failures, _ = CHECK.package_failures(root)
            self.assertTrue(any("not archived" in failure for failure in failures), failures)

    def test_version_drift_fails(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = self.make_root(directory)
            self.install(root, "macos-universal", version="9.9.9")
            failures, _ = CHECK.package_failures(root)
            self.assertTrue(any("VERSION" in failure for failure in failures), failures)

    def test_deferred_platform_needs_a_recorded_decision(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = self.make_root(directory, deferred={"windows-x64": "   "})
            with self.assertRaises(CHECK.ScopeError):
                CHECK.load_scope(root)

    def test_platform_cannot_be_in_scope_and_deferred(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = self.make_root(directory, deferred={"macos-universal": "task 18.6"})
            with self.assertRaises(CHECK.ScopeError):
                CHECK.load_scope(root)


if __name__ == "__main__":
    unittest.main()
