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

## C boundary

`ctex_paint_apply_brush` consumes caller-owned deposition from the canonical
coverage, rejection and masking pipeline. It applies the active material to
every enabled layer channel in layer order, ignores material-only channels, and
writes one caller-owned output raster per enabled channel. A count-only call
reports the exact channel and pixel counts. Every output buffer is validated
before any is written.

`ctex_paint_apply_eraser` consumes the same deposition samples and explicitly
targets either layer opacity or a mask. Alpha-discarded samples preserve their
stroke-start value. Its count-only and short-buffer behavior follows the same
atomic caller-buffer contract.
