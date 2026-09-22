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
the native C ABI from the checked-out source. Tasks 14.12–14.13 add host
transport and mechanically enforce complete C-operation parity.

Run formatting, Clippy, unit tests, compile-fail tests and the unsafe-boundary
audit with:

```sh
just test-rust-binding
```
