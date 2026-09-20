# C ABI capability coverage

[`abi/capi-capabilities.json`](../abi/capi-capabilities.json) is the coverage
inventory for the public C boundary. It contains every capability directory in
the active OpenSpec change and classifies it as:

- `runtime`, whose every OpenSpec requirement must name one or more declared
  `ctex_*` entry points;
- `boundary`, which follows the same rule but may use repository evidence for a
  requirement that describes the boundary itself; or
- `non-runtime`, which must state why it has no library operation and name the
  repository gate or delivery task that proves it.

`just gate-c-api-coverage` compares the manifest to the live OpenSpec requirement
titles and `include/ctex/capi.h`. It rejects missing or invented capabilities,
missing or invented requirements, unknown symbols, unmapped public symbols,
duplicate mappings, and unexplained non-runtime exclusions. A single token entry
cannot make a broad capability pass: coverage is checked requirement by
requirement.

The gate is intentionally red while task 14.8 is in progress. The current C ABI
fully maps its boundary requirements and records the implemented document and
texture-set, colour-management, read-only mesh-ingest, bounded in-memory image
I/O, canonical stroke reconstruction, paint-engine primitives and complete CPU
picking operations. Texture export is reachable through data-driven presets,
planning, sampling and caller-owned output callbacks. Project containers now
expose bounded canonical open/re-save, version probing, opaque-content
preservation and atomic publication. Project I/O additionally exposes
standalone asset export and installation, including self-contained packing and
caller-provided resource search paths. Mesh handles own allocator-routed copies
of host buffers, expose named UV sets and revision changes, and enforce the
declared 100,000,000-vertex and 100,000,000-triangle ceilings before reading
array contents. Picking indexes preserve allocator provenance, rebuild after
mesh replacement, report traversal cost, and expose ray, UV, snap, region and
bounded cancellable batch queries without a GPU. Smart-material validation now
exposes canonical migration, mixed-content inventories, typed parameter
fan-out, anchor editing, cycle refusal, dependency-ordered evaluation, portable
resource resolution and self-contained packaging. Material and mask fragments
can now be applied transactionally, inspected and edited as ordinary entries,
then removed as one undo step. Named shelves enumerate all seven preset kinds
with metadata and thumbnails, and stable identities resolve to standalone
packages. Channel revision cursors, per-tile versions, coalesced delta queries,
stale-epoch resynchronization and indexed-query cost are now public. Budgeted
snapshot pools pin queried tile versions across later edits, report their memory
pressure, negotiate host-selected formats, describe direct-upload layouts and
read only named tiles into caller-owned buffers. In-flight paint previews use
the same versioned snapshot path and remain pinned independently of their
session. Executor registries now expose the CPU reference, an attached host route and the
optional compiled owned-GPU route with complete device feature descriptors.
Selection supports explicit, pinned and `CTEX_EXECUTOR` defaults, while fallback
reports refuse CPU fallback until recovery is established. Host-execution
sessions now expose logical resource handoff, output validation, completion and
cancellation, recovery-gated publication and device-loss restoration without
crossing device handles or pixels. Completion and recovery reports preserve
logical resource identity through atomic caller-owned readback. Bounded CPU work
now exposes staged parallel execution, serialized progress, cooperative
cancellation, real worker limits and pre-allocation memory refusal through C
callbacks. Immutable result handles preserve one-shot execution while
supporting caller-owned message readback. Numeric executor parity tolerances and
complete-array comparison are now public for direct and filtered 8-bit, 16-bit
and floating-point values. The complete fixture gate now accepts registry-backed
executor render callbacks, compares every available route to the CPU reference,
and exposes immutable JSON reports with per-case drift and explicit unmeasured
devices. Image channel expansion now preserves 8-bit, 16-bit and floating-point
components while applying the documented grayscale/RGB/alpha mappings. Import
resampling now exposes selectable nearest and default pixel-centred bilinear
filters with bounded caller-owned output and recorded filter metadata. Brush
and Eraser now consume the same canonical deposition through atomic,
caller-owned multi-channel outputs. Fill now resolves all six scopes from public
surface-map data, intersects masks and rejection, and shades caller-owned
channels atomically. Clone now maps aligned and fixed immutable source snapshots,
publishes resolved sample indices and refuses cross-set operations. Blur and
Smear now filter immutable stroke-start snapshots through explicit surface-aware
neighborhoods and mappings. Stencil now resolves bounded screen-anchored,
transformable and invertible masks that compose with canonical deposition and
Brush shading. Decal now rasterizes caller-retained placements and pinned materials
through resolved surface frames with atomic sample, strength and channel output.
Projection now exposes camera-visible, finite planar and repeating triplanar
material application with weighted samples and atomic channel output. Text now
accepts supplied glyph coverage and length-delimited UTF-8 with per-string size,
tracking and alignment before atomic decal publication. Exactly 98
runtime requirements remain unmapped.
The gate is not part of the aggregate `just check` until that count reaches zero;
its unit tests run in the normal tooling suite throughout the migration.
