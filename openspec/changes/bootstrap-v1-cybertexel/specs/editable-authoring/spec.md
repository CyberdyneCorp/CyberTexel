# editable-authoring — Replayable Painting And Surface Edits

## ADDED Requirements

### Requirement: Versioned operation records
The system SHALL retain replayable operation records alongside raster checkpoints for eligible paint operations. Records SHALL identify algorithm and preset versions, resolved stamps or editable source paths, seeds, channel descriptors, mesh and coordinate frames, pinned resource content identities, and input document revisions. Replay SHALL NOT depend on the current contents of mutable shelf files. Recovery records required by execution-backends SHALL remain available even when user-facing undo steps are evicted.

#### Scenario: Brush asset changes after painting
- **WHEN** an eligible stroke is replayed after the shelf image used by its brush was replaced
- **THEN** replay SHALL use the pinned original resource and reproduce the recorded result within the declared tolerance

### Requirement: Explicit replay eligibility
Every operation SHALL report whether it supports same-resolution recovery replay, resolution-independent replay, or only a raster checkpoint. Clone, blur and smear SHALL pin their source snapshots or be checkpoint-only; a reference to the mutable current layer SHALL NOT suffice. Unknown algorithm versions SHALL disable replay and retain raster content with a diagnostic. Recovery data SHALL be subject to resource-residency admission before commit.

#### Scenario: Snapshot-dependent clone
- **WHEN** a clone source can no longer be reconstructed at a requested higher resolution
- **THEN** resize SHALL report that operation as resample-only and SHALL require the host's explicit resampling policy rather than claiming detailed replay

### Requirement: Resolution change policy
A host SHALL choose replay-eligible, resample-all or cancel when changing document resolution. Replay-eligible SHALL re-evaluate procedural content and eligible operation records at the new resolution, and SHALL require an explicit resampling policy for checkpoint-only segments. The change SHALL be atomic, budgeted and undoable.

#### Scenario: Higher-resolution stroke
- **WHEN** a layer consisting entirely of resolution-independent strokes is resized with replay selected
- **THEN** its tips SHALL be rasterized at the target resolution instead of upscaling its old raster and failure SHALL leave the original layer intact

### Requirement: Persistent editable placement
Decals, text and surface paths SHALL be retainable as editable document entries after gesture commit. Their placement frame, text and font identity, path control points and material parameters SHALL round-trip through project-io. Editing an entry SHALL invalidate its dependent tiles and SHALL create one undo step. Rasterizing an entry SHALL be an explicit host operation.

#### Scenario: Editing saved text
- **WHEN** a saved project is reopened and a text entry is edited
- **THEN** its text and placement SHALL remain editable and undo SHALL restore the preceding text without flattening the layer

### Requirement: Reprojection has an inspectable mapping
Mesh replacement with reprojection SHALL retain the old mesh and source content until completion and SHALL use explicit host-selected distance, normal-angle and visibility limits. The operation SHALL report unmapped and ambiguous regions, texture-set matches and affected entries before commit. The host SHALL choose whether unmapped texels retain existing target values or become channel defaults. Tangent normals SHALL be transformed into the destination basis. Reprojection SHALL NOT modify supplied geometry or UVs.

#### Scenario: Ambiguous thin surfaces
- **WHEN** two candidate source surfaces meet the reprojection limits
- **THEN** the ambiguity SHALL be reported and no result SHALL be committed until the host selects the documented resolution policy

#### Scenario: Cancelled reprojection
- **WHEN** reprojection exceeds its budget or is cancelled
- **THEN** the old mesh binding, painted content and history SHALL remain unchanged
