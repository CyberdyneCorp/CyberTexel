# Versioned operation records

An `EditableOperationRecord` captures the deterministic inputs needed to replay
an eligible edit without consulting mutable shelf content. Each record carries
its schema version, stable identity, algorithm and preset identities and
versions, input document revision, deterministic seed, mesh content identity,
finite coordinate frame, channel descriptors, and a versioned operation
payload.

The replay class is explicit:

- `checkpoint_only` requires at least one raster checkpoint image;
- `same_resolution` may replay only at the recorded document resolution; and
- `resolution_independent` may be considered by a later resize/replay policy.

`assess_operation_replay` compares the recorded algorithm version with explicit
host-supported ranges. It distinguishes same-resolution recovery,
resolution-independent replay, checkpoint-only recovery, required checkpoint
resampling, and an unsupported algorithm. An unknown version never substitutes
a different implementation: replay is disabled and any named raster checkpoint
remains available with a diagnostic.

Clone, blur, and smear records that declare replay eligibility must own a pinned
resource with role `source-snapshot`; otherwise validation refuses the record.
They may instead declare checkpoint-only recovery. This prevents replay from
sampling a later mutable layer state.

## Pinned inputs and checkpoints

Pinned resources are copied into the record with a role and content identity.
Replay therefore observes the recorded bytes even if a shelf file is edited or
removed later. Resource roles and content identities must each be unique.

Checkpoint images are named by stable tiled-image identifiers. Packaging a
record as a project standalone asset records those identifiers as explicit image
dependencies. Pinned resource bytes remain inside the operation-record payload,
so they have no external project-resource dependency.

Project containers store the asset with kind `operation-record`, format version
1, and the canonical record bytes. The normal container framing, input limits,
atomic save, snapshots, autosave, and recovery preserve the record and its
raster dependencies without a separate sidecar.

## Canonical binary format

The little-endian format starts with `CTEXOPR\0` and a schema version. It is
length-delimited, deterministic, and rejects truncated data, trailing bytes,
unknown enum values, invalid channels, non-finite numeric values, duplicate
identities, incomplete metadata, and configured input or aggregate payload
limits. Serializing an unchanged record produces identical bytes.

## C ABI

`ctex_operation_record_create` validates a versioned descriptor and returns
canonical bytes plus a JSON inventory through caller-owned buffers.
`ctex_operation_record_inspect` validates existing bytes and returns the same
canonical representation and inventory. Both use the standard sizing-call
contract.

`ctex_project_container_upsert_operation_record` inserts or replaces a record
without replacing an asset of another kind.
`ctex_project_container_get_operation_record` retrieves and validates a named
record. Project-container read limits continue to bound the enclosing untrusted
container; operation-record decoding additionally applies its own bounded
defaults.

`ctex_operation_record_assess_replay` accepts explicit supported
algorithm-version ranges and reports the effective replay disposition as JSON
and typed fields. `ctex_resource_ledger_admit_operation_recovery` reserves the
canonical record as pinned CPU memory and its named checkpoint bytes as pinned
backing storage. A caller must retain that reservation through commit; an
over-budget or quiescing ledger returns no reservation.

`ctex_project_container_assess_operation_replay` inventories every recovered
operation record against the same supported-version catalogue. Its report names
each unavailable edit and whether a usable raster checkpoint remains, so an
unknown algorithm is never silently substituted during reopen or recovery.

The generated Python and Rust raw bindings expose every operation-record entry
point and descriptor, while Swift imports the same public C header directly.
`just gate-binding-parity` prevents any of those surfaces from falling behind
the C ABI.
