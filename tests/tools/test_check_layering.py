from __future__ import annotations

import importlib.util
import tempfile
import unittest
from pathlib import Path


SCRIPT = Path(__file__).resolve().parents[2] / "tools" / "check_layering.py"
SPEC = importlib.util.spec_from_file_location("check_layering", SCRIPT)
assert SPEC and SPEC.loader
CHECK_LAYERING = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(CHECK_LAYERING)


class LayeringGateTests(unittest.TestCase):
    def test_cycle_names_every_module_in_the_cycle(self) -> None:
        failures = CHECK_LAYERING.graph_failures({"image": {"mesh"}, "mesh": {"image"}})

        self.assertTrue(any("image -> mesh -> image" in failure for failure in failures))

    def test_backend_include_outside_exec_names_the_file(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            root = Path(temporary_directory)
            source = root / "src" / "image"
            source.mkdir(parents=True)
            (source / "image.cpp").write_text("#include <vulkan/vulkan.h>\n", encoding="utf-8")

            failures = CHECK_LAYERING.source_failures(root, {"image": set()})

        self.assertEqual(failures, ["backend include outside exec: src/image/image.cpp:1"])


if __name__ == "__main__":
    unittest.main()
