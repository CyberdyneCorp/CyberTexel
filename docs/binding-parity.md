# Binding parity gate

`just gate-binding-parity` compares the operations declared by the stable C
header with the raw operation surface of each official binding. It reports every
missing operation by C name and binding, so adding a C entry point without all
three language routes fails mechanically.

- Python operations count only when `_native.py` registers a complete ctypes
  signature. Merely resolving an untyped symbol from the shared library is not a
  supported operation.
- Rust operations count when the `cybertexel-sys` crate publishes an `extern
  "C"` declaration. The safe crate continues to isolate its higher-level unsafe
  calls in the documented boundary modules.
- Swift imports the complete public header through the `CyberTexelC` system
  module. Its normal package and link tests verify that the imported surface is
  usable on macOS and iOS.

The first live audit covers 347 C operations. Swift reaches the full raw
surface; Python reaches 24 and is missing 323; Rust reaches 35 and is missing
312. Task 14.13 remains open until both missing sets reach zero and all binding
test suites pass.
