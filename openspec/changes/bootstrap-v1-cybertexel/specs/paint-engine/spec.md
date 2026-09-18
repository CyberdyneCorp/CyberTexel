# paint-engine — Texture-Space Rasterization

## ADDED Requirements

### Requirement: Texture-space rasterization
Paint SHALL be applied by rasterizing the mesh with its UV coordinates emitted as clip-space position, so that one rasterized fragment corresponds to one destination texel. The result SHALL NOT depend on whether the texel's surface is visible on screen, except through the explicit rejection tests.

#### Scenario: Occluded surface is painted
- **WHEN** depth rejection is disabled and a stroke covers a surface hidden behind nearer geometry
- **THEN** the hidden surface's texels SHALL be written

#### Scenario: Off-screen island
- **WHEN** a UV island's surface lies outside the camera frustum but under the stroke's world path
- **THEN** its texels SHALL be written

### Requirement: Swept coverage
For each candidate texel the system SHALL compute the distance from the texel's surface position to the swept segment between consecutive stamps, and SHALL reject the texel when that distance exceeds the stamp radius. Coverage SHALL NOT be computed by stamping discrete discs.

#### Scenario: No gaps at speed
- **WHEN** consecutive stamps are further apart than one radius
- **THEN** the region between them SHALL be covered

### Requirement: Falloff
Strength SHALL fall off from the swept axis as `t2 = clamp((t - hardness) / (1 - hardness), 0, 1)` followed by `1 - t2 * t2 * (3 - 2 * t2)`, multiplied by the stamp opacity, where `t` is the distance from the axis normalized by the radius. A hardness of 1 SHALL produce a hard edge at the radius.

#### Scenario: Hard brush
- **WHEN** hardness is 1.0
- **THEN** strength SHALL be the full opacity inside the radius and zero outside it

### Requirement: Coordinate modes
Material sampling SHALL support UV, triplanar and planar projection modes. Triplanar SHALL blend three world-axis projections weighted by the squared surface normal. Planar SHALL project through a caller-supplied frame.

#### Scenario: Triplanar on poor UVs
- **WHEN** triplanar mode is used on a mesh with distorted UVs
- **THEN** the sampled material SHALL be continuous across the distortion

### Requirement: Depth rejection
Depth rejection SHALL be enabled by default and SHALL discard texels whose surface is occluded along the view direction, using a configurable bias. It SHALL be disableable per operation.

#### Scenario: Painting through the mesh
- **WHEN** depth rejection is disabled
- **THEN** front and back surfaces under the stroke SHALL both be painted

#### Scenario: Symmetry and depth rejection
- **WHEN** a mirrored stamp is applied
- **THEN** depth rejection for that stamp SHALL be evaluated against a depth buffer consistent with the mirrored transform, or SHALL be disabled, and the choice SHALL be reported rather than left implicit

### Requirement: Angle rejection
Angle rejection SHALL be enabled by default and SHALL discard texels whose surface normal deviates from the reference normal at the stamp's surface hit by more than a configurable threshold, defaulting to a dot product of 0.5.

#### Scenario: Painting near a hard edge
- **WHEN** angle rejection is enabled at the default threshold and a stroke is placed near a cube's edge
- **THEN** the perpendicular adjacent face SHALL NOT be painted

### Requirement: Backface handling
The system SHALL support discarding texels whose surface faces away from the view direction, configurable per operation, and SHALL document the winding convention it uses.

#### Scenario: Back faces excluded
- **WHEN** backface rejection is enabled
- **THEN** texels belonging to away-facing triangles SHALL not be written

### Requirement: Alpha discard
Texels whose resulting strength falls below a configurable threshold, defaulting to 0.1 for 8-bit channels and 0.004 for 16-bit and floating-point channels, SHALL be discarded.

#### Scenario: Faint edge
- **WHEN** a soft brush produces strength below the threshold at its outer edge
- **THEN** those texels SHALL not be written

### Requirement: Per-stroke coverage accumulation
Within one stroke the system SHALL accumulate a coverage value per texel and SHALL blend using the maximum of the current strength and the accumulated coverage, so that self-overlap within a single stroke does not accumulate opacity. Coverage SHALL reset at stroke start.

#### Scenario: Scribbling in place
- **WHEN** a semi-transparent brush is scribbled back and forth without lifting
- **THEN** the overlapped region SHALL not be darker than a single pass

#### Scenario: A second stroke accumulates
- **WHEN** the pointer is lifted and the same region is painted again
- **THEN** opacity SHALL accumulate

### Requirement: Flow separate from opacity
Flow SHALL control the per-stamp deposition rate and opacity SHALL control the stroke's maximum accumulation, and the two SHALL be independently settable.

#### Scenario: Low flow build-up
- **WHEN** flow is 0.1 and opacity is 1.0
- **THEN** repeated overlapping stamps within one stroke SHALL build toward full opacity rather than reaching it immediately

### Requirement: Blending against a stroke-start snapshot
Blending SHALL be evaluated in the shading stage against the channel content as it stood at stroke start, not against the partially painted result, so that every blend mode in `texture-document` is available while painting.

#### Scenario: Multiply brush
- **WHEN** the brush blend mode is Multiply
- **THEN** the result SHALL be the multiply of the stroke colour with the pre-stroke colour interpolated by the stroke strength

### Requirement: Masking inputs
Paint SHALL be restricted by, in combination: the active layer's masks, a colour-ID selection, a geometry or polygon-fill selection, a rectangular or lasso screen selection, and a UV-island selection. A texel excluded by any active mask SHALL not be written.

#### Scenario: Two masks intersect
- **WHEN** a colour-ID selection and a rectangular selection are both active
- **THEN** only texels satisfying both SHALL be painted

### Requirement: UV seam dilation
After a paint operation the system SHALL dilate written texels outward across UV borders by a configurable radius, defaulting to 2 texels, by searching outward for the nearest covered texel and extrapolating along the found direction rather than copying it. A radius of 0 SHALL disable dilation.

#### Scenario: Seam does not show under mipmapping
- **WHEN** a painted island is exported and mipmapped
- **THEN** the dilated border SHALL prevent background bleed at the seam

#### Scenario: Gradient preserved
- **WHEN** a gradient runs off the edge of an island
- **THEN** the dilated texels SHALL continue the gradient rather than repeat the edge value

### Requirement: Dilation is deferred within a stroke
Dilation SHALL be performed once per stroke rather than per stamp, and SHALL cover every tile the stroke dirtied.

#### Scenario: Long stroke
- **WHEN** a stroke spans many frames
- **THEN** dilation SHALL run once at its end and SHALL cover the whole dirtied region

### Requirement: Cached UV-space maps
The system SHALL cache, per texture set, a coverage map marking texels covered by geometry, a triangle identity map, and a UV island identity map. Caches SHALL be invalidated when the mesh or UV set changes and SHALL be reusable across operations.

#### Scenario: Island fill reuses the cache
- **WHEN** a UV island fill follows a face fill on the same mesh
- **THEN** the coverage map SHALL be reused rather than rebuilt

#### Scenario: Mesh replaced
- **WHEN** the mesh is replaced
- **THEN** all three caches SHALL be invalidated before the next operation reads them

### Requirement: Bounded work
A paint operation SHALL process only the tiles its stamps and their dilation can reach, and SHALL report the tile set it processed.

#### Scenario: Small stroke on a large canvas
- **WHEN** a short stroke is painted on a 16384 by 16384 texture set
- **THEN** the processed tile count SHALL be proportional to the stroke, not to the canvas

### Requirement: Preview without commit
The system SHALL be able to produce the visual result of an in-flight stroke without modifying the document, and the committed result SHALL equal the last preview within the parity tolerance.

#### Scenario: Preview matches commit
- **WHEN** a stroke is previewed and then committed
- **THEN** the committed pixels SHALL match the final preview within tolerance
