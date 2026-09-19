# Text tool

The Text tool separates host-owned font loading from deterministic engine-owned
layout and painting. A `SuppliedFont` carries a stable identity, line metrics and
one coverage bitmap plus placement metrics for each available Unicode scalar.
This keeps CyberTexel independent of a particular font-file parser while still
making the exact loaded glyph data explicit and reproducible.

`rasterize_text` strictly decodes UTF-8: truncated sequences, invalid
continuations, overlong encodings, surrogate values and values above U+10FFFF
are refused. Every non-line-break scalar must exist in the supplied font or the
operation reports the missing scalar. LF, CR and CRLF form deterministic lines.
Tracking is expressed in em units and applies between scalars. Each line can be
left-, centre- or right-aligned within the widest line. Glyph coverage is
combined by maximum coverage, so overlapping glyphs and negative tracking do
not depend on draw order.

Tracking defaults to zero and is clamped to `[-10, 10]` em. The resolved value
is returned in `TextRaster::tracking_em`, with any clamp in its
`parameter_report`; non-finite tracking is refused.

The supplied glyph bitmaps establish raster resolution. `apply_text_decal`
interprets `TextDecalSettings::size` as surface units per em, scales the decal's
two frame axes from the laid-out width and height, and preserves the placement's
rotation, uniform scale and per-axis scale. It expands each requested material
channel over the text raster and uses glyph coverage as material opacity before
calling the ordinary decal rasterizer. Consequently text inherits canonical
paint masks, optional rejection, blend modes and stroke-start layer snapshots.

Text size defaults to 1 and is clamped to `[0.000001, 1,000,000]` surface units
per em. `TextDecalResult::size` exposes the resolved value, and its
`parameter_report` combines both layout-tracking and size clamps. Non-finite
size is refused.

The immediate operation has editable revision zero. Persistent text identity,
font-resource round-trip, tile invalidation and undo are part of roadmap item
20.4 rather than the tool-level 10.7 contract.
