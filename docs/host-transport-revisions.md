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

Revision zero denotes unchanged initial channel content. Exhausting the 64-bit
sequence refuses the write rather than wrapping to a misleading value.

## Delta queries

`query_channel_delta` accepts the channel revision a host last synchronized and
returns the current revision plus every tile changed after the caller's value.
Each row-major `TileVersion` carries the tile's latest revision, per-tile
generation and residency. Repeated changes are coalesced: a tile appears once
with its newest version, even when many operations touched it. CPU-authored
channels report `TileResidency::cpu`; the enum reserves `host_device` for
resident resources integrated by later execution work.

The query reads metadata only and never fetches or allocates pixels. A caller
revision newer than the channel is rejected. Reset/stale-revision signaling is
task 8.3; snapshot pinning and readback are later tasks. The current scan is
linear in the logical tile count, with the change-proportional index scheduled
for task 8.8.
