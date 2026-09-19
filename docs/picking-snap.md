# Surface snapping

`ctex::pick::snap_to_surface` projects an arbitrary world-space point onto the
nearest point of the mesh within a caller-supplied maximum distance. The world
BVH prunes nodes by point-to-bounds distance before exact point-to-triangle
tests, so snapping does not scan every face.

Exact projection covers triangle interiors, edges and vertices. Degenerate
triangles fall back to their three line segments. Equal-distance triangles are
resolved by the lowest triangle index. A result uses the same `HitRecord` as ray
picking: position, both normals, named UV and UDIM, texture-set identity,
triangle and barycentrics, material ID, and world-space distance. No surface in
range returns an empty optional.
