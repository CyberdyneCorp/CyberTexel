# Texture-set transactions

`TextureSet::begin_transaction()` opens an isolated candidate over a named
history step and an optional declared set of channel tiles. The candidate owns
independent image revision state while initially sharing immutable tile
allocations with the live texture set. Its first tile write therefore uses the
normal copy-on-write path without touching live pixels.

`TextureSetTransaction::write_pixel()` accepts only coordinates within the
declared channel/tile set. `apply_layer_operation()` runs the same validated
atomic layer operations against the staged layer stack. The mutable staged
`layer_stack()` supports validated command edits such as names, opacity, blend
mode, channel modulation and fill graphs; pixel-transforming operations still
go through `apply_layer_operation()`.

Commit first verifies that history, every live target tile, and the live layer
revision still match the transaction's opening state. It then publishes all
changed declared tiles by ownership exchange and publishes the staged layer
stack as one history step. Forty writes to one tile retain one pre-transaction
tile owner, and a mixed pixel/layer transaction needs one undo or redo.
Unchanged declared tiles are omitted. A transaction with no effective changes
creates no step.

Layer history is a reversible command delta: it retains changed entry values,
removed identities, and an identity order only when ordering changed. It does
not retain layer raster snapshots or consume the pixel-history byte budget. A
layer revision on the command detects an external structural edit before pixels
are exchanged.

`cancel()` and destruction of an active transaction discard the candidate.
Because live state was never mutated, pixels, tile and layer revisions, layer
structure, and undo/redo counts remain exactly as they were at open. A failed or
stale commit also consumes the transaction and publishes none of its candidate
state.
