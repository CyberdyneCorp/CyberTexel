from __future__ import annotations

import os
import platform
import shutil
from pathlib import Path

from setuptools import Distribution, setup
from setuptools.command.bdist_wheel import bdist_wheel
from setuptools.command.build_py import build_py

HERE = Path(__file__).resolve().parent
REPOSITORY = HERE.parent


def bundled_name() -> str:
    if os.name == "nt":
        return "cybertexel_c.dll"
    if platform.system() == "Darwin":
        return "libcybertexel_c.dylib"
    return "libcybertexel_c.so"


def native_library() -> Path:
    configured = os.environ.get("CYBERTEXEL_LIBRARY")
    if configured:
        path = Path(configured).expanduser().resolve()
        if not path.is_file():
            raise RuntimeError(f"CYBERTEXEL_LIBRARY does not name a file: {path}")
        return path

    names = (
        ["cybertexel_c.dll"]
        if os.name == "nt"
        else ["libcybertexel_c.dylib", "libcybertexel_c.so"]
    )
    for name in names:
        candidates = sorted((REPOSITORY / "build").glob(f"**/{name}"))
        if candidates:
            return candidates[0].resolve()
    raise RuntimeError(
        "CyberTexel shared library was not found; build it first or set CYBERTEXEL_LIBRARY"
    )


class PlatformDistribution(Distribution):
    def has_ext_modules(self) -> bool:
        return True


class BuildWithNativeLibrary(build_py):
    def run(self) -> None:
        super().run()
        destination = (
            Path(self.build_lib) / "cybertexel" / "_native_lib" / bundled_name()
        )
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(native_library(), destination)


class PlatformWheel(bdist_wheel):
    def finalize_options(self) -> None:
        super().finalize_options()
        self.root_is_pure = False

    def get_tag(self) -> tuple[str, str, str]:
        _python, _abi, platform_tag = super().get_tag()
        return "py3", "none", platform_tag


setup(
    cmdclass={"build_py": BuildWithNativeLibrary, "bdist_wheel": PlatformWheel},
    distclass=PlatformDistribution,
)
