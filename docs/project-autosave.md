# Project snapshots, autosave, and recovery

`capture_project_snapshot` captures one host-supplied committed project
revision. The caller supplies `ProjectSnapshotMetadata` (schema, resources,
standalone assets, and opaque sections, with no pixel vectors) plus the live
tiled images that belong to that revision. Capture visits only allocated tiles
and pins their immutable storage handles; it does not copy, compress, or write
pixel payloads. It also verifies that each image revision stayed fixed during
capture. The host should call it at a committed edit boundary, then may resume
painting immediately. Later writes use the tiled image's copy-on-write storage,
so materializing the snapshot still observes the captured pixels.

`ProjectSaveSnapshot` reports its committed revision, image count, and retained
pixel bytes. `materialize_project_snapshot` performs the deferred pixel copies,
while `save_project_snapshot_atomic` additionally compresses and atomically
publishes the result. Those operations are intended for a worker, not the paint
thread.

Snapshot metadata retains standalone [operation records](operation-records.md)
alongside their named checkpoint images. Because both are normal project
sections, periodic autosave and recovery preserve their canonical bytes and
pinned inputs without a separate replay sidecar.

## Periodic autosave

`ProjectAutosaveSession` owns one worker and one stable recovery path. Its
configuration declares a recovery directory, a portable recovery key, and a
positive interval. `submit` moves a captured snapshot into the session without
performing compression or filesystem I/O. A newer pending revision replaces an
older one, and revisions no newer than the pending, writing, or published
revision are rejected as stale. At the interval boundary, the worker writes the
newest pending snapshot through the same atomic-save path used for normal
projects.

`status` exposes pending, writing, and last-published revisions, the successful
write count, and the last error. `wait_until_idle` observes normal periodic
completion with a caller-supplied timeout. `flush` requests immediate
publication and waits; it is suitable for lifecycle boundaries, but not the
paint loop. Destroying a session waits only for a write already in progress and
discards a merely pending snapshot.

Every autosave embeds its non-zero project revision in a dedicated checkpoint
section inside the atomically published container. The bytes and revision
therefore become durable as one filesystem operation; there is no sidecar that
can name a different revision after interruption. Normal recovery reads remove
this internal lifecycle section from the returned canonical project bytes.

## Suspend and resume

The lifecycle quiesce operation combines a resource ledger with an autosave
session. The host must submit the newest committed snapshot before quiescing and
provides its current revision, deadline, and optional cancellation callback.
Quiesce closes the ledger admission gate first, requests immediate checkpoint
publication, invokes cancellation, and waits within the one shared deadline for
active reservations to drain and the requested revision to become durable.
The cancellation callback may release or destroy reservations, but must not
destroy the ledger or autosave session during the call.

The report distinguishes a durable checkpoint, an exceeded deadline, and a
checkpoint failure. It includes remaining active work, the last durable
revision, and the uncheckpointed revision range when the current revision is
newer. Missing a deadline never claims pending edits survived and leaves
admission closed until the host explicitly resumes it. Atomic publication means
interrupted temporary files are never offered as recovery candidates.

## Recovery discovery

Each session updates `<recovery-key>.ctex-recovery`, leaving at most one current
candidate per document. `enumerate_recoverable_projects` probes only these
files, without decoding their bodies, and returns candidates newest first with
their key, path, schema version, timestamp, and byte size. Truncated or malformed
candidate files are returned separately in `rejected`; unrelated files and
interrupted atomic-save temporary files are ignored. This lets a restarting
host offer valid recovery candidates without treating a corrupt file as a
project.

## C boundary

`ctex_project_autosave_session_create` configures the same worker and stable
recovery path. `submit` accepts bounded encoded project-container bytes, restores
their sparse tiled images long enough to capture pinned tile storage, and owns
the resulting snapshot before returning. The caller may then release or mutate
its source bytes. Validation and capture are synchronous; compression and file
publication remain on the worker. Status, timed wait and explicit flush expose
the complete lifecycle without polling the filesystem.

`ctex_project_recovery_enumerate` uses caller-owned arrays plus packed strings
for valid and rejected candidates. `ctex_project_recovery_read` reads a selected
candidate under normal project-container limits and returns canonical bytes and
the JSON inventory through the standard atomic two-call contract.
`ctex_project_recovery_resume` performs the same bounded read while also
returning the revision embedded in the durable checkpoint. Legacy recovery
files remain readable and explicitly report that no revision metadata exists.
