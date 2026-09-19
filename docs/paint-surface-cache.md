# Paint surface-map cache

`<ctex/paint/surface_cache.hpp>` provides the reusable UV-space maps shared by
paint and fill operations. `SurfaceMapCache::lookup` selects a mesh partition,
named UV set and texture tile, then returns one immutable `CachedSurfaceMaps`
bundle containing:

- byte coverage, with one for geometry-owned texels and zero elsewhere;
- exact source-mesh triangle identities, using `no_surface_triangle` outside
  geometry;
- stable UV-island identities, using `no_uv_island` outside geometry; and
- the corresponding interpolated texture-space surface raster.

The texture-set identity is derived from the partition kind, stable partition
key and UV-set name. Triangle identities remain indices into the original mesh
even though only the selected partition is rasterized. UV islands join faces
only across an edge whose mesh positions and UV coordinates agree. This keeps
hard-normal vertex splits connected while avoiding a false connection between
disconnected geometry that merely overlaps in UV space. Island numbering is
deterministic in source-triangle order.

Cache keys include the texture-set partition, UV set, raster dimensions and
tile origin. Multiple tiles and texture sets can coexist. A repeated lookup
returns the same shared bundle without rebuilding its geometry or maps, so a
face fill followed by an island fill can reuse the first lookup directly.

`MeshBinding` revisions are process-unique. On the first lookup after
`MeshBinding::replace`, the cache removes every entry from the old revision
before it can return a result. Selecting a different UV set removes every tile
for that logical partition before building the new selection. Existing shared
pointers remain safe immutable snapshots, but the cache cannot return them
again. `SurfaceMapStatistics` exposes entries, hits, misses and the number of
invalidated entries for tests and host telemetry.

The mesh descriptor owns non-owning spans, so its source arrays must outlive
both the `MeshBinding` and each lookup. Hosts must call `MeshBinding::replace`
when mesh or UV data changes rather than mutating those arrays in place.
