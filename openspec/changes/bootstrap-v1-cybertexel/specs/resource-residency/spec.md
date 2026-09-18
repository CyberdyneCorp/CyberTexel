# resource-residency — Bounded Desktop And Mobile Resources

## ADDED Requirements

### Requirement: Complete resource accounting
The system SHALL expose CPU-resident, GPU-resident, backing-store and pinned/in-flight byte counts by document storage, history, recovery records, mesh maps, composites, caches and temporary resources. Shared allocations SHALL carry allocation identities so physical memory totals do not double-count CPU/GPU views of the same allocation. Hosts SHALL report device allocations through API-independent descriptors.

#### Scenario: Unified memory accounting
- **WHEN** one shared allocation is accessible from CPU and GPU
- **THEN** the report SHALL show both access roles but count its physical bytes once

### Requirement: Enforced budgets and admission
Hosts SHALL set separate CPU, GPU, backing-store and temporary-storage ceilings. Before an operation is admitted, the system SHALL reserve its bounded working set, history and recovery requirements, evict reconstructible caches or schedule tiled work as necessary, and refuse an operation that cannot fit without changing committed content. Transient copies and pinned snapshots SHALL count toward the ceilings.

#### Scenario: Large export under a small budget
- **WHEN** export cannot fit as a whole-image allocation but can execute in tiles
- **THEN** it SHALL stream tiles within the ceilings and report peak resource use rather than allocate a full additional image

### Requirement: Sparse tiles and eviction
Unpainted constant tiles SHALL be represented without full pixel allocation. Clean derived tiles SHALL be evictable and reconstructible. Authored tiles SHALL be evictable only after lossless backing storage or a checkpoint and pinned operation inputs supporting bit-identical reconstruction on the selected executor exist. Replay within a nonzero parity tolerance alone SHALL NOT qualify for lossless eviction; such tiles SHALL require a lossless pixel checkpoint. Residency SHALL be independent of document resolution and UDIM addressing.

#### Scenario: Evicting authored content
- **WHEN** memory pressure evicts a painted tile and the artist later revisits it
- **THEN** the tile SHALL be restored with identical authored content and no history loss

### Requirement: Host-controlled quality policy
Hosts SHALL be able to reduce preview resolution, defer derived work or release caches under pressure. Such changes SHALL be reported and SHALL NOT silently resize authored data, reduce stored precision or change export results. If no allowed policy fits, the operation SHALL return over-budget.

#### Scenario: Lower quality preview
- **WHEN** a mobile host lowers preview resolution during memory pressure
- **THEN** the authored document and full-quality export SHALL remain unchanged

### Requirement: Suspend and resume
The library SHALL expose a host-driven quiesce operation that stops admitting edits, cancels or finishes in-flight work, and reports when a consistent recovery checkpoint is durable. The host SHALL supply backing-store access and its lifecycle deadline. A missed deadline SHALL be reported, and resume after termination SHALL recover the last durable checkpoint and identify its revision without claiming unsaved edits survived.

#### Scenario: Mobile suspension interrupts readback
- **WHEN** suspension prevents the newest recovery checkpoint from becoming durable
- **THEN** resume SHALL open the last durable revision, report the uncheckpointed revision range when known, and never open a partially written checkpoint
