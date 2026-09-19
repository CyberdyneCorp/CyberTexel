# Device-independent pass plans

`ctex/emit/pass_plan.hpp` defines the complete host-facing description of an
ordered render or compute submission. It contains no graphics API objects or
device handles. A host translates the declared entry points, resources,
bindings, layouts, targets, state, and draw or dispatch command into its own GPU
API.

Every texture is identified by a logical ID and generation. Generations make a
committed input and its replacement distinct even when they share a logical ID.
Texture declarations include their format, extent, mip count, tile shape, role,
and whether their initial contents come from the host. Accesses select mip and
layer ranges and may narrow a single mip to a tile rectangle.

Each pass declares read, write, or read-write access with explicit load and
store actions. Construction rejects undeclared or uninitialized resources,
out-of-range subresources, conflicting access declarations, and overlapping
cross-pass hazards without an ordered dependency. A discarded write
invalidates later reads. Disjoint tile ranges do not create a false dependency.
The plan derives the first and last pass that uses each resource generation;
these lifetimes apply to this submission only.

Bindings are ordered values with explicit group and binding indices. Texture
bindings state sampled or storage access plus 2D/cube view, encoding, and mip
convention metadata when the host needs them. Sampler bindings carry filtering
and addressing, and uniform blocks carry byte-exact field offsets, sizes, and
alignments. Cube views are checked for square six-layer faces. Render passes
additionally name vertex layouts, color and depth
targets, blending and depth state, shader entry points, and draw dimensions.
Compute passes name their compute entry point, storage bindings, and dispatch
dimensions. Target dimensions are checked against the selected mip or tile
rectangle.

Feature-gated [layer-stack emission](feature-gated-emission.md) now produces
target shaders whose bindings, intermediate resources, dependencies, and entry
points match these plans. [Preview emission](preview-shading.md) uses the same
contract for material channels, environment resources, analytic lights, and
unlit channel inspection. The plan deliberately stops at submission
description. The [host execution protocol](host-execution.md) supplies
completion records and prevents recycling a generation while a host submission
still uses it. Complete material
graph, layer-stack, preview and inspection entry points all return their plan
beside the matching shader. Complete layer-stack plans and shaders are retained
together by the [emission cache](emission-cache.md).
