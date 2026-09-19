# Picking hit records

`ctex::pick::pick_nearest` traverses the mesh BVH, performs exact
ray/triangle tests on candidate leaves and returns `std::optional<HitRecord>`.
An empty optional is a normal miss, distinct from invalid input.

The input ray direction is normalized before traversal, so hit distance is in
world-space units even for a caller-supplied non-unit direction. Mesh positions
are interpreted as world-space positions. `pick_nearest` is the default
single-hit query. `pick_ray` additionally accepts nearest or all-hits occlusion;
all hits are ordered by increasing distance and then triangle index.

When a nearest ray lands on a shared edge or vertex, intersections at equivalent
distance (relative tolerance `1e-10`) are owned by the lowest triangle index.
This makes the result independent of BVH traversal order. All-hits mode remains
literal: it reports every intersected adjacent triangle, ordered by exact
distance and then triangle index.

A successful record contains world position, normalized interpolated and
geometric normals, the UV from the texture set's named UV buffer, stable texture
set identity, standard UDIM coordinates and number, triangle index, barycentric
weights, per-face material identifier and distance. The caller supplies a
partition-to-UV binding view; missing or duplicate bindings for the hit
partition are refused rather than returning an incomplete record.

The geometric normal follows counter-clockwise vertex winding through
`normalize(cross(vertex1 - vertex0, vertex2 - vertex0))`. Smooth vertex normals
are barycentrically interpolated and normalized independently, so tools can
distinguish shading orientation from the actual face plane.

Backfaces are accepted by default and may be rejected per call. With the stated
counter-clockwise winding, a face is front-facing when the dot product of its
geometric normal and the ray direction is negative. Backface rejection retains
only those front-facing intersections; it does not alter the mesh or index.
