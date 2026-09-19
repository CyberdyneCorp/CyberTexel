# Picking spatial index

Picking uses a CPU-resident bounding-volume hierarchy built from a validated
`ctex::mesh::MeshBinding`. The hierarchy stores nodes and triangle order in flat
contiguous arrays. Leaves contain at most four triangles; internal nodes split
the centroid range at its median along the longest axis. A query traverses the
hierarchy iteratively and returns only the leaf candidates whose bounds meet the
ray, rather than scanning the mesh's triangle array.

`MeshBinding` wraps an immutable `MeshView` with an opaque, process-unique
revision. The host replaces the binding after changing geometry, UVs, or face
partitions. Replacement first validates the new view, then publishes a new
revision. `SpatialIndex::synchronize` reuses its arrays when that revision is
unchanged and commits replacement arrays only after a complete rebuild, so a
failed rebuild does not leave a partially updated index.

Every candidate query receives the current binding and synchronizes before
traversal. A host therefore cannot accidentally query stale geometry after a
successful replacement; `synchronize()` is also public for optional eager
rebuilds.

The current candidate query is the acceleration boundary for subsequent picking
tasks. Exact ray/triangle intersection, hit records, occlusion and backface
policies remain separate roadmap work.
