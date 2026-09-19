# Host-registered material nodes

`ctex/graph/host_nodes.hpp` provides an instance-owned registry for extending a
material graph without adding process-global callbacks. A registration has a
stable type identifier and version, an ordered socket/property declaration,
CPU evaluation and shader-emission callbacks, supported emission targets,
determinism and resource-dependency declarations, and parity fixtures.

Host identifiers cannot use the reserved `ctex.` prefix. Multiple versions of
one host identifier may coexist, but an exact `(type identifier, version)` pair
can be registered only once. `NodeCategory::host` distinguishes extensions from
the four built-in catalogue families.

## Registration and invocation

`NodeTypeRegistry::register_type` validates the complete declaration before
publishing it. Registration is refused if either semantic implementation is
missing, if sockets or properties are malformed, if an emission target or
resource dependency is duplicated, or if no parity fixture is supplied. In
particular, an emission-only node is rejected by a diagnostic naming its
missing CPU implementation.

`make_node` creates a graph node from the registered defaults. `evaluate` and
`emit` require that the serialized node still has the registered interface,
validate input and output counts and types, and translate exceptions crossing a
host callback into `NodeInvocationError`. Emission callbacks receive the target
and ordered input expressions and return one expression per declared output
plus the concrete resource identifiers used by the emitted code. The currently
named targets are WGSL, MSL, SPIR-V, and HLSL; a registration may support any
non-empty subset.

The registry owns its callbacks. There is no default or global registry, so
applications can isolate plugin sets and can read a fully constructed registry
concurrently as long as they do not mutate it.

## Validation and unknown nodes

`inspect_graph_semantics` resolves every regular node against the built-in
catalogue, graph/group intrinsic types, or an exact host registration. A missing
type/version, stale interface, or unsupported target makes the report
non-emittable and names the node. The registry-aware `validate_graph` and
`validate_workspace` overloads add those issues to the ordinary required-input,
resource, group, and reachability diagnostics.

Graph serialization is deliberately registry-independent. Loading a graph with
an unavailable host type preserves its identifier, version, display data,
sockets, properties, position, and links as an opaque `GraphNode`. Saving it
again is canonical and lossless. Installing the matching registration later
makes the same node usable; deserialization never invokes a callback or drops
unknown content. The same model works inside reusable group subgraphs.

## Replay eligibility and parity fixtures

A resource dependency names the input or property slot whose resolved resource
must be pinned by an operation record. `replay_eligibility` returns true only
when the registration declares deterministic evaluation and the caller names
every declared dependency as pinned. It reports all unpinned slots rather than
silently degrading to checkpoint-only recovery.

Every registration carries at least one `NodeParityFixture`: ordered inputs,
expected CPU outputs, and an explicit numeric tolerance. The fixture verifier
checks CPU results and invokes emission for every declared target, recording
failures by fixture identifier. These fixtures are executor-independent now and
are intended to be consumed unchanged by the cross-executor parity gate in task
7.6, where emitted shaders can be executed and compared to the same expected
outputs.
