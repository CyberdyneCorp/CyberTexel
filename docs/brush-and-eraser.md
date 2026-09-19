# Brush and Eraser

`apply_brush` is the tool-level composition of the canonical painting stages.
It consumes a resolved stroke and its rejected coverage, intersects every active
paint mask, evaluates non-building or build-up deposition, and shades each
enabled layer channel against its stroke-start snapshot. The active material
must provide a raster for every enabled channel; material channels that are not
enabled on the layer are not written. Each raster declares its one-to-four
component count; material and layer counts must agree. `ColourValue` is the
normalized four-lane blend carrier, and consumers retain only the declared
number of semantic channel components.

Stroke reconstruction and coverage remain authoritative for radius, opacity,
flow, hardness, tip rotation, coordinate sampling and symmetry. Brush does not
re-resolve or reinterpret those values. Its blend mode is evaluated through the
same canonical formula used by material graphs and layer painting.
The [shared tool-parameter validator](paint-tool-parameters.md) clamps radius to
its documented bounds and makes that clamp observable through the stroke entry
point's parameter report before either tool runs.

`apply_eraser` follows the identical mask and deposition path. For each texel it
computes:

```text
result = stroke_start_value * (1 - stroke_strength)
```

The target is explicit: `layer_opacity` reduces the active layer's opacity,
while `mask` reduces mask values. Both operations are pure result builders;
invalid inputs are rejected before caller-owned layer or mask storage can be
changed.
