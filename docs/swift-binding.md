# Swift binding

The `CyberTexel` Swift package supports macOS 13 and iOS 16 or newer. Its
`CyberTexelC` system-library target discovers an installed CyberTexel C ABI
through `cybertexel.pc`; consumers do not provide paths into the CyberTexel
source tree. CMake installs the shared desktop library, static iOS library,
public headers and relocatable pkg-config metadata into the selected prefix.

The Swift layer checks ABI major version zero before creating a document. It
uses value types for versions, texture sets and documents, optionals only while
receiving fallible C pointers, and `CyberTexelError` for native result codes,
diagnostic codes and diagnostic text. A document value shares a private
reference-counted storage owner when copied. That owner calls
`ctex_document_destroy` exactly once when the last Swift value leaves scope;
there is no public raw handle and no manual close operation.

The initial task-14.10 surface exposes document and texture-set creation and
enumeration. It does not return borrowed native buffers. Tasks 14.12–14.13 add
host transport and enforce complete C-operation parity.

Build and test the macOS package, then link its native and Swift layers into an
iOS arm64 check executable, with:

```sh
just test-swift-binding
```

The gate requires full Xcode because it uses XCTest and the iOS SDK.
