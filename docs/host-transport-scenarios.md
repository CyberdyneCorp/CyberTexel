# Host transport scenario coverage

`just test-host-transport-scenarios` runs every CTest carrying the
`host-transport-scenario` label. The matrix maps every OpenSpec host-transport
scenario, using executable coverage where the implementation exists and naming
the later cross-capability task for integrations that do not exist yet.

| OpenSpec scenario | CTest | Covered behavior |
|---|---|---|
| Cheap change detection | `host-transport-revisions` | Channel and tile revisions are read without allocating or transferring pixels |
| A small stroke on a large canvas | `host-transport-revisions` | Exactly twelve logical tiles advance on a 16,384-square texture |
| Incremental upload | `host-transport-delta`, `host-transport-readback` | Delta records carry revision, generation and residency; CPU tiles alone enter caller buffers |
| Synchronizing after many operations | `host-transport-delta` | Twenty edits coalesce into the complete row-major changed-tile union |
| Stale revision | `host-transport-delta` | An unrelated revision epoch returns full-resynchronization instead of a partial delta |
| Uploading only what changed | `host-transport-readback` | Only explicitly named versions and buffers are read |
| Direct upload | `host-transport-layout` | Visible edge-tile bytes and row pitch feed the upload stand-in without repacking |
| Host without 16-bit support | `host-transport-format` | Exact-only refusal and host-selected 16-to-8-bit conversion are both explicit |
| Editing during synchronization | `host-transport-snapshot` | A pinned R readback remains immutable and the following delta reports R+1 |
| Unchanged large document | `host-transport-delta`, `host-transport-snapshot` | The 16K current-cursor path visits no index entries and fits a zero-byte snapshot budget |
| Previewing a stroke | `host-transport-preview` | Isolated preview pixels use the normal delta, snapshot, layout, conversion and readback types |
| Caching a compiled pipeline | `host-transport-identity` | A reconstructed or reordered plan resolves the same structured pipeline cache identity |
| A Rust host synchronizes | `test-rust-binding`, `test-swift-binding`, `test-python-binding`, `gate-binding-parity` | Safe Rust, Swift and Python all reach the host transport and the complete raw C operation set |
| Synchronization budget | `host-transport-delta`; numeric gate deferred to 17.6 | Work-count invariants are executable now; reference-device latency limits land with performance gates |
| Saving during painting | `host-transport-snapshot`, `host-transport-readback`, `c-abi-host-transport-snapshot`; save integration deferred to 12.3–12.4 | The transport returns pinned R while R+1 advances; public asynchronous readbacks retain R and release it after success, cancellation, failure or destruction |
| Slow reader retains old tiles | `host-transport-snapshot` | Unique pinned bytes remain reported, admission respects the ceiling, and release restores capacity |

The labeled suite deliberately tests the host-neutral C++ contract. Binding
wrappers must exercise this same loop rather than introduce a second transport
surface. Likewise, task 17.6 must measure the implemented delta and readback
paths on the named reference device; the constant-work fixtures here are not a
substitute for latency budgets.
