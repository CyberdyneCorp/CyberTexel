# Material graph document

`ctex/graph/document.hpp` defines the device-independent material graph model.
Every document contains exactly one output-role node. Regular nodes carry stable
numeric identities, a versioned type identifier, editor position, ordered input
and output socket declarations, and ordered serializable properties. Links name
their source output and target input by stable socket identifier.

`ctex::doc::make_material_graph` constructs that output from the registered
channel descriptors without reversing the dependency: `doc` adapts channels to
`graph`, while `graph` remains unaware of document types. The metallic/roughness
preset produces nine ordered inputs with scalar, vector, or colour constants
taken directly from each channel descriptor.

The document assigns monotonically increasing node identities and serializes the
next identity, so deleting a node and reopening the graph never reuses its ID.
Nodes are stored in identity order and links in endpoint order. Socket and
property declaration order remains significant because it is part of a node
type's user-facing interface.

Socket storage represents scalar, vector, colour, string, image reference, and
boolean values. Link creation uses this exact coercion table:

| Output | Input | Link coercion |
|---|---|---|
| Any type | Same type | Identity |
| Scalar | Vector | Broadcast to XYZ |
| Vector | Scalar | Linear Rec. 709 luminance |
| Colour | Vector | Pass RGB; ignore alpha |
| Colour | Scalar | Linear Rec. 709 luminance over RGB |

The luminance weights are `(0.2126, 0.7152, 0.0722)`. Every other cross-type
connection is refused with a typed diagnostic naming the output and input types.
The document records the required coercion but does not alter either socket's
stored value; emission applies it later.

Each input holds at most one link. Connecting a different output to an occupied
input atomically replaces the old link, and `add_link` returns both the old link
and the new link's coercion. Type and cycle refusals occur before replacement,
so the existing link remains unchanged. Deserialization enforces the same type
and single-link invariants.

## Cycle refusal

Before adding a link from source to target, the document searches the existing
graph for a deterministic target-to-source path. If found, `GraphCycleError`
reports the closed node-ID path and the graph remains byte-identical. Self-links
are the two-entry path `node -> node`. Deserialization also performs a linear
topological pass over a flat CSR adjacency layout and rejects cyclic input.

## Serialization

`serialize_graph` emits canonical `CTEX_GRAPH` version 1 text. Record and field
names make the schema inspectable; arbitrary strings are hexadecimal so names,
UTF-8, tabs, and newlines round-trip without escaping ambiguity. Floating-point
values use their exact IEEE bit patterns. `deserialize_graph` rejects unknown
versions, malformed records, duplicate identities, missing sockets, non-finite
values, and documents that do not have exactly one output.

Serialization, cloning, and equality include node positions, unconnected socket
values, properties, links, and the next stable identity. The graph module has no
device, host, or shading-language type; emission remains a separate module.

The versioned built-in declarations and their math/normal formulas are listed in
the [material node catalogue](material-node-catalogue.md).
Reusable subgraphs, transactional interface propagation, and recursion refusal
are documented in [material node groups](material-node-groups.md).
Structured resource, required-input, and reachability checks are documented in
[material graph validation](material-graph-validation.md).
Versioned extension callbacks, opaque unknown-node preservation, replay rules,
and parity fixtures are documented in [host-registered nodes](host-node-types.md).
