# Paint masking

`<ctex/paint/masking.hpp>` intersects every active restriction before accepted
per-stamp coverage enters deposition. `PaintMaskInputs` has distinct slots for:

- every mask attached to the active layer;
- colour-ID selection;
- geometry or polygon-fill selection;
- rectangular or lasso screen selection; and
- UV-island selection.

Each input is a normalized per-texel weight. Missing inputs contribute one;
active inputs multiply together, so a zero in any input excludes that texel and
soft masks attenuate coverage predictably. Multiple active layer masks are
applied independently. `active_input_count` reports how many restrictions were
combined.

`apply_paint_masks` copies the accepted rejection result, multiplies every
canonical stamp event by the combined mask, and rebuilds maximum aggregate
coverage from those events. The source remains unchanged. The masked events
then become `c_i` for [paint deposition](paint-deposition.md), keeping masking
separate from opacity and flow.

Dimensions and all values are validated before output is returned. The function
also verifies that the incoming aggregate equals the maximum of its canonical
events, preventing a mismatched aggregate from bypassing a restriction.
