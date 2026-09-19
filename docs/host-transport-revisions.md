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
coordinate, declared layout, and byte count validates. Stale CPU versions, malformed host
completions, cancellation, failure, and completion arriving after cancellation
leave every output span unchanged. The caller must keep those spans alive until
the operation reaches a terminal state.

## Stable tile memory layout

`tile_memory_layout` declares the exact payload before a caller allocates its
destination. `TileMemoryLayout` contains the visible tile width and height, byte
row pitch, byte pixel stride, component type, component byte order, interleaved
channel order, and batch contiguity. Readback requests and host completion
records both carry that descriptor and must match exactly.

Payload rows are top-to-bottom and pixels are left-to-right. Components are
interleaved in `R`, `RG`, `RGB`, or `RGBA` order using the channel's native
`uint8_unorm`, `uint16_unorm`, or `float32` representation and native byte
order. Rows have no padding: `row_pitch_bytes` is `width *
pixel_stride_bytes`, and `byte_size()` is `row_pitch_bytes * height`. Edge
tiles contain only their visible extent. A multi-tile request uses one separate
caller buffer per tile; tiles are never implicitly concatenated. This lets a
host pass each completed buffer and its declared row pitch directly to a
texture-upload API without rearranging pixels.

Readback is never triggered by revision or delta queries. Format negotiation
does not run implicitly: without a selection, this contract exposes the
channel's current native format. Until snapshot tokens land in task 8.7, CPU
readback requires the current cursor and fails rather than reading a stale
version.

## Host-controlled format negotiation

`negotiate_readback_format` takes the channel's source `PixelFormat`, an ordered
list of formats accepted by the host, and one of two policies. `exact_only`
refuses any conversion. `allow_conversion` selects the first compatible entry,
so the host's list order is also its preference order. A successful result
reports the source format, selected output format, and named
`ReadbackConversion`; failure distinguishes an invalid declaration from
`no_common_format`.

Negotiation never changes the component count or channel order. It supports all
pairs of the library's `uint8_unorm`, `uint16_unorm`, and `float32` component
types. UNORM values convert through their normalized value. Conversion to UNORM
clamps to `[0, 1]`, maps NaN to zero, and rounds to the nearest integer;
conversion to float produces the normalized floating-point value. Multi-byte
components retain the declared native byte order.

The caller passes the returned `ReadbackFormatSelection` to both
`tile_memory_layout` and the CPU or host-device `TileReadback` factory. The
operation exposes that effective choice through `format_selection()` and
rejects a forged selection, a selection for another channel format, or a
destination layout that differs from the selected output. Thus a conversion can
happen only after the host opts in, and the operation and resulting layout both
report the format that was actually delivered.
