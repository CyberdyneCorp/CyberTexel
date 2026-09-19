# Stroke model and paint engine scenario coverage

`just test-stroke-model-scenarios` and `just test-paint-engine-scenarios` run
the CTests carrying the corresponding scenario label. These matrices map every
OpenSpec scenario to executable headless coverage. A row explicitly naming a
later task records a cross-capability assertion that cannot exist yet; the
implementing task must extend this suite rather than silently treating the row
as covered.

## Stroke model

| OpenSpec scenario | CTest | Covered behavior |
|---|---|---|
| One resolution, many consumers | `paint-scenarios` | One resolved sequence drives coverage, rejection and deposition; preview/history integration follows with task 3.9 |
| Frame rate does not change the result | `stroke-reconstruction` | Redundant time-linear samples and input batch boundaries preserve the sequence |
| Fast flick leaves no gaps | `stroke-reconstruction`, `paint-coverage` | Swept segments span distant stamps and continuous coverage fills the interval |
| Default pen mapping | `stroke-reconstruction` | Default pressure changes radius alone |
| Mouse input | `stroke-reconstruction` | Missing pressure evaluates every enabled mapping at full pressure |
| Re-resolution is stable | `stroke-reconstruction` | Equal seed and input produce byte-equal jittered stamps |
| Tapered stroke | `stroke-reconstruction` | Ten-stamp entry taper starts at its floor and reaches full value on stamp ten |
| Smoothed stroke | `stroke-reconstruction` | Timestamp-grid stabilization follows the documented exponential recurrence |
| Ruler line | `stroke-reconstruction` | Straight-line constraint ignores intermediate positions |
| Three-plane symmetry | `stroke-reconstruction` | All mirror planes emit eight transformed stamps; single-step undo follows with task 3.9 |
| Radial symmetry | `stroke-reconstruction` | Six instances are evenly rotated around the selected object axis |
| Host owns the stroke engine | `stroke-reconstruction` | External stamps pass unchanged without spacing, jitter or taper |
| Older preset loads | `stroke-preset` | Missing flow-jitter field receives its documented default |
| Newer preset is refused | `stroke-preset` | Unknown schema version is named and leaves destination state unchanged |
| Cancelled stroke | `paint-preview` | Cancellation discards copy-on-write tiles; absence of a history step follows with task 3.9 |
| Separated textured tips | `stroke-reconstruction`, `paint-coverage` | Discrete transformed tips retain visible gaps instead of gaining sweeps |
| Batched input | `paint-scenarios` | Coalesced and one-sample batches produce identical stamps and build-up deposition |

## Paint engine

| OpenSpec scenario | CTest | Covered behavior |
|---|---|---|
| Occluded surface is painted | `paint-coverage`, `paint-rejection` | UV-space fragments are camera-independent and disabling depth retains occluded contributions |
| Off-screen island | `paint-coverage` | Texture-space rasterization writes geometry without screen-space visibility input |
| No gaps at speed | `paint-coverage` | Continuous swept coverage fills the interval between distant stamps |
| Hard brush | `paint-coverage` | Hardness one yields exact inside/outside coverage at the radius |
| Triplanar on poor UVs | `paint-coverage` | Squared-normal world-axis projections do not depend on UV distortion |
| Painting through the mesh | `paint-rejection` | Disabling depth accepts both front and occluded contributions |
| Symmetry and depth rejection | `paint-rejection` | Each symmetry instance requires consistent depth or reports explicit disabling |
| Painting near a hard edge | `paint-rejection` | Default normal-dot threshold rejects a perpendicular face |
| Back faces excluded | `paint-rejection` | Counter-clockwise geometric normals drive configurable backface rejection |
| Faint edge | `paint-rejection`, `paint-deposition` | Precision threshold suppresses writes without losing accumulated deposition |
| Scribbling in place | `paint-deposition` | Non-building overlap remains at the single-pass strength |
| A second stroke accumulates | `paint-blending` | A new stroke snapshots the previous result and blends again |
| Low flow build-up | `paint-deposition` | Repeated flow 0.1 events approach full deposition gradually |
| Multiply brush | `paint-blending` | Multiply uses the stroke-start snapshot and canonical formula |
| Two masks intersect | `paint-masking` | Colour-ID and rectangle weights multiply before deposition |
| Seam does not show under mipmapping | task 9.17 | Requires declared mip range, gutter validation and mip fixtures |
| Gradient preserved | `paint-seam-dilation` | Directional extrapolation continues the source gradient |
| Long stroke | `paint-seam-dilation` | All dirty tiles are staged across frames and dilated once at finalization |
| Island fill reuses the cache | `paint-surface-cache` | Face and island operations share one revision-keyed cache bundle |
| Mesh replaced | `paint-surface-cache` | A mesh revision clears coverage, triangle and island maps before lookup |
| Small stroke on a large canvas | `paint-work` | A 16K canvas processes only four reachable tiles without scanning the grid |
| Preview matches commit | `paint-preview` | Final preview and committed tiles are byte-identical after dilation |
| Mirrored island under minification | task 9.17 | Requires tangent-frame seam adjacency and minification fixtures |
| Insufficient gutter | task 9.17 | Requires island-owned padding and unsupported-mip reporting |
