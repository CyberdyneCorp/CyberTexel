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
read only named tiles into caller-owned buffers. Opaque asynchronous readbacks
expose pending, complete, cancelled and failed states, validate host completion
batches atomically and retain their pinned snapshot until destruction. In-flight
paint previews use the same versioned snapshot path and remain pinned
independently of their session. Executor registries now expose the CPU reference,
an attached host route and the optional compiled owned-GPU route with complete
device feature descriptors.
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
devices. Independent CPU viewport and UV-space rasterization now crosses C with
bounded atomic depth, coordinate, coverage and triangle output. Together with
the public parity gate and enforced backend layering, execution-backends is
fully mapped. Image channel expansion now preserves 8-bit, 16-bit and
floating-point components while applying the documented grayscale/RGB/alpha
mappings. Import
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
tracking and alignment before atomic decal publication. Particle now exposes
seeded mesh-collision simulation, ordered contacts and mapped atomic paint
output through a reusable pick index. Picker now resolves one matching
texture-set and UDIM view, returning every enabled channel in caller order and
material provenance only when supplied. Colour-ID selection now produces a
caller-owned mask using an explicit linear-RGB tolerance, including distinct
matched and empty outcomes. Selection now exposes clipped rectangle and lasso
queries plus triangle, UV-island and connected-by-angle polygon masks. The 74
contextual catalogue entries now expose all 69 audited numeric paint
parameters, and the shared validator returns the same bounds and clamp outcome
used by tool execution. The complete named tool inventory is now public. The
complete 52-node built-in material catalogue now publishes stable schemas,
property choices and all documented scalar/vector math formulas. Existing
smart-material inspection and shelf resolution also expose the preset migration
and forward-version refusal policy. Canonical graph documents now expose their
nine-channel output, typed edits, comparison, coercing link replacement, atomic
cycle refusal and resource-aware validation independently of emission. Opaque
graph workspaces now own reusable groups with editable boundary subgraphs,
propagate interface changes across material and nested-group instances while
preserving compatible stored values, and refuse direct or transitive recursion
before mutation with the complete cycle diagnostic. Isolated host-node
registries now copy the complete CPU/emission reference contract, execute its
parity fixtures, report replay eligibility, add registered nodes to documents
and groups, and preserve unknown serialized nodes while naming their missing
type during registry-aware validation. Canonical material libraries now save
named graph presets with stable
identities and thumbnail resources, enumerate them in identity order and resolve
the same independent graph from transferred bytes. Material-graph is therefore
fully mapped. Headless material shader emission now returns WGSL, MSL, SPIR-V or
HLSL artifacts beside a deterministic JSON pass plan. The request carries the
device feature set and logical resources, while the plan publishes stable
bindings, generations, subresource access and submission lifetimes without
device handles. Concurrent calls over different graphs and targets match their
serial results. Layer stacks now expose one-pass and binding-budget-split
compositing, packed per-pass artifacts and explicit carried intermediates.
Material and layer-stack results can share an observable cache keyed by content,
target and device features. Lit previews now declare complete environment and
analytic-light inputs, including uniform layouts, while every document channel
has an unlit inspection route and missing environments use defined fallback
lighting. Inspectable material emission now exposes stable reused variables,
qualified group paths and node attribution for text and binary targets, while
the pinned Kongruent backend identity and licence are queryable at runtime.
Shader emission is therefore fully mapped. Public mesh-map sets now bind one
texture set and mesh revision, copy externally produced pixels, expose
resolution mismatch, normal convention and tangent-frame metadata, reject
missing inputs explicitly, report revision staleness, and account and release
resident map bytes. The same boundary now exposes the deterministic generator
catalogue and evaluation results, synchronous host-owned bake-provider callbacks,
and versioned asynchronous request tokens with stale-result rejection and
settings/map undo. Mesh-maps is therefore fully mapped. Exactly 41 runtime
requirements remained unmapped at that point. Periodic autosave sessions,
revision coalescing, status, flush/wait, recovery discovery and bounded recovery
reads now cross C as well. Exactly 40 runtime requirements remain unmapped
overall. Partition-scoped positive-area UV overlap diagnostics now return
affected face indices and inspectable work counts through C, reducing the exact
remaining gap to 39 runtime requirements.
Resolution-aware non-UDIM coverage now reports exact covered and uncovered
texel-centre counts, normalized uncovered area, bounded work and sorted
out-of-range faces. The exact remaining gap is 38 runtime requirements.
Two-phase document-aware mesh replacement now owns and validates the proposed
mesh, reports stable per-texture-set identity and UV-layout changes, requires a
host keep/reproject/clear decision, refuses stale plans, and publishes clear or
keep decisions atomically with the new mesh. Reprojection leaves both source
mesh and pixels intact for the later editable-authoring operation. The exact
remaining gap is 37 runtime requirements.
The complete content-detected image decoder set is exposed by
`ctex_image_decode_memory`, reducing the exact remaining gap to 36 runtime
requirements.
Sparse UDIM texture sets now cross the boundary through an appended,
backward-defaulted descriptor field, logical occupancy declaration and
enumeration, atomic absolute-UV write batches and caller-owned pixel readback.
The exact remaining gap is 35 runtime requirements.
Validated atlas creation, identity enumeration and ordered region inspection
now expose the document's export-time texture-set grouping. The exact remaining
gap is 34 runtime requirements.
Ordered layer-stack authoring and canonical inspection now expose every entry
kind, atomic nesting validation, live instances, authoritative blend evaluation,
per-channel modulation, applicable masks and exact effective opacity. The exact
remaining gap is 28 runtime requirements.
Budgeted tile-history captures now wrap document pixel publication, retain only
changed declared tiles, exchange storage symmetrically for undo and redo, expose
distinct empty/stale results and invalidate redo on a new commit. The exact
remaining gap is 25 runtime requirements.
The device-free reference compositor now consumes explicit content, coverage and
mask rasters and returns caller-owned enabled-channel pixels with deterministic
bottom-to-top evaluation, isolated groups and Pass Through scope. The exact
remaining gap is 23 runtime requirements.
Owned resolved-raster snapshots and atomic layer operations reduce the exact
gap to 22 requirements. Allocator-owned texture-set transactions reduce it to
21, and aggregate document memory plus pre-save sizing reduce it to 20. The
decoder fuzzing and pinned-dependency audit gates now cover the public in-memory
decode boundary, reducing the exact remaining gap to 18 runtime requirements.
Unified resource accounting now exposes stable physical identities, complete
category and residency-role totals, pinned/in-flight state, and
API-independent host device descriptors. The exact remaining gap is 17 runtime
requirements.
Atomic reservations now enforce independent CPU, GPU, backing-store and
temporary ceilings, schedule the largest fitting bounded tile batch, evict only
eligible reconstructible caches, and leave storage unchanged on refusal. The
exact remaining gap is 16 runtime requirements.
Sparse clear tiles remain allocation-free, while authored flat or UDIM channel
tiles cross a host-provided lossless backing boundary before eviction and reload
by exact generation. Physical cache release is callback-confirmed rather than
an accounting-only deletion. The exact remaining gap is 15 runtime
requirements.
Ordered host preview-quality admission now reports full, deferred, reduced and
combined outcomes, releases eligible caches only after no allowed choice fits,
and returns over-budget without mutating authored storage. The exact remaining
gap is 14 runtime requirements.
Host-driven quiesce now closes resource admission, requests cancellation,
drains reservations and publishes a revision-bearing checkpoint within one
lifecycle deadline. Recovery returns only the last atomic checkpoint and its
durable revision while reporting known uncheckpointed revisions. The exact
remaining gap is 13 runtime requirements.
Python, Swift and safe Rust now drive channel revisions, snapshots, explicit
host readback and host-execution completion through the same C operations. The
exact remaining gap is 12 runtime requirements.
The public project-container inventory, referenced/self-contained asset export
and install paths, and mesh-replacement plan already cover referenced-by-default
mesh identity, optional packing and moved-mesh recovery. Correcting that stale
manifest omission reduces the exact remaining gap to 11 runtime requirements.
The delta-query and explicit readback operations measured by the host-transport
budget already cross the public C boundary. Mapping those operations reduces
the surface gap to 10; the numeric reference-device budget and its gate remain
scheduled under performance task 17.
Canonical operation-record creation, inspection, project insertion and retrieval
now cross the C boundary with pinned input bytes and explicit raster checkpoint
dependencies. Mapping that editable-authoring requirement reduces the exact
remaining gap to 9 runtime requirements; binding parity and replay policy remain
separate roadmap work.
Explicit supported-version assessment now distinguishes same-resolution,
resolution-independent, checkpoint-only, resample-required and unknown
algorithm records. Clone, blur and smear require pinned source snapshots for
replay, and recovery records plus checkpoint bytes pass through resource-ledger
admission before commit. This reduces the exact remaining gap to 8 runtime
requirements.
Committed snapshots and periodic autosave now prove that operation records,
pinned inputs and raster checkpoint images survive atomic recovery publication.
The recovered-container assessment names unsupported algorithm versions and
whether checkpoint fallback remains available through C. This reduces the exact
remaining gap to 7 runtime requirements.
Persistent decal, text and surface-path entries now expose atomic editing,
dependent-tile invalidation, explicit rasterization planning, undo/redo and
lossless project save/reopen through C. Surface paths resolve through the
canonical stroke model. Mapping persistent editable placement reduces the exact
remaining gap to 6 runtime requirements; mesh-replacement reprojection keeps the
separate editable-surface-path requirement open.
Mesh replacement now exposes a bounded, cancellable reprojection preflight with
per-texel and editable-attachment JSON evidence, explicit hole and ambiguity
policies, stale-source refusal and atomic replacement publication. Tangent-space
normal samples are converted through the source and destination bases. Mapping
the reprojection requirement reduces the exact remaining gap to 5 runtime
requirements.
Texture-set resolution changes now accept an explicit replay-eligible,
resample-all or cancel decision. Canonical operation records are assessed
against the host version catalogue; mixed checkpoint segments require a named
filter; complete host replay rasters are validated and published under working
and history byte ceilings. Dedicated undo/redo entry points exchange the whole
base/UDIM state atomically. Mapping the resolution-change requirement reduces
the exact remaining gap to 4 runtime requirements.
Editable surface paths now have strict-C evidence for control-point position,
width and material edits, dependent-tile invalidation, stroke-model
re-evaluation and atomic undo. The existing mesh-reprojection preflight and
commit operations cover attachment migration and invalid-attachment reporting.
Mapping that paint-tools requirement reduces the exact remaining gap to 3
runtime requirements.
Bounded image decode now exposes a separate working-memory ceiling, phase and
row progress, and cooperative cancellation without partial caller output. C++
and strict-C fixtures cover a large OpenEXR plus the public callback boundary.
Mapping that image-io requirement reduces the exact remaining gap to 2 runtime
requirements.
Named PSD layers and multipart OpenEXR parts now cross the boundary through
`ctex_image_decode_layered_memory`, with selectable authored-composite or
individual output, aggregate bounds, cancellation, and atomic caller-owned
metadata/name/pixel buffers. Mapping layered sources reduces the exact remaining
gap to 1 runtime requirement: the lossless project container.
The gate is not part of the aggregate `just check` until that count reaches zero;
its unit tests run in the normal tooling suite throughout the migration.
