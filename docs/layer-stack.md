# Document layer stack

Every `TextureSet` owns one independent `LayerStack`. Its vector order is the
bottom-to-top evaluation order, and every `LayerEntry` reports an explicit kind:
paint layer, fill layer, group, mask, filter, instance, editable decal, editable
text, or surface path. Empty storage therefore never changes an entry's kind.

Stable entry identities and non-empty display names are required. Parents and
attachment targets must precede their consumers in evaluation order. Only a
group may parent another entry, groups can nest to the documented maximum depth
of 32, and masks and filters attach to exactly one preceding layer or group.
The target identity is the filter's explicit composited-input dependency and a
group-targeted mask applies to the group's composited contents.

Mutations validate a complete candidate stack before publication. A refusal
reports a typed `LayerStackRule`—including identity, ordering, group-parent,
depth, single-attachment and attachment-target rules—and leaves the stack
unchanged. Updating a fill layer's graph advances its content revision so hosts
discard derived output instead of treating it as retained raster content.

Applying a smart material or smart mask imports its entries into the same
canonical texture-set stack. Parameter and ordinary entry edits synchronize
that stack, and undo removes the complete imported fragment as one operation.
The preset fragment remains separately available for origin and exposed-
parameter metadata; it is not a second evaluation stack.

Instance reference semantics, blend/channel modulation, general layer
operations, compositing and history are subsequent roadmap tasks.
