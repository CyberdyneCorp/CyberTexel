# c-abi — The Boundary Every Host Crosses

## ADDED Requirements

### Requirement: Symbol prefix and export surface
Every exported symbol SHALL begin with `ctex_`. The export map SHALL list only that prefix, so the library may be linked alongside siblings that export into other prefixes without ambiguity.

#### Scenario: Linked beside a sibling engine
- **WHEN** the library is linked into a host that also links a sibling exporting `cyber_*`
- **THEN** neither export map SHALL match the other's symbols

### Requirement: C linkage only
No exported signature SHALL name a C++ type, a template, a reference or a standard library container, and no exception SHALL cross the boundary.

#### Scenario: Consumable from C
- **WHEN** the public header is compiled by a C compiler
- **THEN** it SHALL compile without error

#### Scenario: An exception is caught at the boundary
- **WHEN** an internal operation throws
- **THEN** it SHALL be caught at the boundary and reported as an error code

### Requirement: Opaque handles
Objects SHALL be exposed as opaque handles created and destroyed by named calls. A handle's internal layout SHALL NOT be part of the ABI.

#### Scenario: Struct layout changes
- **WHEN** an internal structure gains a field
- **THEN** a host compiled against the previous header SHALL continue to work

### Requirement: Integer result codes
Every fallible call SHALL return an integer result code from a documented enumeration. Success SHALL be zero. Result codes SHALL distinguish invalid argument, missing resource, unsupported operation, out of memory, over budget, cancelled and internal error.

#### Scenario: Distinguishing failures
- **WHEN** a call fails because a resource is missing rather than because an argument is invalid
- **THEN** the two SHALL be reported by different codes

### Requirement: Diagnostic retrieval
A failed call SHALL make available a human-readable diagnostic naming what failed and the values involved, retrievable without allocating in the caller.

#### Scenario: Reading a diagnostic
- **WHEN** a call fails
- **THEN** the caller SHALL be able to retrieve a message naming the operation and the offending values

### Requirement: Caller-owned buffers
Data returned in bulk SHALL be written into caller-owned buffers. A call SHALL support being invoked once to learn the required size and again to fill it, and SHALL report the required size when the supplied buffer is too small rather than truncating silently.

#### Scenario: Two-call sizing
- **WHEN** a caller queries a layer list with a null buffer
- **THEN** the required count SHALL be reported and no data SHALL be written

#### Scenario: Buffer too small
- **WHEN** a supplied buffer is smaller than required
- **THEN** the call SHALL fail with the required size reported and SHALL NOT partially fill the buffer

### Requirement: Versioned descriptors
Structures passed across the boundary SHALL begin with their own size in bytes. The library SHALL read only the fields the declared size covers, and a field appended in a later version SHALL take its documented default for an older caller.

#### Scenario: Older caller, newer library
- **WHEN** a host compiled against an older header passes a descriptor
- **THEN** the library SHALL read only the declared fields and apply documented defaults to the rest

#### Scenario: Descriptor size is implausible
- **WHEN** a descriptor declares a size larger than the library's own structure
- **THEN** the call SHALL be refused rather than reading beyond what it understands

### Requirement: ABI version query
The library SHALL expose its ABI version as major, minor and patch integers and as a string, queryable before any other call.

#### Scenario: Host checks compatibility
- **WHEN** a host queries the ABI version at startup
- **THEN** it SHALL be able to refuse to proceed against an incompatible major version

### Requirement: ABI stability rules
Within a major version, existing symbols SHALL NOT be removed, existing signatures SHALL NOT change, enumeration values SHALL NOT be renumbered and descriptor fields SHALL NOT be reordered or repurposed. Additions SHALL be appended.

#### Scenario: Symbol diff gate
- **WHEN** a release is prepared
- **THEN** a symbol and descriptor diff against the previous release SHALL run, and a removal or a signature change SHALL fail the gate unless the major version is incremented

### Requirement: Thread safety is declared
Each call SHALL document its threading contract. Operations on distinct documents SHALL be safe concurrently. Operations on one document SHALL be externally synchronized unless documented otherwise.

#### Scenario: Two documents in two threads
- **WHEN** two threads operate on two different documents
- **THEN** both SHALL complete correctly with no external synchronization

### Requirement: No global mutable state across handles
Behaviour SHALL depend only on the handles passed in and on explicitly documented process-wide settings. There SHALL be no hidden global state that makes one document's behaviour depend on another's.

#### Scenario: Independent documents
- **WHEN** one document's settings are changed
- **THEN** another open document's behaviour SHALL be unaffected

### Requirement: Callbacks carry user data
Every callback SHALL take an opaque user-data pointer and SHALL document whether it may be invoked from a thread other than the caller's.

#### Scenario: Progress from a worker thread
- **WHEN** a progress callback is invoked from a worker
- **THEN** the documentation SHALL have stated that it may be, and the user-data pointer SHALL be passed through unchanged

### Requirement: Full-surface coverage
Every capability SHALL be reachable through the C ABI. A capability reachable only from C++ SHALL be treated as a defect.

#### Scenario: Coverage gate
- **WHEN** the coverage gate runs
- **THEN** every capability's operations SHALL be shown to have a C entry point

### Requirement: Host log sink
A host SHALL be able to install a log callback receiving a severity, a category, a message and its opaque user data, and SHALL be able to set a minimum severity. With no sink installed the library SHALL emit nothing to any stream of its own.

#### Scenario: Routing library logs
- **WHEN** a host installs a log sink
- **THEN** the library's diagnostics SHALL be delivered to it rather than written anywhere else

#### Scenario: Silent by default
- **WHEN** no sink is installed
- **THEN** the library SHALL write nothing to the standard output or error streams

### Requirement: Diagnostics are English with machine-readable codes
Diagnostic messages SHALL be English. Every condition a host may wish to present in another language SHALL also be identified by a stable machine-readable code, so that localization is the host's to perform and does not depend on parsing prose.

#### Scenario: A host localizes a failure
- **WHEN** a host presents a failure in another language
- **THEN** it SHALL key its translation on the stable code rather than on the message text

### Requirement: Allocation control
A host SHALL be able to supply allocation and deallocation callbacks used for the library's own long-lived allocations.

#### Scenario: Host allocator
- **WHEN** a host supplies an allocator
- **THEN** the library's long-lived allocations SHALL route through it
