# Feature-gated layer-stack emission

`ctex/emit/feature_emission.hpp` compiles a flattened layer stack into target
shader artifacts and a validated device-independent pass plan. The caller
supplies a `DeviceFeatureSet` containing the total per-stage binding budget,
maximum texture dimension, supported texture formats, floating-point filtering
support, and compute availability. No GPU object or backend header is involved.
When an executor has been selected, the
[executor capability seam](executor-capabilities.md) supplies this exact set to
the request rather than relying on a second caller-maintained device record.

Layer inputs are ordered bottom-to-top and use premultiplied linear RGBA. The
emitted fragment code applies premultiplied source-over in that order. A pass
uses one shared sampler binding; every sampled layer or carried intermediate
uses one texture binding. Therefore eight layers fit one pass at a binding
budget of nine. When they do not fit, the first pass consumes up to
`budget - 1` layers and each later pass consumes the preceding intermediate
plus up to `budget - 2` new layers. A budget that cannot make forward progress
is refused by name.

Every split is visible in the pass plan. Intermediate textures have stable
logical IDs derived from the emission identity, each later pass depends on its
predecessor, and the carried texture is binding zero. Resource generations,
accesses, lifetimes, render targets, vertex layout, draw command, and actual
target entry-point names are all explicit. Repeating the same request produces
byte-identical shaders and an equal pass plan.

Feature checks happen before compilation:

- every input and output format must appear in the declared supported set;
- every extent must fit the maximum texture dimension;
- every pass must fit the declared binding budget;
- if linear sampling of a floating-point input or intermediate was requested
  but unsupported, emission uses nearest sampling and returns the structured
  `nearest_float_sampling` workaround.

Layer-stack composition uses render passes, so `compute_used` is false even
when the declared device supports compute. Compute availability remains part of
the feature-set contract for later graph operations that can select a compute
path.

WGSL, MSL, SPIR-V, and HLSL are produced through the isolated
[Kong backend](kong-backend.md). Generated SPIR-V layer-stack fixtures are
validated by `spirv-val` in CI. Complete emissions can be retained by the
[emission cache](emission-cache.md), keyed by this request's full content,
target, and normalized feature set. This API consumes a flattened stack; the
editable layer document, blend-mode catalogue, masks, and group semantics
remain owned by texture-document tasks 3.3–3.7.
