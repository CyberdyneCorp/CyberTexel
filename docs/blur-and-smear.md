# Blur and Smear tools

Blur and Smear operate exclusively on the enabled layer's immutable
stroke-start snapshot. Neither tool samples its partially updated output, so a
stroke that crosses its own path cannot feed earlier writes into later ones.

Blur performs two explicit passes with a configurable integer radius: a
horizontal pass over the snapshot followed by a vertical pass over the
horizontal result. `BlurNeighborhood` supplies the surface-aware samples and
tangent frames for each destination texel. This lets filtering cross a UV seam
through mesh adjacency instead of accidentally sampling unrelated UV-neighbor
pixels.

Smear uses one upstream `SmearMapping` per destination texel. Its offset records
the resolved stroke direction, while the configurable strength controls how far
the upstream snapshot value is blended into the destination. Strength is
multiplied by canonical stroke deposition after rejection and paint masks.

Both tools declare their `SamplingFootprint`. Scalar and colour channels use
normalized surface-filter weights. The `pbr.normal` channel is decoded to a
unit tangent-space vector, transformed through the supplied frames at seams,
filtered, renormalized and encoded again.

Blur radius defaults to 1 and is clamped to `[1, 4096]`. Smear strength
defaults to 0.5 and is clamped to `[0, 1]`; each footprint radius defaults to 1
and is clamped to `[0, 4096]`, while an all-zero footprint is refused. The
resolved values drive filtering and are returned with any clamps in the tool
result's `parameter_report`. Non-finite strength is refused.
