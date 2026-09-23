#!/usr/bin/env python3
"""Build, validate, smoke-test and archive one CyberTexel platform package."""

from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import zipfile


ROOT = Path(__file__).resolve().parents[1]
PACKAGE_ROOT = ROOT / "build" / "packages"
DIST = ROOT / "dist"
DESKTOP_PRESETS = {"linux-x64", "macos-universal", "windows-x64"}
MOBILE_PRESETS = {"ios-arm64", "android-arm64"}
SUPPORTED_PRESETS = DESKTOP_PRESETS | MOBILE_PRESETS
FIXED_ZIP_TIME = (1980, 1, 1, 0, 0, 0)


class PackageError(RuntimeError):
    pass


def run(arguments: list[str], *, environment: dict[str, str]) -> None:
    subprocess.run(arguments, cwd=ROOT, env=environment, check=True)


def remove_tree(path: Path) -> None:
    if path.is_dir():
        shutil.rmtree(path)


def library_files(prefix: Path) -> list[Path]:
    libraries = []
    for directory in (prefix / "lib", prefix / "lib64", prefix / "bin"):
        if not directory.is_dir():
            continue
        libraries.extend(
            path
            for path in directory.iterdir()
            if path.is_file() and "cybertexel_c" in path.name.lower()
        )
    return sorted(libraries)


def validate_install(prefix: Path) -> None:
    required = (
        prefix / "include" / "ctex" / "capi.h",
        prefix / "include" / "ctex" / "version.h",
        prefix / "share" / "cybertexel" / "LICENSE",
        prefix / "share" / "cybertexel" / "THIRD_PARTY_NOTICES.md",
    )
    missing = [path.relative_to(prefix).as_posix() for path in required if not path.is_file()]
    if not library_files(prefix):
        missing.append("lib/<CyberTexel C library>")
    if missing:
        raise PackageError(f"installed package is incomplete: {', '.join(missing)}")


def cross_arguments(preset: str, environment: dict[str, str]) -> list[str]:
    if preset == "ios-arm64":
        return [
            f"-DCMAKE_TOOLCHAIN_FILE={ROOT / 'cmake' / 'toolchains' / 'ios.cmake'}",
            "-DCMAKE_OSX_ARCHITECTURES=arm64",
        ]
    if preset == "android-arm64":
        ndk = environment.get("ANDROID_NDK_HOME")
        if not ndk:
            raise PackageError("ANDROID_NDK_HOME is required for android-arm64")
        return [
            f"-DCMAKE_TOOLCHAIN_FILE={Path(ndk) / 'build' / 'cmake' / 'android.toolchain.cmake'}",
            "-DANDROID_ABI=arm64-v8a",
            "-DANDROID_PLATFORM=android-26",
            "-DANDROID_STL=c++_static",
        ]
    return []


def smoke_test(preset: str, prefix: Path, work: Path, environment: dict[str, str]) -> str:
    smoke = work / "smoke"
    remove_tree(smoke)
    configure = [
        "cmake",
        "-S",
        str(ROOT / "tests" / "package-smoke"),
        "-B",
        str(smoke),
        "-G",
        "Ninja",
        "-DCMAKE_BUILD_TYPE=Release",
        f"-DCYBERTEXEL_ROOT={prefix}",
        *cross_arguments(preset, environment),
    ]
    run(configure, environment=environment)
    run(["cmake", "--build", str(smoke)], environment=environment)
    if preset not in DESKTOP_PRESETS:
        return "linked"
    executable = smoke / ("cybertexel_package_smoke.exe" if preset == "windows-x64" else "cybertexel_package_smoke")
    runtime_environment = environment.copy()
    if preset == "windows-x64":
        library_path = str(prefix / "bin")
        runtime_environment["PATH"] = library_path + os.pathsep + runtime_environment.get("PATH", "")
    elif preset == "macos-universal":
        library_path = str(prefix / "lib")
        runtime_environment["DYLD_LIBRARY_PATH"] = library_path
    else:
        library_path = str(prefix / "lib")
        runtime_environment["LD_LIBRARY_PATH"] = library_path
    run([str(executable)], environment=runtime_environment)
    return "executed"


def write_manifest(prefix: Path, preset: str, smoke: str) -> None:
    version = (ROOT / "VERSION").read_text(encoding="utf-8").strip()
    manifest = {
        "schema": 1,
        "name": "CyberTexel",
        "version": version,
        "platform": preset,
        "smoke_test": smoke,
    }
    destination = prefix / "share" / "cybertexel" / "package.json"
    destination.write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def archive_tree(prefix: Path, destination: Path) -> None:
    destination.parent.mkdir(parents=True, exist_ok=True)
    if destination.exists():
        destination.unlink()
    with zipfile.ZipFile(destination, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for path in sorted(item for item in prefix.rglob("*") if item.is_file()):
            name = (Path("cybertexel") / path.relative_to(prefix)).as_posix()
            info = zipfile.ZipInfo(name, FIXED_ZIP_TIME)
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = (0o755 if os.access(path, os.X_OK) else 0o644) << 16
            archive.writestr(info, path.read_bytes())


def build_package(preset: str) -> Path:
    if preset not in SUPPORTED_PRESETS:
        raise PackageError(f"unsupported package preset: {preset}")
    environment = os.environ.copy()
    work = PACKAGE_ROOT / preset
    prefix = work / "root"
    remove_tree(prefix)
    run(["cmake", "--preset", preset], environment=environment)
    run(["cmake", "--build", "--preset", preset], environment=environment)
    run(
        ["cmake", "--install", str(ROOT / "build" / preset), "--prefix", str(prefix)],
        environment=environment,
    )
    validate_install(prefix)
    smoke = smoke_test(preset, prefix, work, environment)
    write_manifest(prefix, preset, smoke)
    version = (ROOT / "VERSION").read_text(encoding="utf-8").strip()
    destination = DIST / f"cybertexel-{version}-{preset}.zip"
    archive_tree(prefix, destination)
    return destination


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("preset", choices=sorted(SUPPORTED_PRESETS))
    return parser.parse_args()


def main() -> int:
    try:
        destination = build_package(parse_arguments().preset)
    except (PackageError, subprocess.CalledProcessError) as error:
        print(f"package failed: {error}", file=sys.stderr)
        return 1
    print(f"ok: {destination.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
