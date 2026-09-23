# Swift binding

The `CyberTexel` Swift package supports macOS 13 and iOS 16 or newer. Its
`CyberTexelC` system-library target discovers an installed CyberTexel C ABI
through `cybertexel.pc`; consumers do not provide paths into the CyberTexel
source tree. CMake installs the shared desktop library, static iOS library,
public headers and relocatable pkg-config metadata into the selected prefix.
The desktop-only headless CLI is excluded from the iOS target and install set.

The Swift layer checks ABI major version zero before creating a document. It
uses value types for versions, texture sets and documents, optionals only while
receiving fallible C pointers, and `CyberTexelError` for native result codes,
diagnostic codes and diagnostic text. A document value shares a private
reference-counted storage owner when copied. That owner calls
`ctex_document_destroy` exactly once when the last Swift value leaves scope;
there is no public raw handle and no manual close operation.
The package tests exercise the incompatible-ABI path directly and require its
typed error to retain both the expected major and complete native version.

The package also exposes the shared [host-execution and transport
workflow](binding-host-transport.md). Swift-owned shader artifacts and pass-plan
strings feed logical submissions; completion preserves host-resident results.
Reference types retain snapshot and readback handles plus their allocated tile
buffers until destruction. Pending output throws instead of exposing bytes.
The [binding parity gate](binding-parity.md) verifies that the system module
continues to import the complete public C header; the macOS and iOS package tests
then compile and link that imported surface. Together with the generated Python
and Rust raw layers, this completes task 14.13.

Build and test the macOS package, then link its native and Swift layers into an
iOS arm64 check executable, with:

```sh
just test-swift-binding
```

The gate requires full Xcode because it uses XCTest and the iOS SDK.
