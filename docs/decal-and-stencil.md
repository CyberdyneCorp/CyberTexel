# Decal and Stencil tools

Decal projects a pinned material image through a frame derived from the stored
surface position and normal. `place_decal_on_surface` captures both directly
from a covered picked texel. Rotation changes the frame around that normal;
uniform and per-axis scale change its projected extent. Material opacity,
ordinary paint masks and explicit rejection acceptance are intersected before
the shared enabled-channel shading path.

Decal rotation defaults to zero and is clamped to `[-2π, 2π]`. Uniform scale
and both positive per-axis scales default to 1 and are clamped to
`[0.000001, 1,000,000]` surface units. `DecalPlacement`, `DecalFrame`, retained
editable entries and `DecalRasterResult` preserve the resolved transform and
its clamp report, so placement, editing and immediate rasterization use the
same decision. Non-finite transform values are refused.

`retain_editable_decal` creates an owned editable value containing stable entry
and material-content identities, the original surface pick, placement
parameters and a pinned material snapshot. `edit_decal_transform` returns a new
revision while preserving the stored position and normal, making the preceding
value suitable for later undo integration. Editing does not rasterize.
`rasterize_editable_decal` is the explicit operation that produces pixels.
`rasterize_decal` exposes the same path for immediate callers such as Text and
reports editable revision zero.

This implements the retained tool-level entry required by roadmap item 10.5.
Project serialization, document tile invalidation and undo/redo integration
remain scheduled together in editable-authoring task 20.4.

Stencil produces a paint restriction from an opacity image anchored in screen
coordinates. Its position, rotation and per-axis scale are evaluated only
against caller-supplied screen positions, independent of model coordinates or
camera motion. Inversion complements the opacity, including the zero-opacity
region outside the image. `apply_stencil` intersects that result with canonical
rejection and paint masks before deposition and channel shading.

Stencil position and rotation default to zero. Each position axis is clamped
to `[-1,000,000, 1,000,000]` screen units and rotation to `[-2π, 2π]`.
Positive scale axes default to 1 and are clamped to
`[0.000001, 1,000,000]`. `StencilMaskResult` exposes the resolved transform and
every clamp; `apply_stencil` carries that result unchanged. Non-finite
transform values are refused.

The public C boundary exposes stencil resolution as
`ctex_paint_resolve_stencil_mask`. Its caller-owned normalized result plugs
directly into `ctex_paint_evaluate_tile_deposition` as a screen-selection mask,
then into `ctex_paint_apply_brush`. This composition preserves per-stamp
build-up behavior while exposing resolved transforms, inversion and every clamp.
