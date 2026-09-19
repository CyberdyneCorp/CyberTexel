# Paint rejection and alpha discard

`<ctex/paint/rejection.hpp>` applies visibility and orientation policy to
geometric stroke coverage before deposition. Depth and angle rejection are
enabled by default; backface rejection is selected per operation.

## Depth rejection

Each `DepthProjectionContext` pairs every texture-space surface texel with its
top-left-origin viewport position and projected depth, plus the visible depth
buffer produced by the same transform. A contribution is occluded when its
surface depth is greater than the visible depth plus `depth_bias`. The default
bias is `1e-4`, and finite values are clamped to `[0, 1,000,000]` projected-depth
units. Projected texels outside the viewport have no comparable depth sample
and are not rejected by this test.

Depth can be disabled for an operation. Symmetric strokes must make one of two
explicit choices:

- `require_consistent_per_instance` requires a transform-consistent depth
  context for every symmetry instance.
- `disable_for_derived_symmetry` applies the base instance's depth context but
  bypasses depth rejection for derived instances.

`RejectionReport::depth_disposition` records the selected behavior so a host
cannot silently reuse an inconsistent camera buffer for mirrored or radial
stamps.

The result retains both the maximum accepted coverage raster and exactly one
ordered coverage event per resolved stamp. The latter feeds the
[deposition stage](paint-deposition.md), preserving canonical event identity
across rendering batches.

## Angle and backface rejection

Angle rejection compares the normalized interpolated surface normal with the
resolved stamp hit normal. Swept segments interpolate their endpoint hit
normals. The default minimum dot product is `0.5`, and the threshold is
clamped from `-1` through `1`.

Backface rejection uses the geometric triangle normal rather than the
interpolated shading normal. Mesh indices use counter-clockwise winding when
viewed from the front. The caller supplies one direction from each surface
texel toward the camera; a texel is front-facing only when its geometric normal
has a positive dot product with that direction.

## Alpha discard

`apply_alpha_discard` produces a write mask after deposition accumulation. The
default threshold is `0.1` for 8-bit channels and `0.004` for 16-bit and
floating-point channels. A strength below the threshold skips the output write;
the returned `retained_strength` remains byte-for-byte numerically equal to the
input accumulation so a later stamp can continue building it. Callers may
provide a custom threshold, with finite values clamped to `[0, 1]`.

`RejectedCoverageRaster::report` exposes the resolved rejection settings and
their clamp report. `alpha_discard_threshold` returns an
`AlphaDiscardThresholdResult`, and `apply_alpha_discard` propagates the same
threshold and clamp report into `AlphaDiscardResult`. Non-finite thresholds are
refused.
