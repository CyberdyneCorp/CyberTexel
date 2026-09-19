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

## Recovery discovery

Each session updates `<recovery-key>.ctex-recovery`, leaving at most one current
candidate per document. `enumerate_recoverable_projects` probes only these
files, without decoding their bodies, and returns candidates newest first with
their key, path, schema version, timestamp, and byte size. Truncated or malformed
candidate files are returned separately in `rejected`; unrelated files and
interrupted atomic-save temporary files are ignored. This lets a restarting
host offer valid recovery candidates without treating a corrupt file as a
project.
