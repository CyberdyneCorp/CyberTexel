# mesh-and-texture-sets — Geometry And UV Input

## ADDED Requirements

### Requirement: Mesh ingest is read-only
The system SHALL accept a triangulated mesh with positions, normals, at least one UV set and optional vertex colours, and SHALL NOT modify it. No capability SHALL write mesh geometry, topology or UVs.

#### Scenario: Geometry is never written
- **WHEN** any operation completes
- **THEN** the supplied mesh buffers SHALL be unchanged

### Requirement: Mesh is supplied, not loaded
The system SHALL accept mesh data through an in-memory interface. File parsing SHALL NOT be a capability of this library; a host or a sibling library supplies the buffers.

#### Scenario: Host supplies buffers
- **WHEN** a host passes vertex and index buffers with an attribute description
- **THEN** the mesh SHALL be accepted without the library opening a file

### Requirement: Multiple UV sets
The system SHALL support at least four UV sets per mesh, named, with one designated default. A texture set SHALL name the UV set it uses.

#### Scenario: Second UV set for a lightmap channel
- **WHEN** a texture set names the second UV set
- **THEN** its layers SHALL rasterize into that set's parameterization

### Requirement: Texture set partitioning
Texture sets SHALL be derivable from material assignment, from object or submesh identity, or from an explicit face partition supplied by the host. Every face SHALL belong to exactly one texture set.

#### Scenario: Partition is total
- **WHEN** texture sets are derived by any method
- **THEN** every face SHALL appear in exactly one set, and a face appearing in none SHALL be reported as an error

### Requirement: Per-set resolution and bit depth
Each texture set SHALL carry its own resolution and default bit depth, selectable independently, with per-channel format overrides as defined in texture-document. Supported resolutions SHALL include 512, 1024, 2048, 4096, 8192 and 16384, and non-square sizes SHALL be supported. Supported bit depths SHALL be 8, 16 and 32 bits per channel.

#### Scenario: Mixed resolutions
- **WHEN** one texture set is 4096 and another 1024 in the same document
- **THEN** both SHALL paint and export at their own resolution

#### Scenario: Resolution change preserves content
- **WHEN** a texture set is resized
- **THEN** existing content SHALL follow the explicit replay or resampling policy in `editable-authoring`, and any resampling filter SHALL be documented

### Requirement: UDIM tiles
A texture set SHALL support UDIM tiling, where a tile is a unit square of the set's UV space addressed by the standard `1001 + u + 10 * v` numbering. Tiles SHALL be allocated on demand.

#### Scenario: Painting across a tile border
- **WHEN** a stroke crosses from one UDIM tile into another
- **THEN** both tiles SHALL be written, allocating the second if it did not exist

#### Scenario: Unused tiles cost nothing
- **WHEN** a mesh occupies three UDIM tiles out of a possible hundred
- **THEN** storage SHALL be allocated for three

### Requirement: Atlases
The system SHALL support grouping several texture sets so that they share one output texture set at export, with each contributing set occupying a declared region.

#### Scenario: Two props share an atlas
- **WHEN** two texture sets are assigned to one atlas
- **THEN** export SHALL produce one texture set covering both

### Requirement: Overlapping UV detection
The system SHALL detect overlapping UV faces within a texture set and SHALL report them, because painting one surface will alter the other.

#### Scenario: Warning on import
- **WHEN** a mesh with overlapping UVs is accepted
- **THEN** the overlap SHALL be reported with the affected face indices, and the mesh SHALL still be usable

### Requirement: Coverage gaps are reported
The system SHALL report UV area that no face covers and faces that fall outside the 0-to-1 range of a non-UDIM texture set.

#### Scenario: Faces outside the UV square
- **WHEN** a non-UDIM texture set has faces outside the unit square
- **THEN** they SHALL be reported by index rather than silently wrapped

### Requirement: Mesh replacement preserves painting
The system SHALL support replacing the mesh under a painted document. Texture sets whose identity and UV layout are unchanged SHALL retain their layers. A set whose UV layout changed SHALL be reported, and the host SHALL choose between keeping the texels as they lie, reprojecting, or clearing.

#### Scenario: Topology changed, UVs preserved
- **WHEN** a mesh is replaced by one with the same UV layout and different topology
- **THEN** existing layers SHALL be retained unchanged

#### Scenario: UVs changed
- **WHEN** a replacement mesh has a different UV layout
- **THEN** the change SHALL be reported per texture set and no content SHALL be discarded until the host chooses

### Requirement: Set identity is stable
A texture set SHALL carry a stable identifier derived from its partition source rather than from an ordinal, so that mesh replacement can match sets across versions.

#### Scenario: Reordered submeshes
- **WHEN** a replacement mesh presents the same partitions in a different order
- **THEN** texture sets SHALL still match by identity

### Requirement: Mesh revision
The mesh SHALL carry a revision that advances whenever geometry, UVs or partitioning change, and every derived structure — the spatial acceleration structure of `picking`, the cached UV-space maps of `paint-engine`, and the bound maps of `mesh-maps` — SHALL be keyed by it.

#### Scenario: Derived state is invalidated once
- **WHEN** the mesh is replaced
- **THEN** the revision SHALL advance and every consumer SHALL detect the change from that revision alone, rather than each maintaining its own notion of freshness

### Requirement: Mesh limits are declared
The specification SHALL state the maximum supported vertex and triangle counts and the behaviour when they are exceeded, which SHALL be a named refusal rather than undefined behaviour.

#### Scenario: Oversized mesh
- **WHEN** a mesh exceeding the declared limit is supplied
- **THEN** it SHALL be refused with a diagnostic naming the limit and the supplied count
