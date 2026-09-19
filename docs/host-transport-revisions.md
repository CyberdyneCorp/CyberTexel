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
