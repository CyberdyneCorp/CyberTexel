# Host-executed operations

`ctex/exec/host_execution.hpp` defines the device-free completion protocol for
the primary host route. `HostExecutedExecutor` is always compiled in. It reports
`host-not-attached` until a host attaches its own device, then becomes available;
the library never receives a graphics API object or calls that device.

## Submission boundary

A `HostSubmissionRequest` names the operation, its exact base document revision,
its replay semantics, and every resource crossing the boundary. Each handoff
contains:

- a logical resource ID and generation, format, extent, mip count and tile shape;
- whether the logical content is library- or host-owned;
- the state the host must establish for that handoff; and
- whether the resource is an output generation.

The pass plan remains the ordered description of per-pass access and state. The
host protocol adds lifetime and publication around that plan. Submitting pins
every named generation and returns a monotonic completion token. A second write
cannot reuse a generation that is already retained.

No submission, completion, revision query, cancellation, or recovery call moves
pixels. Host-resident results therefore remain resident through ordinary paint,
composite, preview, undo and redo. Explicit asynchronous readback belongs to the
host transport in task 8.4.

## Completion and publication

Successful completion must return every declared output exactly once with the
declared generation, format and extent. A wrong count, identity, format or size
is rejected and the document revision remains unchanged. Size diagnostics name
both actual and expected dimensions.

Publication is a single state transition. It occurs only when:

1. execution completed successfully;
2. the submission base still equals the current document revision;
3. every output matches its declaration; and
4. valid recovery evidence is already retained.

Publication advances the revision once and makes the output generations the
authoritative committed resources. A completion that lost a race is stale.
Duplicate, failed, rejected and unknown-token completions never publish.
Replacing a committed logical resource retires its old generation only after
the submission using it has completed.

Cancellation marks executing work but keeps its resources pinned until the host
reports completion. A late success after cancellation is discarded and releases
the submission holds without advancing revision or history. Work that already
finished but is awaiting recovery can be cancelled immediately because no queue
work still references it.

## Recovery before fallback

Deterministic operations may publish with either a completed result checkpoint
or a completed base checkpoint plus a versioned operation record and pinned
inputs. Checkpoint-only operations require a completed result checkpoint. If
execution wins before its asynchronous checkpoint, the completion enters
`awaiting-recovery`: outputs stay pinned and nothing publishes until
`establish_recovery` succeeds.

The session retains recovery evidence for the current commit and reports its
accounted bytes. On device loss it restores that revision, makes all uncommitted
submissions terminal, and reports their released generations. Only a successful
recovery report can be passed to the checked fallback-report factory to describe
CPU fallback. Recovery records and checkpoint persistence are expanded by
editable-authoring tasks 20.1–20.2; memory admission is added in 19.1–19.2.
