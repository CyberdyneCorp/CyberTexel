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

## Instances

An instance stores only the stable identity of a preceding source entry. Calling
`resolved_content()` follows instance chains at query time, so painting or
revising the source is immediately visible through every instance without a
content copy. Missing, forward, self-cyclic and multi-entry cyclic references
are refused before the stack changes.

Opacity, blend-mode identity, per-channel enablement/opacity, group parent and
attached masks remain fields of the instance itself. Their setters replace a
validated candidate entry and never modify the source. `paint_target()` and
`record_paint()` refuse a direct instance target with a diagnostic and related-
identity record naming its source.

Removing a referenced source requires an explicit
`ReferencedSourceDeletionPolicy`. `refuse` preserves the complete stack and
reports all direct and transitive live instances. `make_instances_independent`
copies the resolved source definition and revision into each dependent entry
while preserving its own modulation and attached masks, then removes the
source atomically.

## Blend modes

`graph::blend_mode_definitions()` is the single ordered catalogue of the twenty
document and material-graph modes. Each definition carries its stable identity,
display name and formula; the authoritative component and HSV equations are in
the texture-document specification. `LayerStack::evaluate_blend()` dispatches
to the shared `graph::blend_colour()` CPU reference rather than maintaining a
second formula table.

Every entry validates its blend identity transactionally. Pass Through is
accepted only on a group and means that its children composite directly into
the enclosing accumulator. Setting it on a paint layer—or setting any unknown
mode—reports `LayerStackRule::blend_mode` and preserves the previous mode.

## Per-channel participation

A content layer participates only when its texture-set channel has storage, the
layer is enabled, and the layer has an enabled `ChannelModulation` record for
that semantic identity. A missing record is disabled, so a roughness-only layer
cannot accidentally alter base colour or any other accumulated channel. The
texture-set query validates that the semantic identity is registered before it
resolves participation.

For a participating channel, effective opacity is the product of the layer's
opacity, its independent channel opacity, and every enclosing group's opacity.
A disabled group gates the complete descendant chain. `applicable_masks()`
adds enabled masks attached directly to the layer and enabled masks attached to
any enclosing group, preserving canonical stack order. The caller supplies the
fully evaluated value of each such mask for the texel; all values are multiplied
into effective opacity.

Mask samples are identity-addressed and must cover the active chain exactly
once. Missing, duplicate, unknown, non-finite, or out-of-range values report
`LayerStackRule::mask_sample`; disabled masks are absent from the chain.
`LayerChannelParticipation` reports both the final factor and the ordered masks
that contributed to it. This is a flattened participation report; the CPU
compositor preserves scope boundaries by applying an ordinary group's factor to
its isolated result and propagating the same factor through children only for a
Pass Through group.

The [CPU layer compositor](layer-compositing.md) consumes these semantics for
bottom-to-top channel evaluation, isolated and Pass Through groups, filters,
instances, coverage and deterministic results.

The [atomic layer-operation API](layer-operations.md) transforms the stack and a
complete resolved-content snapshot together, including subtree ownership and
appearance-checked destructive bakes. Tile-scoped history remains a subsequent
roadmap task.
