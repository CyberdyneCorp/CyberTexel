# Atomic layer operations

`TextureSet::apply_layer_operation()` applies one structural or destructive edit
to the layer stack and its complete CPU-resolved raster snapshot. The operation
builds a candidate stack and snapshot, validates both, and publishes the stack
only after every allocation, invariant, and required appearance comparison has
succeeded. A thrown `LayerOperationError` therefore leaves the texture set
unchanged; the request is passed by value, so the caller's original snapshot is
unchanged as well.

The snapshot uses `LayerCompositeRequest`: dense normalized content, independent
coverage, and identity-addressed mask rasters. This is the interchange boundary
for CPU and host-resolved content. Pixel authors use the separate
[tile-history contract](tile-history.md) around channel writes; task 3.10 will
compose structural and pixel changes into larger transactions.

## Structural operations

- Create appends one validated entry and its supplied resolved content.
- Duplicate clones the selected entry, every nested child, and every mask or
  filter attached to that ownership subtree. The selected entry receives the
  requested new identity; owned identities become `<new-root>/<old-identity>`.
  Parent, target, and internal instance references are retargeted.
- Delete removes the same ownership subtree. Live external instances follow the
  explicit refusal or make-independent policy; making one independent requires
  the resolved source rasters. A group instance without a baked group raster is
  refused rather than being converted into an empty independent group.
- Reorder moves the complete ownership subtree before a sibling, or to the end
  of its current scope. Reparent does the same in another group or at root.
  Invalid scope, dependency, cycle, or evaluation order is refused atomically.

## Pixel operations

Clear makes selected resolved channels transparent. Invert changes each active
data component to `1-value` and preserves coverage. Both advance the content
revision and turn procedural resolved content into authored paint content so a
later graph evaluation cannot silently discard the destructive edit.

Merge Down replaces a layer and its lower sibling with one paint layer using
the lower sibling's stable identity. Merge Group replaces a group ownership
subtree with one paint layer using the group's identity. Flatten replaces the
entire stack with the supplied root paint identity. Convert supports paint to
fill when a graph is supplied, and fill to paint when it is absent. Apply Mask
bakes one mask attachment into its target; a group target is rasterized with its
owned subtree.

Merge, merge-group, flatten, convert, and apply-mask are
appearance-preserving operations. Their resolved candidate content is
recomposited before publication and compared per component with the request's
finite, non-negative tolerance. A mismatch reports
`LayerOperationErrorCode::appearance_mismatch` and publishes nothing. This lets
a CPU evaluator or host GPU produce bake rasters while the document remains the
authority that verifies and commits the result.

`maximum_output_bytes` bounds pixels, independent coverage, and mask samples in
the candidate snapshot. A limit violation reports `allocation_limit` before the
stack changes. Invalid structure and invalid raster snapshots are distinguished
as `invalid_operation` and `invalid_content`.
