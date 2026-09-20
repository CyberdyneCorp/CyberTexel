# Fill tool

Fill resolves one of six scopes into a normalized texture-space mask, then
intersects the ordinary paint masks and explicit rejection acceptance before
shading every enabled channel with the active material.

| Scope | Resolution rule |
|---|---|
| Whole set | Every covered texel in the texture set |
| Triangle | Exact cached source-triangle identity at the picked texel |
| Connected by angle | Breadth-first mesh adjacency while each crossed face pair stays within the requested angle |
| UV island | Exact cached island identity at the picked texel |
| UV tile | Integer `(floor(u), floor(v))` tile containing the picked texel |
| Selection | Caller-supplied normalized selection values, clipped to surface coverage |

Connected fill consumes explicit triangle adjacency and unit geometric normals;
it never infers connectivity from neighboring texture pixels, so a UV seam does
not interrupt a surface-continuous flood. The default maximum adjacent-face
angle is 45 degrees.

The maximum adjacent-face angle is clamped to `[0, 180]` degrees. The resolved
angle drives traversal and any clamp is returned in
`FillScopeResult::parameter_report`; non-finite values are refused.

`resolve_fill_scope` returns the scope mask for inspection or reuse.
`apply_fill` additionally applies masks and rejection, then uses the common
stroke-start shading path shared with Brush. Inputs and source rasters are never
mutated.

The public C boundary exposes the same operation as `ctex_paint_apply_fill`.
Callers pass the arrays returned by the surface-map cache, explicit topology for
connected fill, and channel snapshots. A sizing call reports scope, selected
triangle and channel requirements; a filling call publishes all three atomically.
