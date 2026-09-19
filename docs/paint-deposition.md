# Paint deposition

`<ctex/paint/deposition.hpp>` turns accepted per-stamp coverage into effective
stroke strength. Non-building is the default; build-up must be selected
explicitly.

The rejection stage emits exactly one `RejectedStampCoverage` event per
canonical resolved stamp. For continuous brushes the first stamp contributes
its tip and each later stamp contributes its incoming swept segment. A segment
is therefore geometry within one stamp event, not an extra deposition event.
Discrete-alpha brushes contribute one transformed tip per stamp.

## Non-building brushes

For each texel and stamp event `i`, with accepted coverage `c_i`, resolved
opacity `opacity_i`, and resolved flow `flow_i`, the accumulator updates:

```text
C = max(C, c_i)
S = max(S, opacity_i * flow_i * c_i)
```

Both arrays begin at zero for each stroke. Repeated overlap within one stroke
therefore cannot darken a texel beyond the strongest event. Starting another
`StrokeDepositionAccumulator` creates a fresh per-stroke mask so later document
blending can accumulate separate strokes.

## Build-up brushes

Build-up keeps deposition separate from opacity:

```text
D = 1 - (1 - D) * (1 - flow_i * c_i)
S = max(S, opacity_i * D)
```

`D` and `S` begin at zero. Opacity caps the current accumulated deposition; it
is not multiplied into `c_i`, and a lower later opacity cannot erase a stronger
earlier result.

## Batch invariance

`StrokeDepositionAccumulator::apply` accepts any contiguous partition of the
canonical event sequence. It validates a complete batch before changing state,
refuses gaps or reordering, and ignores already-applied ordinals. Hosts may
therefore split work across frames or replay a rasterized segment without
advancing flow twice. `evaluate_deposition` is the one-shot form and requires a
complete canonical sequence.

At task 9.9, `c_i` contains geometric coverage after rejection. Later masking
and alpha-tip stages can attenuate each public event value before it enters the
same formulas; opacity and flow remain separate resolved stamp properties.
