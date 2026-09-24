# Texture document scenario coverage

`just test-texture-document-scenarios` runs every CTest carrying the
`texture-document-scenario` label. The `texture-document-scenario-matrix` test
compares this table with the OpenSpec capability, so scenario names and their
executable evidence cannot drift silently.

| OpenSpec scenario | Executable evidence | Covered behavior |
| --- | --- | --- |
| One material per partition | `mesh-ingest`, `texture-document` | Material partitions produce independently stored texture sets with distinct stacks and channel pixels |
| A layer needs no object mask | `texture-document-layer-stack` | A root layer belongs to its texture set and needs no second mesh or object binding |
| Non-PBR project | `document-channels` | Enabling only base colour and opacity leaves all other channels storage-free and reported resident bytes at zero |
| Enabling a channel later | `document-channels` | A 16-bit height channel starts at its declared default without changing painted 8-bit base colour |
| Kind is explicit | `texture-document-layer-stack` | Every entry kind, including an unallocated mask, is retained as an explicit enum |
| Fill layer is procedural | `texture-document-layer-stack` | Replacing a fill graph advances its derived-content revision instead of retaining stale raster content |
| Editing the source | `texture-document-instances` | Two instances resolve the same source revision without copying source content |
| Instances carry their own modulation | `texture-document-instances` | Instance opacity, blend mode, channel modulation and masks change without modifying the source |
| Painting an instance is refused | `texture-document-instances` | Direct paint reports the referenced source and leaves the stack unchanged |
| Deleting a referenced entry | `texture-document-instances` | Refuse and make-independent policies name dependents and publish atomically |
| Illegal reparent is rejected | `texture-document-layer-stack` | Self-parenting names the evaluation-order rule and preserves the complete stack |
| Group masks apply to contents | `texture-document-channel-participation`, `texture-document-cpu-compositor` | Group masks enter each child's effective chain and modulate the isolated group result once |
| Formula is authoritative | `texture-document-blend-modes`, `executor-parity-gate` | All twenty fixed CPU formulas share the parity corpus used for available executors |
| Pass Through on a layer is rejected | `texture-document-blend-modes` | Invalid Pass Through assignment preserves the paint layer's previous mode |
| Roughness-only layer | `texture-document-channel-participation`, `texture-document-cpu-compositor` | Sparse channel participation changes roughness without changing other enabled channels |
| Nested opacity | `texture-document-channel-participation` | Layer and enclosing-group opacity multiply to the expected effective value |
| Repeat composite | `texture-document-cpu-compositor`, `texture-document-composite-determinism` | Repeated nested composition is bit-identical in-process and across independent runs |
| Merge preserves appearance | `texture-document-layer-operations` | Merge-down recomposites the candidate and retains the pre-operation result within tolerance |
| Failed operation is atomic | `texture-document-layer-operations` | Allocation and layout failures publish neither a stack nor resolved-content snapshot |
| Duplicate group ownership | `texture-document-layer-operations` | Nested children and attachments receive deterministic identities and retargeted internal links |
| Invalid baked result | `texture-document-layer-operations` | Appearance mismatch rejects merge, flatten, conversion and mask candidates before publication |
| Destructive procedural edit | `texture-document-layer-operations` | Clear and invert rasterize procedural entries and advance content revision |
| Bounded operation output | `texture-document-layer-operations` | Candidate bytes are checked before allocation and an exceeded ceiling leaves state unchanged |
| Bounded dirty set | `tiled-image` | A sparse write dirties only intersected 64-square tiles rather than the full image |
| Undo then redo | `texture-document-tile-history` | Undo and redo exchange the exact before/after tile owners without pixel copies |
| Large canvas keeps its history | `texture-document-tile-history` | One changed tile on a 16384-square channel retains one physical tile |
| Declared tile remains unchanged | `texture-document-tile-history` | An unchanged declared target is omitted from the step and byte charge |
| Stale tile is not overwritten | `texture-document-tile-history` | All targets are validated before a stale multi-tile undo exchanges any owner |
| Ceiling reached | `texture-document-tile-history` | The oldest undo step is evicted and the new step succeeds within the byte ceiling |
| A step cannot fit at all | `texture-document-tile-history` | Oversized capture is refused before pixels or history change |
| Empty history operation | `texture-document-tile-history` | Empty undo and redo return distinct typed errors without mutation |
| Renaming costs nothing | `texture-document-transactions` | A staged rename commits, undoes and redoes as command history with zero retained pixel bytes |
| Edit after undo | `texture-document-tile-history` | A replacement edit clears pending redo steps and releases their retained owners |
| A gesture is one step | `texture-document-transactions` | Forty staged writes to one tile commit and undo as one history step |
| Cancel is exact | `texture-document-transactions` | Explicit cancellation and destruction preserve live pixels, revisions, layers and history |
| Mixed transaction is one step | `texture-document-transactions` | Staged pixels and layer operations remain isolated until one commit and share one undo/redo step |
| Transaction becomes stale | `texture-document-transactions` | Live tile, layer or history changes reject commit without partial publication |
| Mixed precision and an added channel | `document-channels`, `project-container`, `texture-export-execution` | Custom coat weight, 8-bit base colour and 16-bit height remain independently described through document, storage and export paths |
| Full-channel write | `texture-document-transactions`, `c-abi-transaction` | Bulk row bytes round-trip and undo restores prior tile owners |
| Partial-tile region | `texture-document-transactions` | Edge tile writes preserve pixels outside the region |
| Write outside declared targets | `texture-document-transactions`, `c-abi-transaction` | Missing tile declaration is refused before staged mutation |
| Size or format mismatch | `texture-document-transactions`, `c-abi-transaction` | Invalid row pitch and byte count are refused before staged mutation |
