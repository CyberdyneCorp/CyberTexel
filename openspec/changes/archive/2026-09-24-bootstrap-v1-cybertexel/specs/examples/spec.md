# examples — The Gallery That Also Checks The Project

## Purpose
Use executable examples as end-to-end evidence for the public library surface.

## ADDED Requirements

### Requirement: Examples are Python and they are the check
Examples SHALL be Python scripts driven through the Python binding, and they SHALL be the project's end-to-end verification rather than a separate demonstration tier. An example both shows a capability to a human and asserts it for CI.

#### Scenario: One artefact, two jobs
- **WHEN** an example runs
- **THEN** it SHALL assert its own result and produce its committed output, and a failure of either SHALL fail CI

### Requirement: Every capability has an example
Each capability SHALL have at least one numbered example that exercises it. A capability without one SHALL fail the coverage gate.

#### Scenario: New capability added
- **WHEN** a capability is added with no example
- **THEN** the example coverage gate SHALL fail naming the capability

### Requirement: Examples assert rather than illustrate
Every example SHALL make explicit assertions about what it produced — pixel values at named coordinates, channel statistics, entry counts, file manifests, reported budgets — and SHALL fail rather than print on a mismatch.

#### Scenario: A blend mode changes
- **WHEN** a change alters a blend mode's formula
- **THEN** the example asserting that mode's output SHALL fail with the expected and actual values

### Requirement: Committed outputs
Each example SHALL commit its output — rendered images, exported texture sets, reports — and CI SHALL compare freshly produced output against the committed form within a stated tolerance.

#### Scenario: A change breaks a picture
- **WHEN** a change alters an example's rendered output beyond tolerance
- **THEN** CI SHALL fail with the difference, rather than the change being discovered when someone next looks

#### Scenario: An intended visual change
- **WHEN** an output changes deliberately
- **THEN** the committed output SHALL be updated in the same commit as the change that caused it

### Requirement: Minimal dependencies
An example SHALL depend on nothing beyond the installed wheel and numpy. No example shall require a GPU, a display, a network connection, a sibling engine or an asset that is not in the repository.

#### Scenario: Clean environment
- **WHEN** the examples run in an environment containing only the wheel and numpy
- **THEN** every one SHALL complete

#### Scenario: Mesh maps without a baker
- **WHEN** an example needs mesh maps and no bake provider is attached
- **THEN** it SHALL use the fixture map set committed to the repository

### Requirement: Fixture assets
The repository SHALL carry a small fixture set — meshes with UVs and UDIM layouts, a mesh map set, brush alphas, images and fonts — sufficient for every example, with each asset's provenance and licence recorded.

#### Scenario: Asset provenance
- **WHEN** the attribution file is read
- **THEN** every fixture asset SHALL appear with its origin and licence

### Requirement: Examples are deterministic
An example SHALL produce identical output across runs and across machines of the same platform, seeding every stochastic operation explicitly.

#### Scenario: Particle example
- **WHEN** the particle example runs twice
- **THEN** its output SHALL be identical, because its seed is set in the script

### Requirement: Examples run on the reference executor by default
Examples SHALL run on the CPU reference executor by default, and SHALL be runnable against any other available executor to serve as a parity check.

#### Scenario: Examples as a parity check
- **WHEN** the examples are run against a GPU executor
- **THEN** their outputs SHALL be compared against the reference within the parity tolerance

### Requirement: Examples are readable
An example SHALL be short enough to read in one sitting, SHALL explain in a header comment what it demonstrates and which capability it covers, and SHALL avoid helper indirection that obscures the API being shown.

#### Scenario: Learning the API
- **WHEN** a new consumer reads an example
- **THEN** the calls they must make SHALL be visible in the file rather than hidden behind repository-local helpers

### Requirement: A worked end-to-end example
At least one example SHALL carry a model from mesh and mesh maps through texture sets, layers, a material graph, a smart material and export, so that the whole pipeline is demonstrated in one readable script.

#### Scenario: The full path
- **WHEN** the end-to-end example runs
- **THEN** it SHALL produce an exported texture set from a supplied mesh without any other input

### Requirement: Examples cover the host-executed route
At least one example SHALL exercise the host-executed route by receiving a pass plan and returning results, using a software stand-in so that it runs with no GPU.

#### Scenario: Host-executed contract exercised headlessly
- **WHEN** the host-executed example runs in CI
- **THEN** the pass plan's completeness SHALL be exercised without a device

### Requirement: Examples are timed against the budgets
Examples SHALL record their timings, and where an example exercises a budgeted operation on a reference device, the figure SHALL feed `device-gate` rather than being reported separately.

#### Scenario: Example feeds the gate
- **WHEN** the stroke example runs on a reference device
- **THEN** its timing SHALL be recorded against the stroke budget

### Requirement: The gallery is published
The examples' committed outputs SHALL be presented together in the documentation, so the project's current capability is visible without building anything.

#### Scenario: Reading the gallery
- **WHEN** the documentation is read
- **THEN** each example SHALL appear with its output, its description and the capability it covers
