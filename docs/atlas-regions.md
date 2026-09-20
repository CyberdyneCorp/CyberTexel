# Atlas regions

`TextureDocument::create_atlas()` groups existing texture sets into one
export-time texture. An atlas has a stable caller-supplied identifier, display
name, output dimensions, and one integer pixel rectangle per member. Rectangles
use a top-left origin and half-open bounds: `[x, x + width)` by
`[y, y + height)`.

Atlas creation is atomic. Every rectangle must be non-empty and inside the
atlas, members must exist and be unique, rectangles must not overlap, and a
texture set may belong to only one atlas. Invalid declarations leave the
document unchanged. The document enumerates atlases and their regions in stable
identifier order.

`export_source_catalogue()` converts a texture document into the source model
used by `plan_texture_export()`, including texture-set dimensions, occupied
UDIMs, layer structure, and atlas regions. Atlas-scope planning produces one
output for all selected members of each atlas. `PlannedTextureExport` carries
the exact contributing `atlas_regions`, so the pixel provider can place each
source without reconstructing layout from names or ordering.

When the export request overrides output resolution, rectangle edges are scaled
with deterministic integer arithmetic. A request that would collapse any
selected rectangle to zero width or height is refused. Selecting only part of
an atlas retains the atlas dimensions but reports only the selected members and
their rectangles; unselected regions are available for the provider to leave
at its configured default.

The older `ExportAtlasSource::texture_set_identifiers` membership-only form is
still accepted for source compatibility. New integrations should supply
`regions`; if both fields are present, their member order must agree.
