# stroke-model Specification

## Purpose
Represent authored strokes as replayable, testable operations.

## Requirements

### Requirement: A stroke resolves to ordered stamps
A stroke SHALL be a path of input samples that resolves, once, into an ordered sequence of stamps. A stamp SHALL carry a position and coordinate frame, a radius, an opacity, a hardness, a rotation, an elongation, a flow value, a tip resource identity and an ordinal index. Everything downstream SHALL consume stamps, never raw input samples.

#### Scenario: One resolution, many consumers
- **WHEN** a stroke is resolved
- **THEN** the paint engine, the preview and the history SHALL all read the same stamp sequence

### Requirement: Spacing
Stamp spacing SHALL be expressed as a fraction of the stamp radius, SHALL default to 0.1, and SHALL accept values from 0.01 to 4.0. Spacing SHALL be measured along the resolved path so that a fast pointer produces the same stamps as a slow one over the same path.

#### Scenario: Frame rate does not change the result
- **WHEN** two sample streams describe the same piecewise-linear position and attribute path, with equivalent timestamps when stabilization is enabled, differing only by redundant samples
- **THEN** the resolved stamp sequence SHALL be identical within the positional tolerance

### Requirement: Continuous coverage between stamps
The stroke model SHALL emit, alongside the stamps, the swept segments between consecutive stamps, so that a consumer may cover the path continuously rather than by discrete stamping.

#### Scenario: Fast flick leaves no gaps
- **WHEN** the pointer moves further in one frame than the stamp radius
- **THEN** the emitted segments SHALL cover the intervening path

### Requirement: Pressure and tilt response
Pressure SHALL be mappable to radius, opacity, hardness, flow and rotation, each independently enabled, each through its own response curve. Tilt SHALL be mappable to rotation and to an elongation factor. A device that reports no pressure SHALL be treated as full pressure rather than as zero.

#### Scenario: Default pen mapping
- **WHEN** a pressure-sensitive pen is used with default settings
- **THEN** radius SHALL respond to pressure and opacity, hardness, flow and rotation SHALL not

#### Scenario: Mouse input
- **WHEN** input arrives from a device reporting no pressure
- **THEN** every pressure-mapped property SHALL evaluate at full pressure

### Requirement: Deterministic jitter
Jitter SHALL be available on position, radius, rotation, opacity and flow, and SHALL be driven by a seeded generator keyed by the stroke seed and the stamp ordinal, so that re-resolving a stroke produces identical jitter.

#### Scenario: Re-resolution is stable
- **WHEN** a stroke is resolved twice with the same seed
- **THEN** the two stamp sequences SHALL be identical

### Requirement: Taper
A stroke SHALL support entry and exit taper over a distance or a stamp count, applied to radius, opacity or both.

#### Scenario: Tapered stroke
- **WHEN** entry taper is set over 10 stamps
- **THEN** the first stamp's tapered property SHALL be at the taper floor and the tenth at full value

### Requirement: Stabilizer
The system SHALL provide a stabilizer with a radius and a time constant. Timestamped samples SHALL be reconstructed on a documented fixed time grid before stabilization; the resolved cursor SHALL move toward the boundary of the radius by the factor 1 - exp(-delta_time / time_constant), with zero time constant applying the full correction. Non-monotonic timestamps SHALL be rejected. Batching identical samples SHALL NOT change the result.

#### Scenario: Smoothed stroke
- **WHEN** the stabilizer radius is non-zero and the pointer moves erratically
- **THEN** the resolved path SHALL be smoother than the raw path and SHALL follow the documented recurrence; the radius SHALL define a dead zone rather than a maximum lag guarantee

### Requirement: Constraints
The system SHALL support a straight-line constraint between the stroke origin and the current sample, an axis constraint locking to the dominant axis, and a grid constraint snapping positions to a configurable step.

#### Scenario: Ruler line
- **WHEN** the straight-line constraint is active
- **THEN** the resolved path SHALL be the segment from the origin to the current sample regardless of the intermediate samples

### Requirement: Symmetry planes
A stroke SHALL be mirrorable across any combination of the X, Y and Z object planes and across a radial symmetry of a configurable count about a chosen axis. Mirrored stamps SHALL be emitted as part of the same stroke so they share one history step.

#### Scenario: Three-plane symmetry
- **WHEN** all three mirror planes are enabled
- **THEN** each stamp SHALL yield eight stamps in total and a single undo SHALL reverse all of them

#### Scenario: Radial symmetry
- **WHEN** radial symmetry of 6 about Z is enabled
- **THEN** each stamp SHALL yield six stamps evenly rotated about the object's Z axis

### Requirement: Externally resolved stamps
A host SHALL be able to submit an already-resolved stamp sequence instead of input samples, bypassing this capability's resolution entirely.

#### Scenario: Host owns the stroke engine
- **WHEN** a host that already resolves strokes with its own engine submits stamps directly
- **THEN** the paint engine SHALL consume them unchanged and no spacing, jitter or taper SHALL be re-applied

### Requirement: Stroke presets are versioned
A stroke configuration SHALL be serializable as a named preset carrying a schema version. A preset from an older version SHALL load with fields it does not name taking their documented defaults; a preset from a newer version SHALL be refused by name rather than partially applied.

#### Scenario: Older preset loads
- **WHEN** a preset written before flow jitter existed is loaded
- **THEN** it SHALL load and flow jitter SHALL take its documented default

#### Scenario: Newer preset is refused
- **WHEN** a preset declaring a schema version this build does not know is loaded
- **THEN** it SHALL be refused with a diagnostic naming the version, and no partial state SHALL be applied

### Requirement: Cancellation leaves no trace
An in-flight stroke SHALL be cancellable, and cancelling SHALL leave the document byte-identical to its state before the stroke began.

#### Scenario: Cancelled stroke
- **WHEN** a stroke in progress is cancelled
- **THEN** the affected tiles SHALL be byte-identical to their pre-stroke content and no history step SHALL be recorded

### Requirement: Continuous and discrete tip modes
A stroke preset SHALL declare continuous-sweep or discrete-alpha tip mode independently from its deposition mode. Continuous mode SHALL fill swept segments; discrete mode SHALL apply the alpha at each resolved stamp using its rotation, scale and elongation. Spacing SHALL define deposition events in both modes and SHALL define visible tip separation in discrete mode.

#### Scenario: Separated textured tips
- **WHEN** a discrete-alpha brush uses spacing larger than its tip diameter
- **THEN** the tips SHALL remain separated and the engine SHALL NOT fill the gap with swept coverage

### Requirement: Input reconstruction limits
The sample interpolation rule, positional tolerance, fixed stabilization time step and timestamp units SHALL be documented and versioned with the stroke preset. Sampling invariance SHALL apply only to equivalent reconstructed paths and attributes; omitted corners or pressure changes SHALL NOT be claimed recoverable.

#### Scenario: Batched input
- **WHEN** the same timestamped samples arrive individually or in coalesced batches
- **THEN** committed stamps and deposition SHALL be identical
