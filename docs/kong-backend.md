# Kong shader backend

CyberTexel vendors ArmorPaint's minikong compiler in `thirdparty/kong` and
exposes it through `ctex::emit::KongContext`. Each wrapper owns isolated parser,
intermediate-representation, token-cache, built-in-type, and target-backend
state. Separate wrappers can compile concurrently—even for different targets;
repeated calls on one wrapper start from a fresh vendor context and produce
deterministic output.

`KongContext::compile` accepts WGSL, MSL, SPIR-V, or HLSL. WGSL and HLSL return
separate vertex and fragment text, MSL returns one source module containing both
entry points, and SPIR-V returns separate vertex and fragment word vectors.
The retained backend lowers both 2D and cube texture declarations for all four
targets; preview environment fixtures exercise cube sampling and explicit LOD.
`supported_shader_targets` exposes the stable available-target inventory. An
unknown request fails with a diagnostic that names the request and every
available target. SPIR-V fixtures are checked by `spirv-val` in CI.

The graph emitter currently produces the deterministic WGSL expression program
documented in [graph-emission.md](graph-emission.md). Feature-gated
[layer-stack emission](feature-gated-emission.md) already compiles complete
multi-pass programs for all four targets. [Preview emission](preview-shading.md)
compiles lit and per-channel programs for the same targets. Complete
[material-graph emission](graph-emission.md) now wraps reachable expressions and
their pass plan for every target. The
concurrency fixture emits one layer stack per target simultaneously and compares
every shader and pass plan with serial output. Generated
programs and plans can be retained by the [emission cache](emission-cache.md).

The vendored C API is intentionally private to the build. Library consumers use
`ctex::emit::KongContext` from `ctex/emit/kong_context.hpp`.
