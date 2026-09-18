# picking — Turning A Pointer Into A Place On The Model

## ADDED Requirements

### Requirement: Ray construction
The system SHALL construct a world-space ray from a screen position together with a caller-supplied view and projection matrix and viewport size, and SHALL support both perspective and orthographic projections.

#### Scenario: Orthographic pick
- **WHEN** a pick is requested with an orthographic projection
- **THEN** the constructed ray SHALL be parallel to the view direction with its origin on the near plane at the screen position

### Requirement: Hit record
A successful pick SHALL report the world position, the interpolated surface normal, the geometric face normal, the UV coordinate in the texture set's UV set, the texture set identifier, the UDIM tile, the triangle index, the barycentric coordinates, the material identifier and the distance along the ray.

#### Scenario: Everything a tool needs from one call
- **WHEN** a pick succeeds
- **THEN** all of those fields SHALL be populated, so that a tool need not perform a second query to learn which texture set it hit

#### Scenario: Interpolated and geometric normals differ
- **WHEN** a pick lands on a smooth-shaded face
- **THEN** the interpolated and geometric normals SHALL both be reported and SHALL be distinguishable

### Requirement: Miss is a distinct outcome
A ray that intersects no geometry SHALL report a miss as a distinct, non-error result, and no hit field SHALL be read as meaningful.

#### Scenario: Clicking the background
- **WHEN** a pick ray intersects nothing
- **THEN** the result SHALL be a miss, not an error and not a zeroed hit

### Requirement: Occlusion policy
A pick SHALL report the nearest hit by default, and SHALL optionally report all hits along the ray in order, so that a tool may paint or select through geometry.

#### Scenario: Picking through the mesh
- **WHEN** all-hits mode is requested on a ray passing through both walls of a hollow model
- **THEN** both intersections SHALL be reported in increasing distance order

### Requirement: Backface policy
A pick SHALL be able to ignore or accept back-facing triangles, selectable per call, with the winding convention documented.

#### Scenario: Ignoring back faces
- **WHEN** backface rejection is enabled
- **THEN** a ray entering a closed mesh SHALL report only the front-facing intersection

### Requirement: UV-space picking
The system SHALL resolve a UV coordinate within a texture set to the surface position, normal and triangle that own it, as the inverse of surface picking, so that tools driven from a 2D view behave identically to tools driven from the 3D view.

#### Scenario: Painting in a 2D view
- **WHEN** a UV coordinate is picked
- **THEN** the owning triangle, surface position and normal SHALL be reported, and an unowned coordinate SHALL report a miss

### Requirement: Surface snapping
The system SHALL project an arbitrary world point onto the nearest surface point, reporting the same hit record as a ray pick, within a caller-supplied maximum distance.

#### Scenario: Snapping a placed decal
- **WHEN** a point near the surface is snapped
- **THEN** the nearest surface position, normal and UV SHALL be reported, or a miss if nothing lies within the maximum distance

### Requirement: Region queries
The system SHALL report the triangles intersecting a screen-space rectangle or lasso, and the triangles within a world-space sphere or box, for selection and geometry masking.

#### Scenario: Lasso selection
- **WHEN** a lasso region is queried
- **THEN** the triangles whose projection intersects it SHALL be reported, with a documented rule for partial coverage

### Requirement: Acceleration structure
Picking SHALL be backed by a spatial acceleration structure over the mesh, built once, reused across queries, and invalidated when the mesh changes. Query cost SHALL NOT be linear in triangle count.

#### Scenario: Large mesh
- **WHEN** picks are performed on a mesh of several million triangles
- **THEN** each SHALL complete without scanning every triangle

#### Scenario: Structure is rebuilt on mesh change
- **WHEN** the mesh is replaced
- **THEN** the structure SHALL be rebuilt before the next query rather than returning stale hits

### Requirement: Determinism at shared boundaries
A ray striking exactly an edge or a vertex shared by several triangles SHALL resolve to one of them by a documented, deterministic rule, and repeating the query SHALL return the same triangle.

#### Scenario: Picking exactly on an edge
- **WHEN** the same edge-coincident ray is cast twice
- **THEN** the same triangle SHALL be reported both times

### Requirement: Picking runs without a GPU
Every pick, snap and region query SHALL be available through the CPU reference executor and SHALL NOT require a device.

#### Scenario: Headless picking
- **WHEN** picking is exercised in CI on a machine with no GPU
- **THEN** every query type SHALL complete

### Requirement: Batched picking
The system SHALL accept many rays in one call and return their hit records together, so that a host need not pay a boundary crossing per ray.

#### Scenario: Stroke resolved in one call
- **WHEN** a stroke's sample positions are submitted as a batch
- **THEN** one call SHALL return a hit record per sample

### Requirement: Picking is bounded and reports its cost
A batched pick SHALL be cancellable, SHALL report progress for large batches, and SHALL respect the configured memory ceiling.

#### Scenario: Cancelling a large batch
- **WHEN** a batch of many thousands of rays is cancelled
- **THEN** it SHALL stop promptly and return no partial record as if complete
