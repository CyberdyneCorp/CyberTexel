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

Consumers that know their complete inputs call `check_required_maps` before
execution. Its structured `MeshMapRequirementReport` carries the consumer and
texture-set identities, a deduplicated stable list of every absent map and an
English diagnostic naming them. `require_maps` turns the same report into a
`MissingMeshMapsError`; direct `map` and `sample` reads use that typed error too.
A satisfied report has no diagnostic. A failed check never binds placeholder
pixels, changes existing bindings or returns a neutral sample, so generators,
smart masks and materials can expose the failure without silently flattening
their result.

## Bake-provider seam

`BakeProvider` is an optional synchronous callback table with an opaque context,
a capability query and a request callback. Requests name the map, texture-set
identity, UV layout and requested resolution. The provider returns a borrowed
strided pixel-buffer view; CyberTexel validates and copies it before the
callback returns, then binds it transactionally. Provider memory never becomes
document-owned memory by accident.

`BakeControl` carries C-style progress and cancellation callbacks. CyberTexel
reports initial zero progress, forwards valid monotonic provider progress below
one, and reports one only after validated output is bound. Existing or observed
cancellation publishes no map. Unsupported requests do not invoke the request
callback and return a diagnostic naming both the requested map and the complete
advertised set. Provider failure, invalid progress and malformed output likewise
leave existing bindings unchanged. The callback arguments and returned image
view are valid only for the synchronous call; revisioned asynchronous requests
are reserved for roadmap task 11.13.
