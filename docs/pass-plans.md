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
bindings state sampled or storage access, sampler bindings carry filtering and
addressing, and uniform blocks carry byte-exact field offsets, sizes, and
alignments. Render passes additionally name vertex layouts, color and depth
targets, blending and depth state, shader entry points, and draw dimensions.
Compute passes name their compute entry point, storage bindings, and dispatch
dimensions. Target dimensions are checked against the selected mip or tile
rectangle.

Feature-gated [layer-stack emission](feature-gated-emission.md) now produces
target shaders whose bindings, intermediate resources, dependencies, and entry
points match these plans. The plan deliberately stops at submission
description. Completion records and
the rule that prevents recycling a generation while a host submission still
uses it arrive with the host execution protocol in task 7.3. Target-specific
graph and preview shader modules continue in tasks 6.14–6.16. Complete
layer-stack plans and shaders are retained together by the
[emission cache](emission-cache.md).
