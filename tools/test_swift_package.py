from __future__ import annotations

import os
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MACOS_PREFIX = ROOT / "build" / "swift-prefix"
IOS_PREFIX = ROOT / "build" / "swift-ios-prefix"


def run(*arguments: str, env: dict[str, str] | None = None) -> None:
    subprocess.run(arguments, cwd=ROOT, env=env, check=True)


def apple_environment() -> dict[str, str]:
    environment = os.environ.copy()
    try:
        subprocess.run(
            ["xcrun", "--sdk", "iphoneos", "--show-sdk-path"],
            env=environment,
            check=True,
            capture_output=True,
            text=True,
        )
    except subprocess.CalledProcessError:
        xcode = Path("/Applications/Xcode.app/Contents/Developer")
        if not xcode.is_dir():
            raise RuntimeError("the Swift package gate requires Xcode with the iOS SDK")
        environment["DEVELOPER_DIR"] = str(xcode)
    return environment


def sdk_path(environment: dict[str, str]) -> str:
    result = subprocess.run(
        ["xcrun", "--sdk", "iphoneos", "--show-sdk-path"],
        cwd=ROOT,
        env=environment,
        check=True,
        capture_output=True,
        text=True,
    )
    return result.stdout.strip()


def install_native(preset: str, prefix: Path, environment: dict[str, str]) -> None:
    run("cmake", "--preset", preset, env=environment)
    targets = ["cybertexel_c"]
    if preset == "macos-universal":
        targets.append("cybertexel_cli")
    run(
        "cmake",
        "--build",
        "--preset",
        preset,
        "--target",
        *targets,
        env=environment,
    )
    run(
        "cmake",
        "--install",
        str(ROOT / "build" / preset),
        "--prefix",
        str(prefix),
        env=environment,
    )
    package = prefix / "lib" / "pkgconfig" / "cybertexel.pc"
    if not package.is_file() or "-lcybertexel_c" not in package.read_text(
        encoding="utf-8"
    ):
        raise RuntimeError(f"installed pkg-config metadata is incomplete: {package}")


def main() -> int:
    environment = apple_environment()
    install_native("macos-universal", MACOS_PREFIX, environment)
    environment["PKG_CONFIG_PATH"] = str(MACOS_PREFIX / "lib" / "pkgconfig")
    include = str(MACOS_PREFIX / "include")
    library = str(MACOS_PREFIX / "lib")
    run(
        "swift",
        "test",
        "--package-path",
        "swift",
        "-Xcc",
        f"-I{include}",
        "-Xlinker",
        f"-L{library}",
        "-Xlinker",
        "-rpath",
        "-Xlinker",
        library,
        env=environment,
    )
    run(
        "swift",
        "run",
        "--package-path",
        "swift",
        "-Xcc",
        f"-I{include}",
        "-Xlinker",
        f"-L{library}",
        "-Xlinker",
        "-rpath",
        "-Xlinker",
        library,
        "CyberTexelWorkflowExample",
        env=environment,
    )

    install_native("ios-arm64", IOS_PREFIX, environment)
    environment["PKG_CONFIG_PATH"] = str(IOS_PREFIX / "lib" / "pkgconfig")
    run(
        "swift",
        "build",
        "--package-path",
        "swift",
        "--target",
        "CyberTexelLinkCheck",
        "--triple",
        "arm64-apple-ios16.0",
        "--sdk",
        sdk_path(environment),
        "-Xcc",
        f"-I{IOS_PREFIX / 'include'}",
        "-Xlinker",
        f"-L{IOS_PREFIX / 'lib'}",
        env=environment,
    )
    print("ok: Swift package tested on macOS and built for iOS arm64")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
