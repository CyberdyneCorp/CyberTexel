# Host transport revisions

Every enabled texture channel exposes a monotonically increasing content
revision through `TextureChannels::channel_revision`. Each logical tile stores
the channel revision at which that tile most recently changed, readable through
`TextureChannels::tile_revision`. Both calls inspect metadata only: they do not
allocate a sparse tile or read pixel bytes.

CPU writes compare the incoming pixel bytes with the current value. A real byte
change advances the channel sequence once and assigns that value to the affected
tile. Writing the same value is a no-op, so polling hosts do not observe false
changes. Other tiles and channels retain their prior revisions.

Revision zero denotes unchanged initial channel content. Revisions are carried
with a nonzero epoch in `RevisionCursor`. A controlled history reset, or
exhaustion of the 64-bit revision counter, advances the epoch and restarts its
revision values at zero. Tile generations continue across the reset. Exhausting
the epoch or a tile generation refuses the change rather than wrapping.

## Delta queries

`query_channel_delta` accepts the epoch-qualified cursor a host last synchronized
and returns the current cursor plus every tile changed after the caller's value.
Each row-major `TileVersion` carries the tile's latest revision, per-tile
generation and residency. Repeated changes are coalesced: a tile appears once
with its newest version, even when many operations touched it. CPU-authored
channels report `TileResidency::cpu`; the enum reserves `host_device` for
resident resources integrated by later execution work.

The query reads metadata only and never fetches or allocates pixels. A caller
revision newer than the current value in the same epoch is rejected. An unknown
cursor or any cursor from another epoch returns
`DeltaQueryDisposition::full_resynchronization_required` with no partial tile
list. The host must fully synchronize and retain the returned current cursor
before incremental queries resume. Snapshot pinning and readback are later
tasks. The current scan is linear in the logical tile count, with the
change-proportional index scheduled for task 8.8.

## Explicit tile readback

`TileReadback` is a move-only asynchronous operation over an exact list of
`TileVersion` records and caller-owned output spans. CPU-resident tiles stage and
validate all requested bytes, then complete immediately through the same state
model. Host-device requests remain `pending` until the host supplies one
matching `HostTileCompletion` payload per requested tile. A request exposes
`pending`, `complete`, `cancelled`, and `failed` states; output is valid only
when `output_readable()` is true.

The library publishes into caller buffers only after every tile, version,
coordinate, and byte count validates. Stale CPU versions, malformed host
completions, cancellation, failure, and completion arriving after cancellation
leave every output span unchanged. The caller must keep those spans alive until
the operation reaches a terminal state.

Readback is never triggered by revision or delta queries. Task 8.4 uses the
channel's current native format and tightly packed visible tile extent; task 8.5
turns that provisional representation into a declared stable layout. Until
snapshot tokens land in task 8.7, CPU readback requires the current cursor and
fails rather than reading a stale version.
