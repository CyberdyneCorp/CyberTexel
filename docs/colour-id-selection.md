# Colour-ID selection

`<ctex/paint/colour_id.hpp>` converts a colour-ID image and a picked colour into
an owned binary texel selection. Matching uses Euclidean distance in linear
RGB; alpha is validated but does not identify a region. The caller supplies a
finite, non-negative tolerance in the same linear colour units.

`ColourIdSelection::status` explicitly distinguishes `matched` from `empty`,
and `selected_texel_count` reports the selected area. A tolerance of zero means
exact RGB equality. It is never widened automatically, so an ID map altered by
filtering can correctly return an empty selection.

The result exposes the same owned selection through three named views:

- `as_paint_restriction()` plugs into `PaintMaskInputs::colour_id_selection`;
- `as_mask_source()` supplies a persistent mask consumer; and
- `as_visibility_filter()` supplies a viewport or host visibility consumer.

All views contain one normalized value per map texel. The result owns those
values; a view remains valid until its result is moved, changed, or destroyed.
Zero-sized maps, mismatched storage, non-finite colours, and negative or
non-finite tolerances are refused before a result is returned.
