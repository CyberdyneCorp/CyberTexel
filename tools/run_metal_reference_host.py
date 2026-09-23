#!/usr/bin/env python3
"""Build the mobile MSL reference host, run it on macOS and build it for iPad.

`build-packaging` requires desktop and mobile reference hosts that exercise the
host-executed route on real device APIs and are built in CI. Actual-device runs
are release gates: a runner without a usable Metal device reports the run as
unmeasured rather than passing it.
"""

from __future__ import annotations

import importlib.util
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
PACKAGE = ROOT / "hosts" / "ipad-metal"
REPORT = ROOT / "build" / "reference-hosts" / "mobile-msl.json"
UNMEASURED = 3


def swift_package_driver():
    path = ROOT / "tools" / "test_swift_package.py"
    spec = importlib.util.spec_from_file_location("ctex_swift_package", path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"could not load the Swift package driver from {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def main() -> int:
    driver = swift_package_driver()
    environment = driver.apple_environment()

    driver.install_native("macos-universal", driver.MACOS_PREFIX, environment)
    environment["PKG_CONFIG_PATH"] = str(driver.MACOS_PREFIX / "lib" / "pkgconfig")
    include = str(driver.MACOS_PREFIX / "include")
    library = str(driver.MACOS_PREFIX / "lib")
    macos_flags = [
        "-Xcc", f"-I{include}",
        "-Xlinker", f"-L{library}",
        "-Xlinker", "-rpath", "-Xlinker", library,
    ]
    driver.run(
        "swift", "build", "--package-path", str(PACKAGE), "-c", "release",
        *macos_flags, env=environment,
    )

    REPORT.parent.mkdir(parents=True, exist_ok=True)
    completed = subprocess.run(
        [
            "swift", "run", "--package-path", str(PACKAGE), "-c", "release",
            *macos_flags, "cybertexel-host-metal", "--report", str(REPORT),
        ],
        cwd=ROOT,
        env=environment,
        check=False,
    )
    if completed.returncode == UNMEASURED:
        print(
            "unmeasured: no Metal device on this runner; the host built but did not run",
            file=sys.stderr,
        )
    elif completed.returncode != 0:
        return completed.returncode

    # The iPad is the named mobile reference device. CI cross-builds and links
    # the same source for it; running it is a device gate, not a CI step.
    driver.install_native("ios-arm64", driver.IOS_PREFIX, environment)
    environment["PKG_CONFIG_PATH"] = str(driver.IOS_PREFIX / "lib" / "pkgconfig")
    driver.run(
        "swift", "build", "--package-path", str(PACKAGE), "-c", "release",
        "--triple", "arm64-apple-ios16.0",
        "--sdk", driver.sdk_path(environment),
        "-Xcc", f"-I{driver.IOS_PREFIX / 'include'}",
        "-Xlinker", f"-L{driver.IOS_PREFIX / 'lib'}",
        env=environment,
    )
    if completed.returncode == UNMEASURED:
        print("ok: mobile MSL reference host built for macOS and iPad; macOS run unmeasured")
        return UNMEASURED
    print("ok: mobile MSL reference host ran on macOS Metal and built for iPad")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
