from __future__ import annotations

import importlib.util
import tempfile
import unittest
from pathlib import Path


SCRIPT = Path(__file__).resolve().parents[2] / "tools" / "check_example_gallery.py"
SPEC = importlib.util.spec_from_file_location("check_example_gallery", SCRIPT)
assert SPEC and SPEC.loader
CHECK = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(CHECK)


class ExampleGalleryCheckTests(unittest.TestCase):
    def test_complete_gallery_passes(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "examples" / "outputs" / "01_demo").mkdir(parents=True)
            (root / "examples" / "01_demo.py").write_text("", encoding="utf-8")
            (root / "examples" / "outputs" / "01_demo" / "result.png").write_bytes(b"png")
            (root / "docs").mkdir()
            (root / "docs" / "gallery.md").write_text(
                "## `01_demo`\n\n../examples/outputs/01_demo/result.png\n",
                encoding="utf-8",
            )
            self.assertEqual(CHECK.gallery_failures(root), [])

    def test_missing_example_and_artifact_are_named(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "examples" / "outputs" / "01_demo").mkdir(parents=True)
            (root / "examples" / "01_demo.py").write_text("", encoding="utf-8")
            (root / "examples" / "outputs" / "01_demo" / "result.json").write_text("{}")
            (root / "docs").mkdir()
            (root / "docs" / "gallery.md").write_text("# Empty\n", encoding="utf-8")
            self.assertEqual(
                CHECK.gallery_failures(root),
                [
                    "gallery is missing example 01_demo",
                    "gallery is missing artifact examples/outputs/01_demo/result.json",
                ],
            )


if __name__ == "__main__":
    unittest.main()
