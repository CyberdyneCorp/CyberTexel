# build-packaging — Build, Layering, Gates And Release

## ADDED Requirements

### Requirement: C++20 CMake build
The project SHALL build with CMake as strict C++20 with warnings as errors on its own code, and SHALL provide presets for every supported platform and backend combination.

#### Scenario: Preset build
- **WHEN** any shipped preset is configured and built
- **THEN** it SHALL compile with no warnings from the project's own sources

### Requirement: Core builds headless
The core library SHALL compile and pass its tests with no GUI toolkit, no graphics backend and no platform SDK beyond a C++20 toolchain.

#### Scenario: Minimal configuration
- **WHEN** the headless preset is built on a machine with no GPU SDK installed
- **THEN** the build SHALL succeed and the test suite SHALL pass

### Requirement: One task runner is the entry point
The repository SHALL provide a `justfile` as the single documented entry point for building, testing, formatting and running every gate. A contributor SHALL NOT need to know the underlying CMake, pytest, cargo or npm invocation to perform any routine task.

The recipe set SHALL cover, at minimum: `build`, `test`, `format`, `format-check`, `examples`, `bench`, `clean`, a `check` recipe that runs everything possible without a device, and one recipe per gate — layering, licence, parity, determinism, budgets, binding parity, example coverage, capability index, version consistency and ABI diff.

#### Scenario: A contributor builds from a clean clone
- **WHEN** a contributor clones the repository and runs the build recipe with the documented toolchain installed
- **THEN** it SHALL succeed without further instructions

#### Scenario: Every gate is reachable by name
- **WHEN** the recipe list is displayed
- **THEN** each gate named in this specification SHALL appear as a recipe whose name identifies it

### Requirement: An unimplemented gate fails
A gate recipe whose implementation does not exist yet SHALL exit non-zero and name the task that delivers it. It SHALL NOT succeed, and it SHALL NOT be absent from the recipe list.

#### Scenario: Gate invoked before it is built
- **WHEN** a gate recipe is run before the task implementing it is done
- **THEN** it SHALL fail, and its output SHALL name the task in the plan that delivers it

#### Scenario: A vacuous pass is impossible
- **WHEN** the full check recipe runs on a tree where a gate is unimplemented
- **THEN** the run SHALL fail rather than report success for a gate that checked nothing

### Requirement: CI runs the same recipes
Continuous integration SHALL invoke the task runner's recipes rather than reimplementing their commands, so that a check cannot pass locally and differ in CI or the reverse.

#### Scenario: A gate's command changes
- **WHEN** a gate's implementation changes
- **THEN** the change SHALL take effect in CI without editing a workflow file

#### Scenario: Drift is detectable
- **WHEN** a CI workflow step runs a command that is not a recipe
- **THEN** it SHALL be treated as a defect in the workflow, because the same command is then defined in two places

### Requirement: The task runner is a documented prerequisite
The task runner and every other tool a contributor needs SHALL be listed with a minimum version in the contributor documentation, and a recipe SHALL report a missing prerequisite by name rather than failing with the underlying tool's error.

#### Scenario: Missing toolchain
- **WHEN** a recipe runs on a machine lacking one of its prerequisites
- **THEN** it SHALL name the missing tool and the version required

### Requirement: Module layering is enforced
The module dependency rule SHALL be enforced by a build gate rather than by convention. No core module except the C ABI composition boundary SHALL depend on the executor module, and no module below the C ABI shall depend on a graphics backend.

#### Scenario: Cycle introduced
- **WHEN** a source file creates a dependency cycle between modules
- **THEN** the layering gate SHALL fail the build naming both modules

#### Scenario: Backend leak
- **WHEN** a module outside the executor includes a graphics API header
- **THEN** the gate SHALL fail naming the file

### Requirement: Permissive-only dependencies
Every dependency compiled into a shipped binary SHALL be permissively licensed under MIT, BSD, Apache-2.0, MPL-2.0, zlib or an equivalent. GPL and LGPL code SHALL NOT be linked.

#### Scenario: Licence gate
- **WHEN** a GPL-licensed dependency is added
- **THEN** the CI licence audit SHALL fail naming the dependency

### Requirement: The audit's subject is what ships
The licence audit SHALL cover what is compiled into a shipped binary, including vendored trees outside the manifest and dependencies fetched at configure time, and each entry's licence SHALL be recorded from that dependency's own licence text.

#### Scenario: Vendored tree
- **WHEN** a third-party compiler is vendored under the source tree
- **THEN** it SHALL appear in the attribution file with its own licence text and pinned revision, and its absence SHALL fail the audit

### Requirement: Test pyramid
The project SHALL maintain unit tests per module, integration tests driven through the Python binding, the cross-executor parity fixture, and example programs that assert their own output. All SHALL run in CI.

#### Scenario: Release gate
- **WHEN** a release is prepared
- **THEN** every tier SHALL pass, and a tier that could not run SHALL be reported as unmeasured rather than counted as passed

### Requirement: Sanitizer and fuzzing gates
CI SHALL run the test suite under address and undefined-behaviour sanitizers on at least one platform, and SHALL fuzz the container reader and every parser.

#### Scenario: Memory error
- **WHEN** a change introduces an out-of-bounds access exercised by the suite
- **THEN** the sanitized run SHALL fail

### Requirement: Determinism gate
CI SHALL verify that emitted shader source, saved documents and exported textures are byte-identical across repeated runs on one platform.

#### Scenario: Non-deterministic output
- **WHEN** an unordered iteration makes emitted source vary between runs
- **THEN** the determinism gate SHALL fail

### Requirement: Single source of truth for the version
The library version SHALL be defined in one place and consumed by the build, the C ABI version query, the package metadata for all three bindings and the container's writer version.

#### Scenario: Version drift
- **WHEN** a package declares a version other than the single source of truth
- **THEN** the version consistency gate SHALL fail naming the package

### Requirement: ABI diff gate
Each release SHALL produce a symbol and descriptor diff against the previous release, and a removal or an incompatible change SHALL fail the gate unless the major version is incremented.

#### Scenario: Removed symbol
- **WHEN** an exported symbol is removed without a major version bump
- **THEN** the gate SHALL fail naming the symbol

### Requirement: Platform packages
The project SHALL produce packages for macOS, Linux, Windows, iOS and Android, each containing the C header, the library, the licence and the attribution file, and each SHALL be smoke-tested by a program that links it.

#### Scenario: Unexercised package
- **WHEN** a package is produced but its smoke test does not run
- **THEN** the release gate SHALL fail rather than publish an unexercised binary

### Requirement: Reference host example
The repository SHALL contain desktop and mobile reference hosts that exercise the host-executed route end to end on real device APIs in the first painting slice, and they SHALL be built in CI. Actual-device runtime checks SHALL be release gates; unavailable devices SHALL be reported as unmeasured. These integration hosts SHALL be distinct from the display-free Python examples.

#### Scenario: Host-executed route stays honest
- **WHEN** the pass plan's contents change
- **THEN** the reference host SHALL be updated and CI SHALL fail until it builds and runs again

### Requirement: Benchmarks with declared floors
The project SHALL carry benchmarks for stroke application, layer compositing, emission, generator evaluation and export, each with a declared floor on a named reference device. A case SHALL count as measured only where its figure clears the floor the gate compares against.

#### Scenario: Unreachable budget
- **WHEN** a declared floor is one no configuration can reach
- **THEN** it SHALL be reported as unreachable rather than recorded as passed

### Requirement: Reproducible builds of the shipped artifacts
Building the same commit twice with the same toolchain SHALL produce identical library binaries where the toolchain supports it, and any unavoidable non-determinism SHALL be documented.

#### Scenario: Repeat build
- **WHEN** the same commit is built twice
- **THEN** the produced libraries SHALL be identical, or the differing input SHALL be named in the documentation
