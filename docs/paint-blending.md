# Stroke-start snapshot blending

`<ctex/paint/blending.hpp>` is the paint shading stage. A
`StrokeSnapshotBlender` copies the channel pixels at stroke start and evaluates
every later shade against that immutable copy. Interactive updates never read
the partially painted result back as their base.

The stage consumes normalized RGBA paint values and the effective per-texel
strength produced by [paint deposition](paint-deposition.md). For a mode result
`B(base, paint)` and strength `S`, output is the shared blend formula evaluated
as:

```text
output = mix(stroke_start_snapshot, B(stroke_start_snapshot, paint), S)
```

Multiply therefore computes the snapshot multiplied by the paint colour, then
interpolates that result from the same snapshot by `S`.

Painting uses `ctex::graph::blend_colour`, the canonical CPU formula surface
also used by the portable Blend node. All twenty declared modes are available:
Normal, Darken, Multiply, Color Burn, Lighten, Screen, Color Dodge, Add,
Overlay, Soft Light, Linear Light, Difference, Exclusion, Subtract, Divide, Hue,
Saturation, Color, Value, and Pass Through.

Construction copies caller storage, so later host changes cannot alter the
snapshot. Each `shade` call validates the entire paint and strength arrays,
builds replacement output separately, and publishes it only on success. A new
stroke creates a new blender from the previously committed result, allowing
separate strokes to accumulate while preventing self-feedback within a stroke.
