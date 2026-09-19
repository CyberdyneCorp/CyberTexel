# Projection tool

`apply_projection` projects a pinned material image onto cached surface texels,
then passes the sampled material through the canonical paint masks, optional
rejection acceptance and stroke-start layer blending. Its result exposes the
source image samples, weights, effective strength, sampled material and final
enabled-channel output.

The mapping is an explicit variant, so settings for one projection kind cannot
silently affect another:

- `CameraProjection` transforms world positions with a finite, invertible,
  column-major view-projection matrix. OpenGL clip bounds are used: x, y and z
  are within `[-w, w]`, and positive w faces the camera. NDC x and y become
  bottom-left image UVs. The required `visible_surface` mask must contain one
  normalized value per destination texel; callers obtain it from the current
  view's depth/visibility pass. Consequently, geometry behind the visible
  surface receives no projected paint even if it shares the same camera pixel.
- `PlanarProjection` uses a finite orthonormal `PlanarProjectionFrame`. Its
  origin is the image centre, its u/v axes set orientation, and its positive
  two-dimensional extent sets the finite projected width and height. Samples
  outside that rectangle do not contribute. Each extent axis defaults to 1 and
  is clamped to `[0.000001, 1,000,000]` surface units.
- `TriplanarProjection` uses the squared components of the normalized surface
  normal as X/Y/Z plane weights. World-space plane coordinates use the supplied
  positive scale and two-dimensional offset, repeat at integer boundaries and
  blend all three samples. Material opacity participates in that weighted blend
  before masks and rejection are applied. Scale defaults to 1 and is clamped to
  `[0.000001, 1,000,000]`; each periodic offset defaults to zero and is clamped
  to `[-1, 1]`.

Finite out-of-range projection controls are resolved before sampling and every
clamp is returned in `ProjectionResult::parameter_report`. Non-finite controls,
invalid planar frames, camera matrices and masks remain refused.

Material images use the same channel-and-opacity representation as decals.
Rows are stored top-down while projection UVs are bottom-left based, and nearest
sampling is deterministic. Invalid dimensions, channel data, matrices, frames,
extents, scales, masks and opacity values are rejected before shading.
