# Batched picking

`ctex::pick::pick_nearest_batch` resolves many world-space rays in one call and
returns one `std::optional<HitRecord>` per input ray in the same order. A miss is
an empty entry; the complete batch is therefore directly aligned with stroke or
selection samples. The shared `SpatialIndex` is synchronized and reused by all
rays, and no GPU is required.

## Completion and cancellation

`BatchPickResult::status` distinguishes `complete`, `cancelled`, and
`memory_ceiling_exceeded`. The hit array is populated only for `complete`; a
cancelled batch reports how many rays it processed but returns no partial array
that a caller could mistake for a complete stroke.

Cancellation is polled before every ray. The optional progress callback receives
an initial zero, every configured non-zero `progress_interval`, and the final
count. Callbacks are non-owning `noexcept` function pointers with caller data,
which avoids allocation and maps directly to the future C boundary.

## Memory bound

Before allocating the result or traversing a ray, the batch computes
`required_memory_bytes`. This conservative logical heap-payload bound includes:

- one optional hit slot and a worst-case texture-set identifier per ray;
- the maximum candidate capacity for one BVH traversal; and
- transient exact-hit and record materialization storage.

Every mesh partition must have exactly one valid texture-set binding so that the
worst-case record size is known before work starts. If the bound exceeds
`memory_ceiling_bytes`, the call returns `memory_ceiling_exceeded` with zero
processed rays. Allocator bookkeeping outside container payloads is not included.
