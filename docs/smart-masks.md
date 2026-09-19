# Smart masks and independent instances

`SmartMaskPreset` is a reusable, derived-only mask definition built from the
same canonical stack and exposed-parameter model as a smart material. Its stack
has exactly one root mask; every later mask, generator or filter is a descendant
of that root. Layers, groups, model-specific pixels, multiple roots and
graph-free definitions are refused.

The preset has its own `CTEX_SMART_MASK` schema and embeds the canonical smart
material fragment. This keeps its asset kind explicit while reusing graph,
binding and validation semantics. Smart-mask schema 1 is current; future outer
versions are refused by name. Its embedded smart-material definition migrates
schemas 1–5 using the documented smart-material defaults before the complete
mask is validated, so no partially migrated mask is returned.

## Instantiation

`instantiate_smart_mask()` attaches a deep-copied definition to a named layer or
group target. It records a stable instance identity, target identity and kind,
and the origin preset identity and schema version. Every exposed parameter is
reset to its declared default during instantiation.

Each `SmartMaskInstance` owns both its fragment graphs and an ordered current
parameter-value set. `set_smart_mask_parameter_value()` updates all bindings in
a copy, synchronizes that instance's current value, validates the result, and
then publishes it atomically. Editing one instance therefore cannot change the
source preset or another instance created from it. Missing, mistyped and
out-of-range parameters leave the instance unchanged.

[`TextureSet::apply_smart_mask()`](preset-application.md) inserts that independent
instance against an applied layer or group. It records the outer mask origin,
publishes no partial state on refusal, and is removed as one undo step without
removing its target.
