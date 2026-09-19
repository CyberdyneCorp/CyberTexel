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

The canonical `query_channel_delta` accepts a `SnapshotPool` and the
epoch-qualified cursor a host last synchronized. An admitted query returns the
current cursor, every tile changed after the caller's value, and a move-only
`SnapshotToken`. `query_channel_delta_metadata` is the tokenless metadata helper
used by internal validation and does not authorize later readback.
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
before incremental queries resume.

Each image maintains a revision-ordered index containing one entry per tile at
that tile's latest change. Rewriting a tile rekeys its existing entry instead of
adding history, and starting a new revision epoch clears the index. A delta
query uses the first indexed revision newer than the caller's cursor, visits
only that suffix, and row-major sorts only the returned coordinates. Querying
the current cursor takes the constant-time empty fast path. Consequently query
work is `O(1)` for an unchanged current cursor and `O(log I + K)` for `K`
returned tiles among `I` tiles changed in the current epoch. A fixed-pass radix
ordering of the 64-bit `(y, x)` key preserves row-major results without adding a
document-sized scan or comparison sort.

`ChannelDelta::indexed_tiles_visited` exposes the number of candidate index
entries traversed. It is zero for the unchanged 16K fixture and equals the
coalesced result count for changed queries, making the scaling invariant a
deterministic test rather than a wall-clock threshold.

## Explicit tile readback

`TileReadback` is a move-only asynchronous operation over an exact list of
`TileVersion` records and caller-owned output spans. CPU-resident tiles stage and
validate all requested bytes, then complete immediately through the same state
model. Host-device requests remain `pending` until the host supplies one
matching `HostTileCompletion` payload per requested tile. A request exposes
`pending`, `complete`, `cancelled`, and `failed` states; output is valid only
when `output_readable()` is true.

The library publishes into caller buffers only after every tile, version,
coordinate, declared layout, and byte count validates. Stale CPU versions,
malformed host completions, cancellation, failure, and completion arriving
after cancellation leave every output span unchanged. The caller must keep
those spans alive until the operation reaches a terminal state.

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
channel's current native format. The current-cursor overload remains available
for immediate CPU work; synchronization uses the snapshot overload so a later
edit cannot invalidate the requested version.

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

## Snapshot consistency and pressure

`SnapshotPool` has an immutable pinned-byte ceiling. An admitted delta query
returns a `SnapshotDelta` containing its metadata and an explicitly releasable
`SnapshotToken`. The token lists the exact versions it retains, reports its
retained bytes, and remains valid until `release()`; destruction is a fail-safe
release. `SnapshotMemoryReport` exposes the ceiling, current pinned bytes,
active token count, and unique pinned allocation count.

CPU tiles use shared immutable allocation handles. The query retains those
handles without reading or copying pixels. A later write to a pinned tile uses
copy-on-write, so token-based readback still observes the exact bytes at the
query cursor while the edit appears in the following delta. Readback refuses a
released token or a tile version the token does not contain.

The pool charges full physical tile allocations and counts the same allocation
only once when several tokens retain it. Admission computes the incremental
physical bytes before changing pool state. If that increment would cross the
ceiling, the query returns `over_budget`, no token, and leaves the memory report
unchanged. Releasing the last reference to an old version immediately returns
its CPU allocation to the available snapshot budget; host-device completion
will provide the corresponding reclamation fence when host-resident delta
versions are integrated.
