# UV-space picking

`ctex::pick::UvSpatialIndex` builds a flat two-dimensional BVH for one named UV
set. Nodes and triangle order are contiguous, leaves contain at most four
triangles, and point queries traverse only bounds containing the requested UV.
The index synchronizes against `MeshBinding::revision()` before every query and
commits replacement storage only after a complete rebuild.

`pick_uv` is the inverse of surface ray picking. It limits candidate triangles
to the requested texture-set partition, performs an exact 2D barycentric test,
and returns the owning surface position, interpolated and geometric normals,
UV, texture-set and UDIM identities, triangle, barycentrics, and material ID.
An unowned coordinate returns an empty optional rather than a fabricated record.

If UVs overlap within one texture set, the lowest triangle index is selected
deterministically. Overlap reporting remains task 4.5; callers should use that
diagnostic to explain ambiguous painted regions to users.
