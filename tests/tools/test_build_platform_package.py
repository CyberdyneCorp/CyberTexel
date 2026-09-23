from __future__ import annotations

import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
import zipfile


ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "tools" / "build_platform_package.py"
SPEC = importlib.util.spec_from_file_location("build_platform_package", SCRIPT)
assert SPEC and SPEC.loader
PACKAGE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(PACKAGE)


class PlatformPackageTests(unittest.TestCase):
    def complete_prefix(self, root: Path) -> Path:
        prefix = root / "prefix"
        for relative in (
            "include/ctex/capi.h",
            "include/ctex/version.h",
            "lib/libcybertexel_c.a",
            "share/cybertexel/LICENSE",
            "share/cybertexel/THIRD_PARTY_NOTICES.md",
        ):
            path = prefix / relative
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(relative + "\n", encoding="utf-8")
        return prefix

    def test_incomplete_install_names_every_missing_contract_file(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaisesRegex(PACKAGE.PackageError, "capi.h"):
                PACKAGE.validate_install(Path(directory))

    def test_manifest_records_platform_and_smoke_disposition(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            prefix = self.complete_prefix(Path(directory))
            PACKAGE.validate_install(prefix)
            PACKAGE.write_manifest(prefix, "ios-arm64", "linked")
            manifest = json.loads(
                (prefix / "share/cybertexel/package.json").read_text(encoding="utf-8")
            )
            self.assertEqual(manifest["platform"], "ios-arm64")
            self.assertEqual(manifest["smoke_test"], "linked")

    def test_archive_is_reproducible_and_rooted(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            prefix = self.complete_prefix(root)
            first = root / "first.zip"
            second = root / "second.zip"
            PACKAGE.archive_tree(prefix, first)
            PACKAGE.archive_tree(prefix, second)
            self.assertEqual(first.read_bytes(), second.read_bytes())
            with zipfile.ZipFile(first) as archive:
                self.assertTrue(all(name.startswith("cybertexel/") for name in archive.namelist()))


if __name__ == "__main__":
    unittest.main()
