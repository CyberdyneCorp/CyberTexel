from __future__ import annotations

import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
import zipfile


SCRIPT = Path(__file__).resolve().parents[2] / "tools" / "verify_release_assets.py"
SPEC = importlib.util.spec_from_file_location("verify_release_assets", SCRIPT)
assert SPEC and SPEC.loader
VERIFY = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(VERIFY)


class ReleaseAssetTests(unittest.TestCase):
    def fixture(self, directory: str) -> tuple[Path, Path]:
        root = Path(directory)
        (root / "release").mkdir()
        assets = root / "assets"
        assets.mkdir()
        (root / "VERSION").write_text("0.1.0\n")
        scope = {"in_scope": [
            {"preset": "linux-x64", "kind": "desktop", "smoke_test": "executed"},
            {"preset": "macos-universal", "kind": "desktop", "smoke_test": "executed"},
            {"preset": "ios-arm64", "kind": "mobile", "smoke_test": "linked"},
        ]}
        (root / "release" / "platforms.json").write_text(json.dumps(scope))
        for platform in scope["in_scope"]:
            self.archive(assets, platform)
        return root, assets

    def archive(self, assets: Path, platform: dict[str, str], *, version: str = "0.1.0") -> None:
        path = assets / f"cybertexel-0.1.0-{platform['preset']}.zip"
        with zipfile.ZipFile(path, "w") as archive:
            for name in VERIFY.REQUIRED - {"cybertexel/share/cybertexel/package.json"}:
                archive.writestr(name, "content")
            archive.writestr("cybertexel/lib/libcybertexel_c.a", "library")
            if platform["kind"] == "desktop":
                archive.writestr("cybertexel/bin/cybertexel", "cli")
            archive.writestr("cybertexel/share/cybertexel/package.json", json.dumps({
                "version": version, "platform": platform["preset"],
                "smoke_test": platform["smoke_test"],
            }))

    def test_three_complete_archives_pass(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root, assets = self.fixture(directory)
            failures, checksums = VERIFY.verify_assets(assets, root)
            self.assertEqual(failures, [])
            self.assertEqual(len(checksums), 3)

    def test_wrong_version_and_extra_platform_are_refused(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root, assets = self.fixture(directory)
            self.archive(assets, {"preset": "ios-arm64", "kind": "mobile", "smoke_test": "linked"},
                         version="9.9.9")
            (assets / "cybertexel-0.1.0-windows-x64.zip").write_bytes(b"unreleased")
            failures, _ = VERIFY.verify_assets(assets, root)
            self.assertTrue(any("version" in failure for failure in failures), failures)
            self.assertTrue(any("unexpected release asset" in failure for failure in failures), failures)


if __name__ == "__main__":
    unittest.main()
