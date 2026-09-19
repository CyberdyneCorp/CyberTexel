# Material graph validation

`ctex/graph/validation.hpp` validates graph documents without invoking or
depending on shader emission. A caller supplies the image-resource and mesh-map
identifiers currently available to the material. Validation returns all known
problems in deterministic node, code, and subject order rather than stopping at
the first failure.

## Diagnostics

| Code | Severity | Meaning |
|---|---|---|
| `unconnected_required_input` | Error | An input has neither an incoming link nor a stored constant. |
| `missing_mesh_map` | Error | A Mesh Map node names a map absent from the validation resources. |
| `missing_image_resource` | Error | An image-valued socket or property names an unavailable resource. |
| `missing_group` | Error | A workspace group instance names no owned group definition. |
| `unreachable_node` | Warning | No directed path carries the node's result to the graph output. |

Each `GraphDiagnostic` includes its severity and code, node ID and type, a
machine-readable subject, and an English message. Resource diagnostics use the
missing resource identifier as the subject and repeat it in the message. An
unset resource is named `<unset>`. Required-input diagnostics use the socket
identifier; unreachable-node diagnostics use the node display name.

Warnings do not make `GraphValidationReport::valid()` false. This lets an editor
retain disconnected work-in-progress nodes while still surfacing them. Every
error blocks a valid report. `error_count()` and `warning_count()` avoid forcing
callers to parse messages.

## Required inputs and resources

`std::monostate` is the graph model's declaration that an input has no stored
constant. Such an input is required and must have an incoming link. Any other
type-correct stored value satisfies the input when it is unconnected.

Every `ImageValue` in node inputs, outputs, or properties is checked against the
available image identifiers. Mesh Map nodes read their stable `map` property and
check it against the available map identifiers. Validation reports unavailable
resources even when their node is also unreachable so a downloaded material can
present the complete repair list in one pass.

## Reachability and workspaces

Reachability is computed from the single graph output through a flat reverse CSR
adjacency layout. Its cost is linear in nodes plus links and does not invoke node
evaluation. A node is reachable when any directed path carries it to the output.

`validate_workspace` validates every owned material and group subgraph, checks
group-instance references, and wraps each graph diagnostic with the owning kind
and stable identifier. This keeps identically numbered nodes in different
materials unambiguous.
