# Material graph emission

`ctex/emit/graph_emission.hpp` lowers the reachable portion of a material graph
to a deterministic sequence of WGSL statements. The result is an expression
program intended for a generated shader function body. `material_emission.hpp`
wraps that program in complete vertex and fragment stages for WGSL or lowers it
through Kong for MSL, SPIR-V and HLSL. It returns the shader together with a
device-independent [pass plan](pass-plans.md) naming the output, graph resources,
sampler, vertex layout, state and draw command.

Each intermediate variable is derived from the stable node ID and output socket
identifier. A node inside a group is additionally qualified by the stable group
identifier and the instance node ID for every enclosing group. Identifiers are
hex-encoded in generated names, so arbitrary user-facing names cannot collide
or inject shader text. Attribution comments retain the producing node and its
complete group path.

The expression program also carries the same attribution structurally: stable
variable name, qualified node path, type ID, node ID and output socket. Complete
material emission preserves that table for all targets. The public
`ctex_shader_emit_material_inspectable` route publishes it as deterministic JSON
beside textual shaders or as companion metadata beside raw SPIR-V modules.

Emission starts at the material output and memoizes every requested node output.
A node callback therefore runs once even when its result fans out to multiple
consumers. Unreachable nodes emit nothing. Socket coercions are inserted at each
use: scalar-to-vector broadcast, vector-to-scalar Rec. 709 luminance,
colour-to-vector RGB extraction, and colour-to-scalar Rec. 709 luminance.

The current shader lowering covers Constant Value and Constant Colour, which
are sufficient to establish naming, grouping, fan-out, literal formatting, and
coercion behavior. Registered host nodes participate through their checked WGSL
callbacks and report their resource identifiers without duplication. Remaining
built-in semantics arrive with their owning executor work; requesting one now
fails by type instead of emitting placeholder code.

`emit_wgsl_expressions` handles a standalone graph. Group instances require
`emit_material_wgsl_expressions`, which resolves definitions through the owning
`GraphWorkspace`. The shader-emission determinism gate runs a grouped fixture
twice and compares the resulting bytes. The concurrency fixture starts four
different graphs together and requires every result to equal its serial
baseline.

`MaterialShaderEmissionCache` retains the complete shader and pass plan by
canonical graph/workspace content, host-node semantics, target, normalized
features, resource declarations and request settings. A constant-only edit
changes shader bytes while leaving the pass plan and binding layout stable.
Host resource callbacks use `material_resource_name` to obtain an injection-safe
textual identifier matching the declared binding.
