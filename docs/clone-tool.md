# Clone tool

Clone copies enabled channels from an immutable source snapshot through the
canonical stroke coverage, rejection, masking, deposition and stroke-start
blending stages. The source is stored independently in `CloneSourceState`, so a
host can set or clear it without beginning a paint operation.

Both modes capture their mapping at stroke start:

- **Aligned** computes one offset from the source UV and destination anchor UV,
  then adds that constant offset to every destination texel UV.
- **Fixed** samples the source anchor UV for every accepted destination texel.

Sampling uses deterministic nearest-texel selection in the destination's UV
tile. Samples outside that tile do not contribute; edge texels are never
silently clamped. The source and destination must name the same texture set.
Cross-set attempts are refused before shading, with both stable texture-set
identities in the diagnostic.

The caller supplies the source snapshot explicitly. Clone never reads from its
partially updated output, so crossing the stroke's own path cannot feed newly
cloned pixels back into later samples.
