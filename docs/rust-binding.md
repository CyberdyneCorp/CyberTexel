# Rust binding

The Cargo workspace contains two crates. `cybertexel-sys` builds and links the
native shared library and exposes raw C-compatible declarations.
`cybertexel` is the safe wrapper; its only `unsafe` code is isolated in the
documented `src/ffi.rs` boundary and a checked repository gate enforces that
layout.

The safe crate checks ABI major version zero before handle creation, represents
native failures as `Error::Native` with typed result, diagnostic code and text,
and destroys each owned document through `Drop`. `Document` requires mutable
access for mutations and is `Send` but intentionally not `Sync`, matching
the C contract that callers must serialize access to one document. A compile-fail
documentation test enforces the negative `Sync` guarantee, while a unit test
asserts that the type remains `Send`.

The sys build accepts `CYBERTEXEL_LIBRARY_DIR` for development against an
already-built shared library. Without it, the build script configures and builds
the native C ABI from the checked-out source.

`cybertexel-sys/src/lib.rs` is generated from the public C header with bindgen
0.72.1 and committed so crate consumers do not need libclang. Run
`just generate-rust-sys` after changing the C header. The parity gate checks both
the complete operation set and the recorded header digest.

The safe crate implements the shared [host-execution and transport
workflow](binding-host-transport.md). Owned shader artifacts and pass-plan
strings feed typed logical submissions. `Rc`-owned pool storage, consuming
snapshot-to-readback conversion, boxed tile buffers and `Drop` encode the native
lifetime rules without exposing raw pointers. Pending readback output returns a
typed error. The [binding parity gate](binding-parity.md) compares every C
operation with the public `cybertexel-sys` declarations. The raw Rust surface is
complete, and task 14.13 is complete across all three official bindings.

Run formatting, Clippy, unit tests, compile-fail tests and the unsafe-boundary
audit with:

```sh
just test-rust-binding
```
