# Paint seam dilation

`<ctex/paint/seam_dilation.hpp>` provides the UV-border extrapolation shared by
painting and export. `dilate_uv_seams` accepts a one-to-four-component floating
point raster plus the binary geometry coverage map produced by the
[surface-map cache](paint-surface-cache.md). Its radius defaults to two texels;
zero returns the source unchanged, and larger values are clamped to 4,096.
Direct dilation returns the resolved radius and clamp report. Deferred stroke
dilation, preview finalization and bounded-work planning use the same resolver
and expose the same decision through their output or session report.

For every uncovered texel within the Euclidean radius, the operation finds the
nearest covered texel. Equal-distance ownership is deterministic in row-major
order. It then searches farther into the covered region along that direction,
estimates the directional gradient, and extends the value by the target's
distance. All components are extrapolated independently from the immutable
source raster, so generated border values never feed later samples and a
linear gradient remains linear through the gutter. Values are finite-checked
but not clamped, preserving HDR and signed channel data.

An island only one texel thick has no interior sample from which to derive a
gradient. That case uses the mathematically constant, zero-gradient
extrapolation and increments `zero_gradient_texel_count`; it is never silently
reported as directional extrapolation. Covered texels are never overwritten.

`DeferredStrokeDilation` owns the stroke-end scheduling contract. A host stages
the latest raster and coverage for each dirtied `UvTileCoordinate` as a stroke
crosses frames. Re-staging a coordinate replaces its earlier pixels rather than
adding another pass. `provisional_preview` exposes those undilated pixels with
the explicit `provisional` state. `finish` processes every staged tile once,
marks the output `final`, and is idempotent so a repeated finalization request
returns the same result without another pass. No tile can be staged afterward.
This final output is shared by the
[paint preview and commit path](paint-preview.md), so dilation is included in
preview-to-commit parity.
