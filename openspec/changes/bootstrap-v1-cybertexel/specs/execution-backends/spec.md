# execution-backends — Where The Passes Run

## ADDED Requirements

### Requirement: Three execution routes
The system SHALL provide three routes for running a pass plan: a host-executed route in which the host owns the device and the library never touches one, a CPU reference executor that owns no device at all, and an optional owned-GPU executor. The host-executed route and the CPU reference SHALL always be available; the owned-GPU executor SHALL be optional at build time.

#### Scenario: Host owns the device
- **WHEN** a host requests the host-executed route
- **THEN** the library SHALL return source and a pass plan, SHALL accept the resulting pixels back, and SHALL make no graphics API call itself

#### Scenario: Build with no GPU backend
- **WHEN** the library is built with every GPU backend disabled
- **THEN** it SHALL build, and every capability SHALL complete through the CPU reference executor

### Requirement: The CPU reference defines correctness
The CPU reference executor SHALL implement every operation and SHALL define the correct result. Every other executor SHALL be measured against it. No capability SHALL be reachable only on a GPU.

#### Scenario: CI machine with no GPU
- **WHEN** the full test suite runs on a machine with no GPU
- **THEN** every capability SHALL be exercised and SHALL pass

### Requirement: The CPU reference is an independent implementation
Because painting depends on depth and UV buffers that a GPU host renders, the CPU reference SHALL rasterize its own depth, UV and coverage buffers from the mesh and camera. The specification SHALL state that this is a reimplementation rather than a fallback, and the parity gate SHALL be the mechanism that keeps the two aligned.

#### Scenario: Reference rasterization
- **WHEN** the CPU reference runs a paint operation
- **THEN** it SHALL produce its own depth and UV buffers and SHALL apply the same rejection tests the GPU path applies

### Requirement: Declared parity tolerance
The specification SHALL state a per-channel tolerance for agreement between executors. Tolerance SHALL be stated separately for 8-bit, 16-bit and floating-point channels, and for values that pass through texture filtering.

#### Scenario: Tolerance is a number
- **WHEN** a parity comparison runs
- **THEN** it SHALL compare against the stated numeric tolerance rather than against an unqualified notion of equality

### Requirement: Parity fixture and gate
A committed fixture of documents, strokes, cameras and materials SHALL be rendered by every available executor and compared per texel against the CPU reference. The comparison SHALL run in CI and SHALL fail the build when an executor exceeds the tolerance.

#### Scenario: A backend drifts
- **WHEN** a GPU executor's result exceeds tolerance on a fixture case
- **THEN** CI SHALL fail naming the case, the channel and the measured deviation

#### Scenario: Unavailable device is reported, not skipped silently
- **WHEN** an executor is compiled in but its device is absent at test time
- **THEN** the gate SHALL report the executor as unmeasured rather than counting it as passed

### Requirement: Executor selection and fallback
A host SHALL be able to enumerate the available executors with their device names and capabilities, select one, and pin a default. When a selected executor fails at run time the system SHALL fall back to the CPU reference, complete the operation, and report the fallback rather than producing a partial result.

#### Scenario: Device lost mid-operation
- **WHEN** an owned-GPU executor loses its device during an operation
- **THEN** the operation SHALL complete on the CPU reference and the report SHALL name the failure and the fallback

#### Scenario: Selection without a rebuild
- **WHEN** a consumer that exposes no backend UI sets the documented environment variable to a compiled-in executor
- **THEN** that executor SHALL be the process default, and an unrecognized value SHALL leave the automatic choice in place

### Requirement: Device capability reporting
An executor SHALL report the feature set `shader-emission` consumes: binding budget, maximum texture dimension, supported texture formats, floating-point filtering and compute availability.

#### Scenario: Emission matches the device
- **WHEN** a host selects an executor and requests emission
- **THEN** emission SHALL receive that executor's reported feature set

### Requirement: Host-executed contract
In the host-executed route the library SHALL declare which resources it owns and which the host owns, SHALL declare the required state of every resource it hands over, and SHALL detect a host that returns results in a format or size it did not request.

#### Scenario: Host returns the wrong size
- **WHEN** a host returns a result whose dimensions differ from the pass plan's declaration
- **THEN** the library SHALL reject it with a diagnostic naming both sizes and SHALL leave the document unchanged

### Requirement: Asynchronous execution and cancellation
Long-running operations SHALL be cancellable cooperatively and SHALL report progress. Cancelling SHALL leave the document byte-identical to its state before the operation.

#### Scenario: Cancelling an expensive fill
- **WHEN** a fill over a 16K texture set is cancelled midway
- **THEN** it SHALL stop promptly and the document SHALL be byte-identical to its prior state

### Requirement: Worker bound
A host SHALL be able to bound the number of worker threads the CPU reference executor uses, including bounding it to one for reproducibility.

#### Scenario: Single-threaded reproducibility
- **WHEN** the worker bound is one
- **THEN** results SHALL be identical to the multi-threaded result

### Requirement: Memory ceiling
A host SHALL be able to declare a memory ceiling for executor working storage. An operation whose working set would exceed it SHALL be refused before allocation, naming the request and the ceiling, or SHALL be tiled to fit where the operation supports tiling.

#### Scenario: Over budget
- **WHEN** an operation's working set exceeds the declared ceiling and cannot be tiled
- **THEN** it SHALL be refused before any allocation, with both figures in the diagnostic

### Requirement: No module depends on an executor
No module other than the executor module itself SHALL depend on a backend or a graphics API, and the dependency rule SHALL be enforced by a build gate.

#### Scenario: Layering violation
- **WHEN** a source file outside the executor module includes a graphics API header
- **THEN** the layering gate SHALL fail the build naming the file
