# Resolution changes

`ctex::doc::change_texture_set_resolution` requires one explicit policy:

- `replay_eligible` consumes assessed operation sources and a complete
  host-evaluated target raster for every enabled base and UDIM channel.
- `resample_all` resamples every current raster in the core with an explicit
  nearest or bilinear filter.
- `cancel` returns without changing document state.

Operation records remain the authority for replay eligibility.
`ctex::io::assess_operation_replay` and `resolution_replay_source` distinguish
resolution-independent replay from checkpoint-only, same-resolution and
unsupported-version fallback. Any source that cannot replay independently must
retain a checkpoint and the request must name its resampling filter. The host
replay raster represents the final evaluation of the mixed layer sequence,
including procedural entries; the core verifies exact channel identity, byte
count and target dimensions before publication.

The working-byte ceiling covers the complete staged output and, for
resample-all, the dense source scratch space. The history ceiling covers the
prior base channels, every allocated UDIM channel and its tile-history state.
Budget refusal, missing or extra output, duplicate source identity and replay
refusal leave dimensions and pixels unchanged.

Successful publication swaps the fully staged storage and starts a fresh tile
history at the existing tile-history budget. Resolution undo restores the old
dimensions, pixels, UDIM storage and tile history as one step; redo restores the
resized state. Retained resize snapshots are included in the texture-set and
document memory reports.

The C boundary exposes the same contract through
`ctex_texture_set_change_resolution`,
`ctex_texture_set_undo_resolution_change` and
`ctex_texture_set_redo_resolution_change`. Replay requests carry canonical
operation-record bytes, the supported algorithm-version catalogue and
caller-owned final rasters. A zero C descriptor budget selects the documented
1 GiB default; C++ callers receive the same default in
`ResolutionChangeRequest`.
