# project-io Specification

## Purpose
Persist editable project state, assets and recovery records reproducibly.

## Requirements

### Requirement: One lossless container
The system SHALL define a single container format that round-trips a document losslessly: texture sets, layers, masks, groups, filters, material graphs, node groups, stroke and export presets, mesh map bindings, resource references, channel descriptors, editable entries, replay records and required checkpoints, and document settings.

#### Scenario: Round trip
- **WHEN** a document is saved and reopened
- **THEN** every entry, graph, binding and setting SHALL compare equal to the original

### Requirement: Backward-open reading
A reader SHALL open a file written by a newer version of the library, preserving the parts it does not understand rather than failing, and SHALL report what it did not understand. Re-saving SHALL preserve those parts.

#### Scenario: Newer file on an older build
- **WHEN** a file containing an unknown entry kind is opened
- **THEN** it SHALL open, the unknown parts SHALL be reported, and saving SHALL write them back unchanged

### Requirement: Versioning
The container SHALL carry a schema version readable without decoding the body, and the version SHALL have a single source of truth shared with the library version.

#### Scenario: Probing a file
- **WHEN** a file's version is queried
- **THEN** it SHALL be readable without decoding the document

### Requirement: Pixel storage
Layer pixel data SHALL be stored per tile, compressed, with the compression identified in the container so a future reader can decode an older file.

#### Scenario: Sparse document
- **WHEN** a 16384 texture set has painted content in one corner
- **THEN** only the occupied tiles SHALL be stored

### Requirement: Referenced versus packed resources
Resources SHALL be referenced by relative path by default and SHALL be packable into the container on request. A packed document SHALL open with no external file present.

#### Scenario: Sharing a project
- **WHEN** a document is saved with packing enabled
- **THEN** it SHALL open on another machine with every image, font and map present

#### Scenario: Missing unpacked resource
- **WHEN** a referenced resource is absent and not packed
- **THEN** it SHALL be reported by identifier, the document SHALL still open, and the dependent input SHALL be marked missing

### Requirement: Save is atomic
A save SHALL either produce a complete file or leave the previous file untouched. A partially written file SHALL never replace a good one.

#### Scenario: Interrupted save
- **WHEN** the process is killed during a save
- **THEN** the previously saved file SHALL remain intact and readable

### Requirement: Autosave and recovery
The system SHALL support periodic autosave to a recovery location at a configurable interval, and SHALL expose a query for recoverable documents after an abnormal exit.

#### Scenario: Recovering after a crash
- **WHEN** the host restarts after a crash
- **THEN** it SHALL be able to enumerate and open the most recent autosave

#### Scenario: Autosave does not block
- **WHEN** an autosave runs
- **THEN** painting SHALL remain responsive and the autosave SHALL capture a consistent snapshot

### Requirement: Mesh is referenced, not embedded by default
The mesh SHALL be referenced by path and identity by default and SHALL be embeddable on request.

#### Scenario: Reopening after the mesh moved
- **WHEN** a document whose mesh path no longer resolves is opened
- **THEN** it SHALL open with the mesh reported as missing and SHALL accept a replacement through the mesh replacement path

### Requirement: Import of individual assets
The system SHALL import materials, smart materials, smart masks, brushes, stroke presets, export presets and node groups as standalone files, using the same container with the relevant parts populated.

#### Scenario: Importing a material file
- **WHEN** a standalone material file is imported
- **THEN** it SHALL be added to the library without opening it as a document

### Requirement: Export of individual assets
Any material, smart material, smart mask, brush, stroke preset, export preset or node group SHALL be exportable as a standalone file, optionally self-contained.

#### Scenario: Sharing a smart material
- **WHEN** a smart material is exported self-contained
- **THEN** the file SHALL carry its resources and import cleanly elsewhere

### Requirement: Document memory and size reporting
The system SHALL report a document's in-memory footprint broken down by texture set, channel, history and bound maps, and SHALL estimate its on-disk size before a save.

#### Scenario: Before saving a large document
- **WHEN** the estimated size is queried
- **THEN** it SHALL be reported without performing the save

### Requirement: Untrusted input is bounded
The reader SHALL treat every file as untrusted: declared sizes SHALL be validated against the actual data, allocations SHALL be bounded by a configurable ceiling, and malformed input SHALL be refused by name rather than trusted.

#### Scenario: Declared size exceeds the file
- **WHEN** a container declares a tile larger than the remaining bytes
- **THEN** the read SHALL be refused with a diagnostic and no allocation of the declared size SHALL occur

#### Scenario: Fuzzing gate
- **WHEN** the reader is fuzzed in CI
- **THEN** no input SHALL produce a crash, an out-of-bounds access or an unbounded allocation

### Requirement: Deterministic writing
Saving an unchanged document twice SHALL produce byte-identical files.

#### Scenario: Reproducible save
- **WHEN** a document is saved twice without edits
- **THEN** the two files SHALL be byte-identical

### Requirement: Snapshot save and recovery records
Save SHALL capture one committed revision, request asynchronous pixels only where the container needs them, and preserve pinned replay inputs and checkpoint versions. Incremental recovery writes SHALL become discoverable only after atomic publication. A file with unsupported replay algorithms SHALL retain usable raster checkpoints and report which edits cannot be replayed.

#### Scenario: Older reader lacks a replay algorithm
- **WHEN** a project contains a stroke algorithm unknown to the reader but a supported raster checkpoint
- **THEN** the raster content SHALL remain available and the reader SHALL report replay unavailable without substituting a different algorithm
