# Material graph emission

`ctex/emit/graph_emission.hpp` lowers the reachable portion of a material graph
to a deterministic sequence of WGSL statements. The result is an expression
program intended for a generated shader function body. Device-independent
[pass plans](pass-plans.md) now define resource generations, layouts, bindings,
state, and commands; target-specific entry-point integration follows in tasks
6.11–6.16.

Each intermediate variable is derived from the stable node ID and output socket
identifier. A node inside a group is additionally qualified by the stable group
identifier and the instance node ID for every enclosing group. Identifiers are
hex-encoded in generated names, so arbitrary user-facing names cannot collide
or inject shader text. Attribution comments retain the producing node and its
complete group path.

Emission starts at the material output and memoizes every requested node output.
A node callback therefore runs once even when its result fans out to multiple
consumers. Unreachable nodes emit nothing. Socket coercions are inserted at each
use: scalar-to-vector broadcast, vector-to-scalar Rec. 709 luminance,
colour-to-vector RGB extraction, and colour-to-scalar Rec. 709 luminance.

The current built-in lowering covers Constant Value and Constant Colour, which
are sufficient to establish naming, grouping, fan-out, literal formatting, and
coercion behavior. Registered host nodes participate through their checked WGSL
callbacks and report their resource identifiers without duplication. Remaining
built-in semantics are added with the complete material/shader scenario suite in
task 6.16; requesting one now fails by type instead of emitting placeholder code.

`emit_wgsl_expressions` handles a standalone graph. Group instances require
`emit_material_wgsl_expressions`, which resolves definitions through the owning
`GraphWorkspace`. The shader-emission determinism gate runs a grouped fixture
twice and compares the resulting bytes.
