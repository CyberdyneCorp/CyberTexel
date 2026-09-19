# Mesh maps

`ctex::maps::MeshMapSet` owns the map bindings for one texture set and its
named UV layout. The stable inventory is tangent-space normal, object-space
normal, world-space direction, ambient occlusion, curvature, thickness,
position, height, bent normal, material ID, object ID, UV density and vertex
colour.

Every binding names its texture-set identity and UV set. A mismatch is refused
before the set changes. Normal, direction and position maps require three
channels; scalar and identifier maps require one; vertex colour accepts three
or four. Storage can use 8-bit normalized, 16-bit normalized or 32-bit float
channels through `ctex::image::TiledImage`.

Map resolution is independent of the texture-set resolution. `bind` accepts a
different resolution and returns one `MapResolutionMismatch` in that bind's
result; later sampling does not repeat the report. Rebinding a kind replaces its
previous pixels and reports that disposition.

`sample` accepts finite normalized UV coordinates. Continuous maps use bilinear
filtering, with UV `(0, 0)` at the bottom-left and edge clamping implicit at the
inclusive normalized boundary. Material-ID and object-ID maps use nearest
sampling so filtering cannot create identities that were never stored. Missing
maps and malformed bindings are refused by name instead of returning neutral
values.
