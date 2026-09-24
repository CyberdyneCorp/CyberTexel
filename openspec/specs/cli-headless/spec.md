# cli-headless Specification

## Purpose
Provide scriptable, display-free project and material workflows from the command line.

## Requirements

### Requirement: A dedicated binary
The project SHALL ship a command-line binary that drives the library with no window, no interaction and no GPU requirement, so the same engine an artist drives can run in CI, on a farm and inside another tool's build step.

#### Scenario: Running on a build machine
- **WHEN** the binary runs on a headless machine with no GPU
- **THEN** every subcommand SHALL complete through the CPU reference executor

### Requirement: Subcommands
The binary SHALL provide, at minimum: `export` (write texture sets from a document), `bake-request` (drive an attached bake provider and bind the results), `apply` (apply a smart material or preset to named texture sets), `run` (execute a Python script against a document), `info` (report a document's texture sets, layers, channels, bound maps and estimated sizes), and `validate` (check a document, a material or a preset without producing output).

#### Scenario: Re-exporting after a mesh update
- **WHEN** `export` runs with a document, a replacement mesh and a preset
- **THEN** the mesh SHALL be swapped, the document reconciled per the documented policy, and the texture sets written

### Requirement: Argument validation
Every argument SHALL be validated before any work begins. An invalid or missing argument SHALL be reported by name with the accepted values, and SHALL NOT produce partial output.

#### Scenario: Unknown preset
- **WHEN** `export` is given a preset name that does not resolve
- **THEN** the run SHALL fail naming the preset and listing the available ones, and no file SHALL be written

### Requirement: Exit codes distinguish outcomes
The binary SHALL return distinct exit codes for success, invalid arguments, a missing or unreadable input, an unsupported operation, a resource or mesh map that could not be resolved, cancellation, exceeding a declared budget, and an internal error.

#### Scenario: Missing mesh map
- **WHEN** a run fails because a required mesh map is not bound
- **THEN** the exit code SHALL differ from the one used for invalid arguments, and the diagnostic SHALL name the map

### Requirement: Machine-readable reports
Every run SHALL be able to emit a machine-readable report instead of prose: the inputs resolved, the operations performed, every file written with its dimensions, format, colour space and bit depth, the executor used, any fallback, any clamped parameter, and timings.

#### Scenario: Pipeline consumption
- **WHEN** an automated pipeline runs `export` with reporting enabled
- **THEN** it SHALL locate every output and detect every clamp and fallback from the report alone

### Requirement: Quiet and verbose output
Prose output SHALL be suppressible, and diagnostics SHALL go to the error stream so that the report on the output stream stays parseable when both are used.

#### Scenario: Piping a report
- **WHEN** the report is piped to a consumer while diagnostics are emitted
- **THEN** the piped stream SHALL contain only the report

### Requirement: Determinism
A given document, arguments and executor SHALL produce byte-identical outputs across runs and across machines of the same platform.

#### Scenario: Repeat export
- **WHEN** the same export runs twice
- **THEN** the written files SHALL be byte-identical

### Requirement: Executor selection
The binary SHALL allow selecting the executor by flag and by the documented environment variable, SHALL report which executor was used, and SHALL report any fallback to the CPU reference.

#### Scenario: Forcing the reference executor
- **WHEN** the CPU reference executor is selected explicitly
- **THEN** it SHALL be used regardless of available devices, and the report SHALL name it

### Requirement: Budgets are honoured
The binary SHALL accept the memory ceiling, texel ceiling and worker bound as flags, and SHALL refuse an operation that would exceed them, naming the request and the ceiling.

#### Scenario: Over budget on a farm node
- **WHEN** an export exceeds the supplied memory ceiling
- **THEN** it SHALL be refused before allocation and the exit code SHALL be the budget code

### Requirement: Cancellation
The binary SHALL handle an interrupt signal by cancelling cooperatively, leaving no partially written file, and exiting with the cancellation code.

#### Scenario: Interrupted export
- **WHEN** an export of twenty textures is interrupted after eight
- **THEN** no partial file SHALL remain and the exit code SHALL be the cancellation code

### Requirement: Scripting is the extension point
`run` SHALL execute a Python script against an open document using the same binding the tests use, so that any operation reachable from Python is reachable from the command line without a new flag.

#### Scenario: A pipeline-specific operation
- **WHEN** a studio needs an operation the subcommands do not cover
- **THEN** it SHALL be expressible as a Python script driven by `run`

### Requirement: Help is complete and accurate
`--help` SHALL list every subcommand and flag with its accepted values and default, and a gate SHALL verify the help text matches the implemented flags.

#### Scenario: Flag added without documentation
- **WHEN** a flag is added but not documented in the help text
- **THEN** the help consistency gate SHALL fail naming the flag

### Requirement: Smoke-tested in CI
The binary SHALL be exercised end to end in CI on every supported desktop platform, covering each subcommand and each exit code path.

#### Scenario: Release gate
- **WHEN** a release is prepared
- **THEN** the CLI smoke tests SHALL have run on each desktop platform, and an unexercised binary SHALL fail the gate
