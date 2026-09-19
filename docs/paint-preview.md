# Paint preview and commit

`<ctex/paint/preview.hpp>` provides an isolated in-flight paint session for an
enabled document channel. `PaintPreviewSession` starts from the channel's exact
`TiledImage`, retaining its object identity and revision cursor. Pixel writes
use the image store's copy-on-write tiles, so provisional painting cannot change
the document's pixels, revision, generation counters, dirty flags, or sparse
allocation state.

The session has four explicit states:

- `provisional`: accepts paint writes and exposes an undilated interactive
  preview;
- `final`: immutable after `finalize`, with UV-border dilation included;
- `committed`: the exact final preview has been published to the document; and
- `cancelled`: the private result has been discarded and cannot commit.

Finalization decodes the channel's 8-bit UNORM, 16-bit UNORM, or 32-bit float
storage into the shared [seam-dilation](paint-seam-dilation.md) operation and
encodes the result back into the same format. Quantized channels round-trip
through their exact integer levels; float values are checked for finite storage.
Invalid coverage or extrapolation is transactional and leaves the session
provisional.

Commit is allowed only while the session is final and the original channel
object and revision cursor are unchanged. A newer document edit therefore
causes a stale-preview refusal instead of being overwritten. Publication first
copies the complete final preview and then uses the image's non-throwing move
assignment, so an allocation failure cannot partially mutate the document. The
committed image and retained final preview share the exact immutable tile
payloads; `PaintPreviewCommitReport` consequently reports a maximum component
error of zero, along with the baseline, preview and committed revisions and the
changed tile set.
