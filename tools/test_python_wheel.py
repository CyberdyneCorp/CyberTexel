from __future__ import annotations

import os
import platform
import subprocess
import sys
import tempfile
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DIST = ROOT / "build" / "python-dist"


def run(*arguments: str, cwd: Path = ROOT, env: dict[str, str] | None = None) -> None:
    subprocess.run(arguments, cwd=cwd, env=env, check=True)


def venv_python(directory: Path) -> Path:
    return directory / ("Scripts/python.exe" if os.name == "nt" else "bin/python")


def validate_wheel(wheel: Path) -> None:
    with zipfile.ZipFile(wheel) as archive:
        names = archive.namelist()
        if "cybertexel/_script_runner.py" not in names:
            raise RuntimeError("wheel is missing the headless CLI script runner")
        native = [
            name
            for name in names
            if name.endswith(
                ("libcybertexel_c.so", "libcybertexel_c.dylib", "cybertexel_c.dll")
            )
        ]
        if len(native) != 1:
            raise RuntimeError(
                f"wheel must contain exactly one native library, found {native}"
            )
        metadata_name = next(
            name for name in names if name.endswith(".dist-info/METADATA")
        )
        metadata = archive.read(metadata_name).decode("utf-8")
        dependencies = [
            line.removeprefix("Requires-Dist: ")
            for line in metadata.splitlines()
            if line.startswith("Requires-Dist: ")
        ]
        if len(dependencies) != 1 or not dependencies[0].startswith("numpy"):
            raise RuntimeError(
                f"wheel runtime dependencies must contain only NumPy: {dependencies}"
            )


def packaged_native_library() -> Path:
    configured = os.environ.get("CYBERTEXEL_LIBRARY")
    if configured:
        path = Path(configured).expanduser().resolve()
        if not path.is_file():
            raise RuntimeError(f"CYBERTEXEL_LIBRARY does not name a file: {path}")
        return path
    preset = {
        "Darwin": "macos-universal",
        "Linux": "linux-x64",
        "Windows": "windows-x64",
    }.get(platform.system())
    if preset is None:
        raise RuntimeError(f"Python wheels are not supported on {platform.system()}")
    run("cmake", "--preset", preset)
    run("cmake", "--build", "--preset", preset, "--target", "cybertexel_c")
    name = (
        "cybertexel_c.dll"
        if os.name == "nt"
        else (
            "libcybertexel_c.dylib"
            if platform.system() == "Darwin"
            else "libcybertexel_c.so"
        )
    )
    candidates = sorted((ROOT / "build" / preset).glob(f"**/{name}"))
    if not candidates:
        raise RuntimeError(f"{name} was not produced by the {preset} build")
    return candidates[0].resolve()


def main() -> int:
    python_version = os.environ.get("CTEX_PYTHON_VERSION", "3.10")
    build_environment = os.environ.copy()
    build_environment["CYBERTEXEL_LIBRARY"] = str(packaged_native_library())
    run(
        "uv",
        "build",
        "--wheel",
        "--clear",
        "--python",
        python_version,
        "--out-dir",
        str(DIST),
        str(ROOT / "python"),
        env=build_environment,
    )
    wheels = list(DIST.glob("cybertexel-*.whl"))
    if len(wheels) != 1:
        raise RuntimeError(f"expected one CyberTexel wheel, found {wheels}")
    wheel = wheels[0]
    validate_wheel(wheel)
    with tempfile.TemporaryDirectory(prefix="ctex-python-wheel-") as temporary:
        environment = Path(temporary)
        run("uv", "venv", "--python", python_version, str(environment))
        interpreter = venv_python(environment)
        run("uv", "pip", "install", "--python", str(interpreter), str(wheel))
        smoke_environment = os.environ.copy()
        smoke_environment.pop("CYBERTEXEL_LIBRARY", None)
        smoke_environment.pop("PYTHONPATH", None)
        run(
            str(interpreter),
            str(ROOT / "python" / "tests" / "wheel_smoke.py"),
            cwd=ROOT / "python" / "tests",
            env=smoke_environment,
        )
    print(f"ok: built and smoke-tested {wheel.name} with Python {python_version}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
