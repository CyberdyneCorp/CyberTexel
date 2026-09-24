# Release packaging

The first release gates versioned ZIP archives for Linux x86-64, macOS
universal and iOS arm64. Windows x86-64 and Android arm64-v8a are tracked in
the active `windows-android-release` OpenSpec change. Every archive has one
`cybertexel/` root and contains:

- `include/ctex/capi.h` and the generated `version.h`;
- the platform C ABI library and its required static support libraries;
- pkg-config metadata where the platform supports it;
- `LICENSE`, `THIRD_PARTY_NOTICES.md` and a package manifest under
  `share/cybertexel/`;
- the headless CLI on desktop platforms.

Run `just package-native` for the current desktop, or one of
`package-linux`, `package-macos`, `package-windows`, `package-ios` and
`package-android`. Archives are written under `dist/`. Android requires
`ANDROID_NDK_HOME`; iOS requires Xcode and its iPhoneOS SDK.

The package builder stages a clean CMake install and refuses missing headers,
libraries, licence or attribution. It then configures a standalone consumer
project against only that install. Desktop consumers are linked and executed;
mobile consumers are cross-compiled and linked because their binaries require a
device. The archive records that disposition in
`share/cybertexel/package.json`. CI runs and retains archives for the three
first-release platforms. Windows and Android package and validation work remains
in the active follow-up change.

The `verify-first-release-assets` CI job downloads the three package artifacts
from the same run and invokes `just gate-release-assets`. That gate checks each
ZIP's integrity, required contents, version, platform and smoke-test manifest,
and rejects extra platforms. It prints SHA-256 checksums for the release notes.
The initial release tag is `v0.1.0`, matching `VERSION`.

To publish, confirm that the OpenSpec CI workflow and the named reference-device
workflow both pass for the same commit. Download the Linux, macOS and iOS
artifacts from that OpenSpec run into one directory and run
`just gate-release-assets <directory>`. Tag that commit as `v0.1.0`, create the
GitHub release using `docs/releases/v0.1.0.md` as its notes, and upload only
those three verified ZIPs. Verify the published tag, asset names, sizes and
SHA-256 digests against the downloaded files.

ZIP entry order, timestamps and permissions are normalized. Repeating the
archive step over identical installed bytes therefore produces identical ZIP
bytes; compiler and linker reproducibility is evaluated separately by the
release reproducibility gate.
