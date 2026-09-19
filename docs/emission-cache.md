# Emission cache

`ctex/emit/emission_cache.hpp` provides caches for the three emission products
currently implemented:

- `GraphEmissionCache` stores deterministic WGSL expression programs for a
  standalone graph or a material in a graph workspace.
- `LayerStackEmissionCache` stores the complete target shader set, workaround
  report, and validated pass plan produced by feature-gated layer-stack
  emission.
- `PreviewEmissionCache` stores lit previews and unlit channel inspections,
  partitioned by the full preview resource contract and display mode.

Cache keys retain canonical content bytes rather than a truncated digest, so a
hash collision cannot return an unrelated program. Graph keys contain the
canonical graph serialization, selected material and group definitions for a
workspace, and the declarative host-node registry semantics. A host-node
callback whose behavior changes must receive a new node-type version, as
required by the versioned registration contract. Layer-stack keys contain the
ordered layers, complete logical texture declarations, output, stable identity,
and sampling request. Preview keys additionally contain ordered channel
semantics, component counts, environment resources, analytic-light count,
geometry count, and whether the result is lit or inspecting a named channel.

Both key types additionally contain the requested shader target and every
`DeviceFeatureSet` field: binding budget, maximum texture dimension, supported
texture formats, floating-point filtering, and compute availability. Supported
formats are treated as a set, so ordering and duplicate declarations do not
create false misses. Even a feature that does not alter a particular generated
program creates a distinct entry when its value changes.

An emission returns an immutable shared result plus `cache_hit`. A hit therefore
returns the exact stored source and pass plan without invoking code generation.
Entry, hit, and miss counts are observable through `statistics`; `clear`
removes entries and resets those counters. A failed producer increments the
miss count but is never inserted, so correcting an invalid request can retry
normally.

The cache lock protects lookup, insertion, statistics, and clearing. Expensive
code generation happens outside that lock, which avoids serializing unrelated
misses. A dedicated fixture starts four distinct graph emissions together
through one cache and verifies each result byte-for-byte against serial
emission. It also runs simultaneous WGSL, MSL, SPIR-V, and HLSL layer-stack
emissions through a shared cache and compares their complete shaders and pass
plans with serial results.

Graph expression emission currently supports WGSL and refuses other targets
before lookup. Complete layer-stack emissions are cached independently for
WGSL, MSL, SPIR-V, and HLSL, as are [preview and inspection
shaders](preview-shading.md). Complete graph entry-point integration remains
scheduled for task 6.16.
