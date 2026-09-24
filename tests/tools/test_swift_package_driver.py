from __future__ import annotations

import importlib.util
from pathlib import Path
import tempfile
import unittest
from unittest import mock


SCRIPT = Path(__file__).resolve().parents[2] / "tools" / "test_swift_package.py"
SPEC = importlib.util.spec_from_file_location("test_swift_package_driver", SCRIPT)
assert SPEC and SPEC.loader
DRIVER = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(DRIVER)


class SwiftPackageDriverTests(unittest.TestCase):
    def test_install_builds_every_installed_target_on_clean_runner(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            prefix = Path(directory)
            commands: list[tuple[str, ...]] = []

            def fake_run(*args: str, env: dict[str, str]) -> None:
                commands.append(args)
                if "--install" in args:
                    metadata = prefix / "lib" / "pkgconfig" / "cybertexel.pc"
                    metadata.parent.mkdir(parents=True)
                    metadata.write_text("Libs: -lcybertexel_c\n")

            with mock.patch.object(DRIVER, "run", side_effect=fake_run):
                DRIVER.install_native("macos-universal", prefix, {})
            self.assertIn(("cmake", "--build", "--preset", "macos-universal", "--target",
                           "cybertexel_c", "cybertexel_cli"), commands)


if __name__ == "__main__":
    unittest.main()
