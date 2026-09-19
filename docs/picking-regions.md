# Picking region queries

`ctex/pick/region.hpp` provides four CPU region queries over the revision-keyed
world-space picking BVH:

- `query_screen_rectangle` selects projected triangles intersecting a closed
  screen-space rectangle.
- `query_screen_lasso` selects projected triangles intersecting a closed lasso.
- `query_world_sphere` selects triangles whose exact closest point lies within
  the sphere.
- `query_world_box` selects triangles that exactly intersect the axis-aligned
  box.

Every result contains ascending triangle indices and traversal counters. The
ordering is independent of BVH layout. The counters expose how many nodes and
leaf triangles the broad phase visited without conflating candidates with exact
matches.

## Screen-space rule

Screen coordinates use the same top-left origin and column-major view and
projection matrices as ray construction. Rectangle and lasso points must lie
inside the non-empty viewport. A lasso has at least three points, closes from
its last point to its first, and uses the even-odd interior rule.

Partial coverage is inclusive: a triangle is selected when any visible portion
of its projection overlaps the region, including contact at an edge or vertex.
Triangles are clipped to the canonical OpenGL clip volume before the exact 2D
test, so geometry beyond the near or far plane does not select. The lasso's
screen bounding rectangle supplies a conservative six-plane BVH frustum; the
lasso polygon itself is applied only during the exact test.

## World-space rule

Sphere and box boundaries are closed, so boundary contact selects a triangle.
The sphere query uses point-to-AABB pruning followed by exact
point-to-triangle distance. The box query uses AABB pruning followed by clipping
the triangle against all six box planes. Degenerate triangles are handled as
their remaining points and segments.

All four entry points synchronize the shared `SpatialIndex` with the mesh
revision before traversal and require no GPU.
