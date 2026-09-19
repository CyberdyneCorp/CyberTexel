# Seam-aware filtering and padding

Surface-continuous operations use `SurfaceFilterRequest`. Every blur, smear,
derivative or mip-generation request names its base-level sampling footprint and
provides explicit surface-adjacent taps. Each tap's logical offset is validated
against that footprint. `SurfaceAdjacencyLink` records a bidirectional seam
connection and the tangent frame at both ends; it is not inferred from nearby UV
storage, so a kernel can cross a seam without sampling unrelated atlas pixels.

Tangent-space samples carry their source frame. Before filtering, each vector is
transformed through object space into the requested output frame; the filtered
result is normalized. This also handles mirrored UV islands: their opposite
bitangent handedness is part of the frame conversion instead of being treated
as a colour-channel sign convention. Tangent generation and validation remain
the mesh-map responsibility described by task 11.12.

`plan_island_padding` expands a declared footprint for every requested mip
level. For mip level `m`, a base footprint radius `r` requires
`(r + 1) * 2^m - 1` base-level gutter texels. The plan:

- assigns empty texels only when one nearest UV island owns them;
- leaves equidistant, contested gutter unassigned;
- never changes an already valid island texel; and
- reports each unsupported mip level with its required radius and sorted set of
  affected island identifiers.

`apply_island_padding` copies the nearest valid texel from the assigned island
into those unambiguous slots. A fixed two-texel dilation therefore makes no
claim about higher mip levels: callers must request and inspect a padding plan
for the filtering footprint and mip range they intend to publish.
