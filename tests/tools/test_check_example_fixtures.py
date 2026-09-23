from __future__ import annotations

import hashlib
import importlib.util
import json
import tempfile
import unittest
from pathlib import Path


SCRIPT = Path(__file__).resolve().parents[2] / "tools" / "check_example_fixtures.py"
SPEC = importlib.util.spec_from_file_location("check_example_fixtures", SCRIPT)
assert SPEC and SPEC.loader
CHECK = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(CHECK)


class ExampleFixtureTests(unittest.TestCase):
    def test_missing_manifest_is_named(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            self.assertEqual(
                CHECK.check(Path(directory)),
                ["fixture manifest and ATTRIBUTION.md are required"],
            )

    def test_digest_provenance_and_categories_are_enforced(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            asset = root / "asset.bin"
            asset.write_bytes(b"fixture")
            digest = hashlib.sha256(asset.read_bytes()).hexdigest()
            (root / "ATTRIBUTION.md").write_text("`asset.bin`\n", encoding="utf-8")
            (root / "manifest.json").write_text(
                json.dumps(
                    {
                        "assets": [
                            {
                                "path": "asset.bin",
                                "category": "image",
                                "origin": "test",
                                "license": "Apache-2.0",
                                "sha256": digest,
                            }
                        ]
                    }
                ),
                encoding="utf-8",
            )
            failures = CHECK.check(root)
            self.assertIn("fixture category is missing: font", failures)
            asset.write_bytes(b"changed")
            self.assertIn("fixture digest changed: asset.bin", CHECK.check(root))


if __name__ == "__main__":
    unittest.main()
