# Release packaging

CyberTexel produces versioned ZIP archives for Linux x86-64, macOS universal,
Windows x86-64, iOS arm64 and Android arm64-v8a. Every archive has one
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
`share/cybertexel/package.json`. CI runs every platform recipe and retains the
resulting archives as workflow artifacts.

ZIP entry order, timestamps and permissions are normalized. Repeating the
archive step over identical installed bytes therefore produces identical ZIP
bytes; compiler and linker reproducibility is evaluated separately by the
release reproducibility gate.
