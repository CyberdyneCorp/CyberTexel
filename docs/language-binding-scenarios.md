# Language-binding scenario coverage

`just test-language-binding-scenarios` runs the Python wheel, Swift package,
Rust workspace, per-binding workflow examples, surface-parity gate and this
matrix test. Platform CI runs the same constituent recipes in their supported
jobs. The matrix refuses missing, duplicate or unknown OpenSpec scenarios,
unknown recipes, recipes absent from CI, or a missing capability label.

The Python examples remain the integration harness and published visual
gallery. The Swift and Rust workflow examples independently assert document
creation, painting, material authoring, mesh-map binding, smart-material
application and export planning through their native binding surfaces.

| OpenSpec scenario | Executable evidence | Covered behavior |
| --- | --- | --- |
| All three build | `just test-python-binding`, `just test-swift-binding`, `just test-rust-binding` | Clean platform jobs build and test the wheel, macOS/iOS Swift package and both Rust crates. |
| Parity gate | `just gate-binding-parity` | Generated Python and Rust declarations and Swift's imported public header are compared with every stable C entry point. |
| Integration suite | `just examples` | The installed Python wheel drives every numbered integration example and compares deterministic committed outputs. |
| Reading a channel | `just test-python-binding` | A document read returns a C-contiguous, independently owned NumPy array with its documented dtype and shape. |
| Missing mesh map | `just test-python-binding` | A generator missing AO raises the typed missing-resource exception with the native diagnostic. |
| Clean environment install | `just test-python-binding`, `just examples` | An isolated wheel containing the native library imports with only declared runtime dependencies and runs the gallery workflows. |
| Resolving from another repository | `just test-swift-binding` | SwiftPM consumes only installed headers, library and relocatable pkg-config metadata, then links macOS and iOS targets without source-tree include paths. |
| Handle lifetime | `just test-swift-binding` | Copied Swift document values share one storage owner and destroy the native handle exactly once. |
| Unsafe is contained | `just test-rust-binding` | The audit confines safe-wrapper `unsafe` to `ffi.rs`; raw operations and the full workflow example live in `cybertexel-sys`. |
| Threading contract in the type system | `just test-rust-binding` | A compile-fail test rejects `Document: Sync`, while a unit assertion retains the permitted `Send` contract. |
| A wgpu host in Rust | `just test-rust-binding` | The safe host route returns owned WGSL and a pass plan, accepts externally executed completion and retains the resulting logical resource without readback. |
| Mismatched native library | `just test-python-binding`, `just test-swift-binding`, `just test-rust-binding` | Every idiomatic layer injects an incompatible major and reports both its expected and loaded native versions. |
| Example gate | `just examples`, `just test-swift-binding`, `just test-rust-binding` | Python's numbered examples and the complete Swift/sys-Rust workflows execute in CI and assert their results. |
| A returned view's lifetime | `just test-python-binding`, `just test-swift-binding`, `just test-rust-binding` | Public binding results are owned native-language values; pending readback is inaccessible and copied output remains valid after native handles are released. |
