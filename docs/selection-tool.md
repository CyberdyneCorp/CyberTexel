# Selection tool

`<ctex/paint/selection.hpp>` maps interactive screen and polygon selections to
binary texture-space masks:

- rectangle and lasso modes use the exact, clipped screen-region queries from
  the revision-keyed picking index as a broad phase, then project cached surface
  texels and select only those inside the screen polygon;
- triangle mode selects the picked texel's source triangle;
- UV-island mode expands to every triangle in the picked cached island; and
- connected-by-angle mode traverses declared triangle adjacency without
  crossing the configured normal-angle limit.

All modes return dimensions, owned normalized values, selected texel and
triangle counts, and the selection kind. Screen modes also preserve BVH
traversal counters. A screen selection refuses surface maps from an older mesh
revision. Invalid dimensions, coverage, identities, region geometry, picks,
and topology are refused before a result is returned. The connected-polygon
angle shares Fill's 45-degree default and `[0, 180]` bounds. Its resolved value
drives expansion, clamp information is propagated through
`SelectionResult::parameter_report`, and non-finite angles are refused.

`SelectionResult::as_paint_restriction()` supplies an active restriction to
`PaintMaskInputs`. `store_selection_mask()` validates and copies the region into
an independent `StoredSelectionMask`, whose view can be retained as a mask
source for as long as the stored object lives.
