# texture-document — The Layer Stack

## ADDED Requirements

### Requirement: Document and texture sets
A document SHALL own one or more texture sets, where a texture set is the pair of a mesh partition and a UV set, and SHALL carry its own resolution, bit depth, layer stack and mesh map bindings. A layer SHALL belong to exactly one texture set and SHALL require no further mesh binding of its own.

#### Scenario: One material per partition
- **WHEN** a mesh whose faces carry two distinct material assignments is opened
- **THEN** two texture sets SHALL be created, each with an independent layer stack, and painting in one SHALL NOT alter the other

#### Scenario: A layer needs no object mask
- **WHEN** a layer is created inside a texture set
- **THEN** it SHALL apply to every face of that set's partition without the caller supplying an object binding

### Requirement: Channel set
A texture set SHALL carry the channels base colour, opacity, roughness, metallic, normal, height, occlusion, emission and subsurface. A host SHALL be able to enable a subset, and a disabled channel SHALL allocate no storage.

#### Scenario: Non-PBR project
- **WHEN** a host enables only base colour and opacity
- **THEN** no roughness, metallic, normal, height, occlusion, emission or subsurface storage SHALL be allocated, and the memory report SHALL reflect that

#### Scenario: Enabling a channel later
- **WHEN** a channel is enabled on a set that already has painted layers
- **THEN** it SHALL be allocated at the set's resolution and initialised to the channel's documented default without disturbing existing channels

### Requirement: Entry kinds
The stack SHALL support paint layers, fill layers, groups, masks, filters and instances. A kind SHALL be an explicit field on the entry rather than inferred from which storage happens to be allocated.

#### Scenario: Kind is explicit
- **WHEN** an entry is queried
- **THEN** its kind SHALL be reported directly, and a mask with no allocated storage yet SHALL still report as a mask

#### Scenario: Fill layer is procedural
- **WHEN** a fill layer's material graph changes
- **THEN** the layer's content SHALL be re-derived rather than retaining previously rasterized pixels

### Requirement: Instances
An instance SHALL reference another entry's content rather than copying it. Editing the referenced entry SHALL update every instance of it. An instance SHALL carry its own opacity, blend mode, channel enablement and masks, and SHALL NOT be directly paintable. An instance SHALL reference only an entry that precedes it in evaluation order, and a reference that would form a cycle SHALL be refused.

#### Scenario: Editing the source
- **WHEN** a layer referenced by two instances is painted
- **THEN** both instances SHALL reflect the change

#### Scenario: Instances carry their own modulation
- **WHEN** an instance's opacity is changed
- **THEN** only that instance SHALL change, and the referenced entry SHALL be unaffected

#### Scenario: Painting an instance is refused
- **WHEN** a paint operation targets an instance
- **THEN** it SHALL be refused with a diagnostic naming the referenced entry

#### Scenario: Deleting a referenced entry
- **WHEN** an entry with live instances is deleted
- **THEN** the deletion SHALL be refused naming the instances, or SHALL convert them to independent copies, and which of the two SHALL be the caller's choice

### Requirement: Nesting rules
Groups SHALL nest to a documented maximum depth of at least 8. A mask SHALL attach to exactly one layer or group. A filter SHALL attach to exactly one layer or group and SHALL read that target's composited output. An operation that would violate a nesting rule SHALL be rejected with a diagnostic naming the rule, and SHALL leave the stack unchanged.

#### Scenario: Illegal reparent is rejected
- **WHEN** a group is dragged inside itself
- **THEN** the move SHALL be refused, the diagnostic SHALL name the rule, and the stack order SHALL be byte-identical to before

#### Scenario: Group masks apply to contents
- **WHEN** a mask is attached to a group
- **THEN** it SHALL modulate the composited result of every entry inside that group

### Requirement: Blend modes
The system SHALL provide the blend modes Normal, Darken, Multiply, Color Burn, Lighten, Screen, Color Dodge, Add, Overlay, Soft Light, Linear Light, Difference, Exclusion, Subtract, Divide, Hue, Saturation, Color, Value and Pass Through, and SHALL define each by a formula in the specification rather than by reference to another product. Pass Through SHALL be valid only on a group.

#### Scenario: Formula is authoritative
- **WHEN** a blend mode is applied by the CPU reference executor and by a GPU executor
- **THEN** both SHALL implement the specified formula and agree within the parity tolerance

#### Scenario: Pass Through on a layer is rejected
- **WHEN** Pass Through is set on a paint layer
- **THEN** the call SHALL fail with a diagnostic and the layer's blend mode SHALL be unchanged

### Requirement: Per-channel participation
Each layer SHALL carry an independent enable flag and an independent opacity per channel, and SHALL contribute a channel only when that channel is enabled on both the layer and the texture set.

#### Scenario: Roughness-only layer
- **WHEN** a layer enables roughness alone
- **THEN** compositing SHALL alter roughness and leave every other channel of the accumulated result unchanged

### Requirement: Effective opacity
Effective opacity for an entry SHALL be its own opacity multiplied by the opacity of every enclosing group and by every mask that applies to it, evaluated per texel.

#### Scenario: Nested opacity
- **WHEN** a layer at opacity 0.5 sits in a group at opacity 0.5
- **THEN** its contribution SHALL be scaled by 0.25 before blending

### Requirement: Compositing order and determinism
Compositing SHALL proceed from the bottom of the stack upward, and SHALL be deterministic: the same document composited twice on the same executor SHALL produce bit-identical output.

#### Scenario: Repeat composite
- **WHEN** an unchanged document is composited twice
- **THEN** the two results SHALL be bit-identical

### Requirement: Layer operations
The system SHALL provide create, duplicate, delete, reorder, reparent, clear, invert, merge down, merge group, flatten, convert between paint and fill, and apply mask. Every operation SHALL either complete or leave the document unchanged.

#### Scenario: Merge preserves appearance
- **WHEN** two layers are merged down
- **THEN** the composited result of the texture set SHALL be unchanged within the parity tolerance

#### Scenario: Failed operation is atomic
- **WHEN** a merge fails because the target resolution cannot be allocated
- **THEN** both source layers SHALL remain and the document SHALL be unchanged

### Requirement: Tiled storage
Channel storage SHALL be tiled, with a documented tile size, and the system SHALL track which tiles of which channels a given operation dirtied.

#### Scenario: Bounded dirty set
- **WHEN** a stroke covers 2% of a texture set's UV area
- **THEN** the reported dirty tile set SHALL cover that area and SHALL NOT be the whole set

### Requirement: Undo history is tile-scoped
An undoable pixel operation SHALL snapshot only the tiles it dirtied. Restoring SHALL exchange tile ownership rather than copying pixels back, so undo and redo cost the same.

#### Scenario: Undo then redo
- **WHEN** a stroke is undone and then redone
- **THEN** the texture set SHALL return to the post-stroke state and no additional full-channel copy SHALL be performed

#### Scenario: Large canvas keeps its history
- **WHEN** a 16384 by 16384 texture set receives a small stroke
- **THEN** the snapshot SHALL be proportional to the dirtied tiles, not to the canvas

### Requirement: Declared history budget
A host SHALL set a memory ceiling for history. The system SHALL report how many steps are currently retained and SHALL discard the oldest step rather than exceed the ceiling. It SHALL NOT silently reduce a configured step count.

#### Scenario: Ceiling reached
- **WHEN** a new step would exceed the ceiling
- **THEN** the oldest step SHALL be discarded, the retained count SHALL be reported, and the operation SHALL succeed

#### Scenario: A step cannot fit at all
- **WHEN** a single operation's snapshot exceeds the whole ceiling
- **THEN** the operation SHALL be refused with a diagnostic naming the requested size and the ceiling, and the document SHALL be unchanged

### Requirement: Non-pixel edits are cheap
Renaming, reordering, reparenting, opacity, blend mode, channel enablement and graph edits SHALL be recorded as command records, not as pixel snapshots.

#### Scenario: Renaming costs nothing
- **WHEN** a layer is renamed
- **THEN** the history memory report SHALL not increase measurably

### Requirement: Redo invalidation
Recording a new step while redo steps are pending SHALL discard those redo steps and release their storage.

#### Scenario: Edit after undo
- **WHEN** a new stroke is made after two undos
- **THEN** redo SHALL report zero available steps and their tiles SHALL be released

### Requirement: Composite grouping
A host SHALL be able to open a transaction so that many operations collapse into one undo step, and SHALL be able to cancel it, leaving the document byte-identical to its state at open.

#### Scenario: A gesture is one step
- **WHEN** a drag produces forty operations inside one transaction
- **THEN** a single undo SHALL reverse all forty

#### Scenario: Cancel is exact
- **WHEN** a transaction is cancelled
- **THEN** the document SHALL be byte-identical to its state when the transaction opened
