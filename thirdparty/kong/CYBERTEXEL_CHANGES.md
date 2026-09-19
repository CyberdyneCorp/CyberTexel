# CyberTexel changes to minikong

This directory is copied from ArmorPaint revision
`c5ccdf27818a36e67decb691009a3db457d59ab3`, under `base/sources/kong`.
ArmorPaint identifies that tree as minikong derived from Kongruent revision
`1b0f3b70673122e3b20701cdb434781a065555d7`.

CyberTexel changes:

- Removed the dependency on ArmorPaint's `iron_system.h` logging interface.
- Moved the parser, intermediate representation, token cache, built-in type IDs,
  and WGSL, HLSL, Metal, and SPIR-V backend traversal state into an opaque
  `kong_context`.
- Made stb_ds hash seeding thread-local and reset it for each context so separate
  compiler contexts may run concurrently with deterministic output.
- Added `kong_context.h`, the narrow C API used by CyberTexel, with explicit
  text, unified-module, and binary artifact ownership.
- Used the compiler-recognized `__typeof__` spelling in stb_ds so its C99
  literal-address helper remains available under strict C11 compilation.

All four retained backends are compiled and independently context-isolated.
