# Kong shader backend

CyberTexel vendors ArmorPaint's minikong compiler in `thirdparty/kong` and
exposes it through `ctex::emit::KongContext`. Each wrapper owns isolated parser,
intermediate-representation, token-cache, built-in-type, and WGSL traversal
state. Separate wrappers can compile concurrently; repeated calls on one wrapper
start from a fresh vendor context and produce deterministic output.

The public wrapper currently exposes WGSL only. HLSL, Metal, and SPIR-V source
files are retained in the vendor tree for provenance but are not linked into the
library. Task 6.11 will add their context-owned adapters and public target
selection. Graph-to-Kong emission, result naming, and fan-out remain task 6.9.

The vendored C API is intentionally private to the build. Library consumers use
`ctex::emit::KongContext` from `ctex/emit/kong_context.hpp`.
