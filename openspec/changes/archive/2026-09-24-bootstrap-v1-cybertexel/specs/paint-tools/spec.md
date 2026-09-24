# paint-tools — The Tool Set

## Purpose
Provide the authored tool behaviors used to edit texture channels and masks.

## ADDED Requirements

### Requirement: Tool inventory
The system SHALL provide the tools Brush, Eraser, Fill, Clone, Blur, Smear, Decal, Stencil, Projection, Text, Particle, Picker, Colour ID and Selection. Every tool SHALL inherit the masking, rejection and symmetry behaviour defined in `paint-engine` without restating it.

#### Scenario: Symmetry applies to every tool
- **WHEN** mirror symmetry is enabled and any tool is used
- **THEN** its effect SHALL be mirrored

### Requirement: Brush
The Brush SHALL apply the active material along the stroke into the enabled channels of the active layer, honouring radius, opacity, flow, hardness, rotation, blend mode and coordinate mode.

#### Scenario: Painting a stroke
- **WHEN** the Brush is dragged over the mesh with a material selected
- **THEN** the material SHALL be applied along the swept path into every enabled channel

### Requirement: Eraser
The Eraser SHALL reduce the active layer's per-texel opacity along the stroke. On a mask it SHALL reduce the mask value.

#### Scenario: Erasing on a layer
- **WHEN** the Eraser is dragged over painted content
- **THEN** the covered texels' opacity SHALL fall in proportion to the stroke strength

### Requirement: Fill modes
Fill SHALL support the scopes Whole set, Triangle, Connected-by-angle, UV island, UV tile and Selection. Triangle fill SHALL resolve by exact triangle identity. Connected-by-angle SHALL expand across triangles whose normals lie within the angle threshold. UV island fill SHALL flood the cached island map from the picked texel.

#### Scenario: Face fill
- **WHEN** Triangle scope is used and a triangle is picked
- **THEN** exactly that triangle's texels SHALL be filled

#### Scenario: Island fill
- **WHEN** UV island scope is used and a texel is picked
- **THEN** the connected island containing it SHALL be filled and no other island SHALL be touched

### Requirement: Clone
Clone SHALL copy from a source location to the painted location by offsetting in UV space, with the offset fixed at stroke start. The source SHALL be settable independently of painting, and SHALL be expressible as aligned (offset follows the stroke) or fixed (offset anchored).

#### Scenario: Aligned clone
- **WHEN** a source is set and an aligned clone stroke is painted
- **THEN** content from the constant offset SHALL be copied along the stroke

#### Scenario: Cloning across texture sets is refused
- **WHEN** a clone source lies in a different texture set from the destination
- **THEN** the operation SHALL be refused with a diagnostic naming both sets

### Requirement: Blur and Smear
Blur SHALL apply a separable blur of configurable radius over a snapshot taken at stroke start. Smear SHALL drag content along the stroke direction with a configurable strength.

#### Scenario: Blur does not feed back
- **WHEN** a blur stroke crosses its own path
- **THEN** the result SHALL be computed from the stroke-start snapshot rather than from the already-blurred result

### Requirement: Decal
Decal SHALL project the active material onto the surface through a frame built from the surface normal at the placement point, with position, rotation, uniform and per-axis scale. A placed decal SHALL remain editable after gesture commit when retained as an editable entry; rasterization SHALL be explicit as defined by `editable-authoring`.

#### Scenario: Placing a sticker
- **WHEN** a decal is placed and then rotated before commit
- **THEN** the projected result SHALL follow the rotation without re-picking the surface

### Requirement: Stencil
Stencil SHALL constrain paint through an image anchored in screen space, with position, rotation and scale, and SHALL support inverting the constraint.

#### Scenario: Painting through a stencil
- **WHEN** a stencil is active
- **THEN** paint SHALL appear only where the stencil is opaque, in a screen-anchored position that does not follow the camera's model

### Requirement: Projection
Projection SHALL apply a texture through a camera or planar frame onto the surface, and SHALL support a tri-planar variant.

#### Scenario: Photo projection
- **WHEN** a photograph is projected through the current view
- **THEN** it SHALL be applied to the visible surface under the projection frame

### Requirement: Text
Text SHALL rasterize a string in a supplied font and apply it as a decal. It SHALL support per-string size, tracking and alignment, and SHALL handle UTF-8 input.

#### Scenario: Applying a label
- **WHEN** a UTF-8 string is applied with a loaded font
- **THEN** the rasterized text SHALL be projected onto the surface at the requested size

### Requirement: Particle
Particle SHALL emit simulated particles that collide with the mesh and deposit at their contact points, with configurable count, lifetime, initial speed, mass, gravity, friction, restitution and randomness. The simulation SHALL be deterministic for a given seed.

#### Scenario: Deterministic spray
- **WHEN** a particle stroke is replayed with the same seed
- **THEN** the deposited pattern SHALL be identical

### Requirement: Picker
The Picker SHALL read every enabled channel's value at a surface point and report them together, and SHALL optionally select the material that produced them.

#### Scenario: Reading a surface
- **WHEN** a painted surface is picked
- **THEN** the base colour, roughness, metallic, normal, height, occlusion, emission and subsurface values at that point SHALL be reported

### Requirement: Colour ID selection
The system SHALL accept a colour-ID map and SHALL select the region matching a picked colour, within a configurable tolerance. The selection SHALL be usable as a paint restriction, as a mask source and as a visibility filter.

#### Scenario: Selecting a region
- **WHEN** a colour is picked from a colour-ID map
- **THEN** subsequent paint SHALL be restricted to texels whose ID colour matches within tolerance

#### Scenario: Tolerance of zero
- **WHEN** the tolerance is zero and the map has been resampled with filtering
- **THEN** the selection SHALL be reported as empty rather than silently widened

### Requirement: Selection tool
The Selection tool SHALL produce a screen-space rectangle or lasso, and a polygon-fill selection by triangle, by UV island or by connected-by-angle expansion. A selection SHALL be storable as a mask.

#### Scenario: Restricting to a region
- **WHEN** a lasso selection is active
- **THEN** paint outside it SHALL be rejected

### Requirement: Parameter defaults and ranges
Every tool parameter SHALL have a documented default, minimum and maximum, and SHALL be validated identically at every entry point, reporting any value it clamped.

#### Scenario: Out-of-range radius
- **WHEN** a radius above the documented maximum is supplied through any binding
- **THEN** it SHALL be clamped, the clamp SHALL be reported, and the same clamp SHALL occur through the C ABI, Python, Swift and Rust

### Requirement: No inert parameters
Every documented tool parameter SHALL be able to change the output. A parameter that cannot SHALL be removed or wired through rather than retained as a placeholder.

#### Scenario: Parameter audit
- **WHEN** the parameter audit test runs
- **THEN** each documented parameter SHALL be shown to alter the result of at least one operation

### Requirement: Editable surface paths
The system SHALL support a path of control points attached to the mesh surface, evaluated through stroke-model and retained as an editable-authoring entry. Position, width and material edits SHALL be undoable; mesh replacement SHALL use the reprojection policy and report invalid attachments.

#### Scenario: Editing a painted seam line
- **WHEN** a control point of a committed surface path is moved
- **THEN** the path SHALL be re-evaluated without leaving its previous rasterized trace and undo SHALL restore the original path
