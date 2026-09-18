# mesh-maps — Consuming Baked Maps

## ADDED Requirements

### Requirement: The library does not bake
The system SHALL NOT implement a baker. It SHALL define the map set it consumes, the interface a baker implements, and the behaviour when a map is absent.

#### Scenario: No baker is present
- **WHEN** the library is built and linked with no bake provider
- **THEN** it SHALL build and run, and generators requiring a map SHALL report the missing map by name

### Requirement: The mesh map set
The system SHALL define and consume the maps: tangent-space normal, object-space normal, world-space direction, ambient occlusion, curvature, thickness, position, height, bent normal, material ID, object ID, UV density and vertex colour.

#### Scenario: Binding a map
- **WHEN** an ambient occlusion map is bound to a texture set
- **THEN** it SHALL be readable by a Mesh Map node and by every generator that names ambient occlusion

### Requirement: Maps are per texture set
Each mesh map SHALL be bound to a texture set and SHALL match that set's UV layout. A map whose dimensions differ from the set's resolution SHALL be accepted and sampled, and the mismatch SHALL be reported.

#### Scenario: Lower-resolution map
- **WHEN** a 1024 AO map is bound to a 4096 texture set
- **THEN** it SHALL be sampled with filtering and the resolution mismatch SHALL be reported once

### Requirement: Bake provider interface
The system SHALL expose a provider interface of C callbacks by which a host or a sibling library supplies maps: a query for which maps it can produce, a request to produce one for a texture set at a resolution, a progress callback and a cancellation token.

#### Scenario: Requesting a bake through a provider
- **WHEN** a provider is attached and a curvature map is requested
- **THEN** the provider SHALL be invoked, progress SHALL be reported, and the result SHALL be bound to the texture set

#### Scenario: Provider cannot produce a map
- **WHEN** a map is requested that the provider does not advertise
- **THEN** the request SHALL fail naming the map and the provider's advertised set, and no placeholder SHALL be bound

### Requirement: No build dependency on a baker
The seam to a baking library SHALL be a format and an interface. There SHALL be no build or link dependency on CyberRemesherAndUV, ClayCore or any other sibling.

#### Scenario: Dependency audit
- **WHEN** the dependency audit runs
- **THEN** no sibling engine SHALL appear among the library's link inputs

### Requirement: A missing map fails loudly
A generator, smart mask or material that reads a map which is not bound SHALL report the missing map by name and SHALL NOT substitute a neutral value.

#### Scenario: Smart material on an unbaked model
- **WHEN** a smart material reading curvature is applied to a texture set with no curvature map
- **THEN** the application SHALL report the missing map rather than producing a flat result that looks subtly wrong

### Requirement: Map staleness
A bound map SHALL record the mesh revision it was produced from. When the mesh changes, bound maps SHALL be marked stale and every consumer SHALL report the staleness, without the maps being discarded.

#### Scenario: Mesh replaced after baking
- **WHEN** the mesh is replaced
- **THEN** existing maps SHALL be marked stale, remain usable, and their staleness SHALL be reported to the host

### Requirement: Import of externally produced maps
The system SHALL accept maps produced outside any provider, supplied as pixel buffers with a declared channel meaning and colour space.

#### Scenario: Maps baked in another application
- **WHEN** an AO map baked elsewhere is supplied as a buffer with its colour space declared
- **THEN** it SHALL bind and behave identically to a provider-produced map

### Requirement: Normal map conventions
The system SHALL record, per bound normal map, whether its green channel follows the OpenGL or DirectX convention, and SHALL convert on read so downstream code sees one convention.

#### Scenario: DirectX normal map
- **WHEN** a normal map declared as DirectX is bound
- **THEN** its green channel SHALL be inverted on read and downstream nodes SHALL see the library's single convention

### Requirement: Generators
The system SHALL provide generators that derive a mask from mesh maps: ambient occlusion, curvature, thickness, position gradient, world-space direction, dirt, edge wear and scratches. Each SHALL name the maps it requires.

#### Scenario: Edge wear
- **WHEN** an edge wear generator runs with curvature bound
- **THEN** it SHALL produce a mask concentrated on convex edges

#### Scenario: Generator declares its inputs
- **WHEN** a generator is queried
- **THEN** it SHALL report the maps it requires, so a host can request them before applying it

### Requirement: Generator parameters are documented and bounded
Every generator parameter SHALL have a documented default, range and meaning, and SHALL be validated at every entry point.

#### Scenario: Out-of-range parameter
- **WHEN** a generator parameter outside its range is supplied
- **THEN** it SHALL be clamped and the clamp SHALL be reported

### Requirement: Generators are deterministic
A generator SHALL produce the same result for the same maps and parameters on every executor, within the parity tolerance.

#### Scenario: Cross-executor generator parity
- **WHEN** a generator runs on the CPU reference and on a GPU executor
- **THEN** the results SHALL agree within the stated tolerance

### Requirement: Map memory is accounted
Bound maps SHALL be included in the document's memory report and SHALL be releasable by a host without discarding the document.

#### Scenario: Releasing maps under pressure
- **WHEN** a host releases bound maps to reclaim memory
- **THEN** the document SHALL remain usable and consumers SHALL report the maps as absent rather than crash

### Requirement: Tangent frame contract
Mesh and normal-map descriptors SHALL declare the tangent basis algorithm and version, normal orientation, handedness, UV set and coordinate conventions. Supplied per-corner tangents SHALL be accepted; generation when absent SHALL use a documented pinned algorithm. Incompatible normal-map and mesh tangent bases SHALL require explicit conversion or refusal, not merely a green-channel flip. Height derivatives and normal blending SHALL use the same declared frame.

#### Scenario: Mirrored UV handedness
- **WHEN** two mirrored islands use a shared tangent-space normal texture
- **THEN** preview and export validation SHALL account for each island's tangent handedness and agree with the reference surface fixture

### Requirement: Versioned asynchronous bake requests
Each bake request SHALL identify the mesh, UV layout, texture set, map, bake-setting revision and request generation. Completion SHALL bind results only while all identities remain current. Superseded or cancelled results SHALL be discarded. The host SHALL be able to group a bake-setting edit and its accepted map replacements into a document transaction; undo SHALL restore the prior settings and map bindings and invalidate pending requests.

#### Scenario: Old bake completes after settings change
- **WHEN** a previous bake completes after a newer settings revision was requested
- **THEN** it SHALL NOT overwrite the current map and the host SHALL receive a stale-result report

#### Scenario: Undo while rebaking
- **WHEN** a bake-setting transaction is undone before its provider completes
- **THEN** the previous settings and map bindings SHALL be restored and the late result SHALL NOT publish
