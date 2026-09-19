# Material node groups

`ctex/graph/groups.hpp` provides `GraphWorkspace`, the ownership boundary for
material graphs and reusable node-group definitions. Keeping the related graphs
in one workspace makes interface propagation complete and transactional rather
than relying on unsafe observers into independently owned documents.

A group has a caller-supplied stable identifier, display name, monotonically
increasing interface version, declared input and output sockets, and an editable
`GraphDocument` subgraph. Its subgraph always starts with two boundary nodes:

- `ctex.group-input` exposes the declared group inputs as internal outputs.
- `ctex.group-output` is the subgraph's single output-role node and accepts the
  declared group outputs as internal inputs.

Instances use the serializable `ctex.group-instance` node type and store the
stable group identifier as a string property. They may appear in any material
owned by the workspace or in another group's subgraph.

## Interface propagation

`update_group_interface` builds updated copies of every material and group graph
before committing. It updates both boundary nodes and every matching instance,
then swaps the complete result into the workspace only after validation. An
allocation error, invalid socket declaration, or graph-invariant failure leaves
the workspace unchanged.

Inputs are rebuilt in declaration order. A stored input value survives only when
both its socket identifier and type are unchanged. A new or type-changed input
takes the new declaration's default. Links survive only across sockets whose
identifier and type are unchanged; removed links are returned with the owning
material or group identifier. This makes propagation visible to editors instead
of silently discarding connections.

## Recursion refusal

When group `B` is placed inside group `A`, the workspace first searches the
existing dependency graph from `B` back to `A`. A discovered path would close a
cycle, so placement throws `GroupRecursionError` before creating the instance.
The exception owns the deterministic closed path, for example
`A -> B -> C -> A`. Placing a group inside itself reports `A -> A` and leaves
the subgraph byte-identical.

The dependency search follows only explicit group-instance nodes and orders
siblings by stable group identifier. Material graphs cannot participate in a
group recursion cycle because groups do not reference their containing material.
