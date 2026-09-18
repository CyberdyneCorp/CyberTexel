# host-transport — Telling A Host What Changed

## ADDED Requirements

### Requirement: Revisions
Every channel of every texture set SHALL carry a monotonically increasing revision that advances whenever its content changes. A host SHALL be able to read a revision without reading pixels.

#### Scenario: Cheap change detection
- **WHEN** a host compares a stored revision against the current one
- **THEN** it SHALL learn whether anything changed without transferring any pixel data

### Requirement: Per-tile revisions
Each tile of each channel SHALL carry its own revision, so that a host can determine exactly which tiles to re-upload rather than which channels.

#### Scenario: A small stroke on a large canvas
- **WHEN** a stroke dirties twelve tiles of a 16384 texture set
- **THEN** exactly twelve tile revisions SHALL have advanced

### Requirement: Delta query
A host SHALL be able to ask, given a revision it last synchronized, for the set of tiles that have changed since. The answer SHALL be complete: no changed tile shall be omitted.

#### Scenario: Incremental upload
- **WHEN** a host queries the delta since its last synchronization
- **THEN** it SHALL receive every tile it must re-upload and no tile it need not

#### Scenario: Synchronizing after many operations
- **WHEN** a host has not synchronized for twenty operations
- **THEN** one delta query SHALL return the union of the tiles those operations dirtied, coalesced rather than repeated

### Requirement: Revision overflow and reset
The revision scheme SHALL define its behaviour on wraparound, and a host holding a revision the system can no longer relate to the present SHALL be told to resynchronize fully rather than given an incorrect delta.

#### Scenario: Stale revision
- **WHEN** a host presents a revision too old to be answered precisely
- **THEN** the system SHALL report that a full resynchronization is required rather than return a partial delta

### Requirement: Tile readback
A host SHALL be able to read a named set of tiles into caller-owned buffers, in a declared layout, without reading the whole channel.

#### Scenario: Uploading only what changed
- **WHEN** a host reads the tiles named by a delta query
- **THEN** it SHALL receive exactly those tiles in the declared layout

### Requirement: Declared memory layout
The tile layout crossing the boundary — tile dimensions, row pitch, channel order, component type and whether tiles are contiguous — SHALL be declared and stable, so a host can upload without repacking.

#### Scenario: Direct upload
- **WHEN** a host uploads a read tile to its own texture
- **THEN** the declared layout SHALL allow it without an intermediate repack

### Requirement: Format negotiation
A host SHALL declare the texture formats it can accept, and readback SHALL deliver in one of them or report that no common format exists, rather than silently converting to something the host must convert again.

#### Scenario: Host without 16-bit support
- **WHEN** a host declares it cannot accept 16-bit-per-channel textures and the document is 16-bit
- **THEN** either a declared conversion SHALL be applied and reported, or the mismatch SHALL be reported, and the choice SHALL be the host's

### Requirement: Synchronization is a snapshot
A delta query and the readback that follows it SHALL observe a consistent state: an edit made between them SHALL NOT produce a torn result, and SHALL instead appear in the next delta.

#### Scenario: Editing during synchronization
- **WHEN** a stroke completes between a delta query and its readback
- **THEN** the readback SHALL deliver the state as of the query, and the new stroke SHALL appear in the following delta

### Requirement: Cost is proportional to the change
The cost of a delta query SHALL be proportional to the number of changed tiles, not to the number of tiles in the document.

#### Scenario: Unchanged large document
- **WHEN** a delta query runs on a large document with nothing changed
- **THEN** it SHALL return an empty set in time independent of the document's tile count

### Requirement: Preview transport
The in-flight preview of an operation SHALL be transportable by the same mechanism, so a host can display a stroke before it is committed without a separate path.

#### Scenario: Previewing a stroke
- **WHEN** a stroke is in progress
- **THEN** the host SHALL obtain the affected tiles' preview content through the same delta and readback calls

### Requirement: Host-owned resources are tracked by identity
Where a host holds its own copy of a resource the library produced, the library SHALL provide a stable identity for it, so the host can key its cache without relying on pointer values or ordinal positions.

#### Scenario: Caching a compiled pipeline
- **WHEN** a host caches a pipeline built from an emitted pass plan
- **THEN** it SHALL be able to key that cache by a stable identity the library provides

### Requirement: Transport is reachable from every binding
Revision reads, delta queries and tile readback SHALL be reachable from Python, Swift and Rust, with the layout declaration expressed in each language's terms.

#### Scenario: A Rust host synchronizes
- **WHEN** a Rust host runs the synchronization loop
- **THEN** it SHALL do so through the safe crate without dropping to raw pointers

### Requirement: Transport has a stated budget
The cost of a delta query and of tile readback SHALL carry declared budgets on the reference device, gated as `device-gate` requires.

#### Scenario: Synchronization budget
- **WHEN** the synchronization benchmark runs on the reference device
- **THEN** its figures SHALL be compared against the declared budgets and a miss SHALL fail the gate
