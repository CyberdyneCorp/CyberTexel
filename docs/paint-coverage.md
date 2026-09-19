# Paint coverage

`ctex::paint` separates texture-space geometry coverage from rejection,
deposition, blending, and masking. The public contract is declared in
`<ctex/paint/coverage.hpp>`.

## Texture-space surface raster

`rasterize_texture_space` rasterizes mesh UV triangles directly into a requested
tile. It interpolates caller-supplied world-space position and normal for every
covered texel; it does not accept a camera, so off-screen and occluded surfaces
remain candidates until later rejection stages. Texel rows use a top-left
origin while UV coordinates retain their conventional increasing-up direction.
If UV triangles overlap, the lowest triangle ordinal owns the texel
deterministically.

## Geometric brush coverage

`evaluate_stroke_coverage` consumes a validated `ResolvedStroke` and returns one
geometric coverage value per surface texel. Continuous brushes evaluate both
resolved stamps and every branch-local swept segment, interpolating radius and
hardness along a segment so fast input cannot leave gaps. Discrete-alpha brushes
evaluate only their resolved tips; rotation and elongation transform each tip,
and no segment is synthesized between tips.

Falloff follows the paint-engine specification exactly. With normalized radial
distance `t`, the soft region uses:

```text
t2 = clamp((t - hardness) / (1 - hardness), 0, 1)
coverage = 1 - t2 * t2 * (3 - 2 * t2)
```

Hardness `1` produces a hard edge. Coverage here is geometry only: opacity,
flow, masks, rejection, and deposition are intentionally applied by later paint
stages.

## Material coordinates

`material_coordinates` returns weighted projections for the requested mode:

- UV returns the rasterized surface UV with weight one.
- Triplanar returns YZ, XZ, and XY world-axis projections. Their weights are the
  squared components of the normalized surface normal.
- Planar projects the surface position relative to a caller-supplied origin onto
  caller-supplied orthonormal U and V axes.

The triplanar result keeps all three projections explicit so a material sampler
can sample each projection before combining values by the returned weights.
