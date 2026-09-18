# language-bindings — Python, Swift And Rust

## ADDED Requirements

### Requirement: Three official bindings
The system SHALL ship Python, Swift and Rust bindings over the C ABI. Each SHALL be built and tested in CI.

#### Scenario: All three build
- **WHEN** CI runs
- **THEN** the Python wheel, the Swift package and the Rust crates SHALL each build and their test suites SHALL pass

### Requirement: Parity with the C ABI
Every operation reachable through the C ABI SHALL be reachable through each binding. A binding is not permitted to be a convenience subset.

#### Scenario: Parity gate
- **WHEN** the parity gate runs
- **THEN** any C entry point without a counterpart in Python, Swift or Rust SHALL fail the gate naming the entry point and the binding

### Requirement: Python is the test harness
The Python binding SHALL be the harness the integration suite and the examples are written in, so that a capability unreachable from Python is an untested capability.

#### Scenario: Integration suite
- **WHEN** the integration suite runs
- **THEN** it SHALL drive the library through the Python binding rather than through internal C++ calls

### Requirement: Python is numpy-native
Pixel data, mesh buffers and map data SHALL cross the Python boundary as numpy arrays without copying where the memory layout permits, and the copying behaviour SHALL be documented per call.

#### Scenario: Reading a channel
- **WHEN** a channel's pixels are read into Python
- **THEN** a numpy array of the documented dtype and shape SHALL be returned, and whether it is a view or a copy SHALL be documented

### Requirement: Python errors are exceptions
A failed call SHALL raise a typed Python exception carrying the result code and the diagnostic, not return an error value.

#### Scenario: Missing mesh map
- **WHEN** a generator requiring an unbound map is applied from Python
- **THEN** a typed exception naming the missing map SHALL be raised

### Requirement: Python packaging
The Python binding SHALL ship as a wheel for the supported desktop platforms and Python versions, SHALL declare its dependencies, and SHALL depend on nothing beyond numpy at run time.

#### Scenario: Clean environment install
- **WHEN** the wheel is installed into an environment containing only numpy
- **THEN** it SHALL import and run the examples

### Requirement: Swift package
The Swift binding SHALL ship as a SwiftPM package with a system target over the C header and a Swift layer above it, resolvable without repository-relative native paths, and SHALL support iOS and macOS.

#### Scenario: Resolving from another repository
- **WHEN** an application resolves the package by URL and version
- **THEN** it SHALL build without the consumer supplying paths into this repository

### Requirement: Swift is idiomatic and memory-safe
The Swift layer SHALL present value types, optionals and thrown errors rather than raw pointers and result codes, and SHALL manage handle lifetime automatically.

#### Scenario: Handle lifetime
- **WHEN** a Swift document value goes out of scope
- **THEN** the underlying handle SHALL be destroyed exactly once

### Requirement: Rust crates
The Rust binding SHALL ship as two crates: a `-sys` crate that builds the native library and exposes raw bindings, and a safe wrapper crate that is the only place `unsafe` appears above the `-sys` layer.

#### Scenario: Unsafe is contained
- **WHEN** the workspace is audited
- **THEN** `unsafe` SHALL appear only in the `-sys` crate and in the wrapper's documented boundary modules

### Requirement: Rust is idiomatic
The safe crate SHALL present ownership through Rust types, errors through `Result` with a typed error enumeration, and SHALL be `Send` and `Sync` exactly where the C ABI's threading contract permits.

#### Scenario: Threading contract in the type system
- **WHEN** a document handle is shared between threads without synchronization
- **THEN** the compiler SHALL reject it unless the C ABI declares it safe

### Requirement: Host-executed route is reachable from every binding
The host-executed execution route — receiving source and a pass plan, returning results — SHALL be usable from Python, Swift and Rust.

#### Scenario: A wgpu host in Rust
- **WHEN** a Rust host requests a pass plan and executes it on its own device
- **THEN** it SHALL be able to return the results through the safe crate

### Requirement: Versions move together
The bindings SHALL carry the same version as the library, derived from the single source of truth, and a binding SHALL refuse to load a native library whose ABI major version it was not built against.

#### Scenario: Mismatched native library
- **WHEN** a binding loads a native library with an incompatible ABI major version
- **THEN** it SHALL fail at load naming both versions

### Requirement: Examples per binding
Each binding SHALL ship runnable examples covering document creation, painting, material authoring, mesh map binding, smart material application and export, and they SHALL run in CI.

#### Scenario: Example gate
- **WHEN** CI runs
- **THEN** every binding's examples SHALL execute and assert their results

### Requirement: Documented ownership and lifetime rules
Each binding SHALL document who owns each buffer that crosses the boundary and how long it stays valid.

#### Scenario: A returned view's lifetime
- **WHEN** a binding returns a view over library memory
- **THEN** the conditions under which it becomes invalid SHALL be documented and enforced by the language where possible
