# Tasks: bootstrap-v1-cybertexel

Task IDs are stable capability work packages, not a strict numerical execution
order. The delivery slices below govern scheduling. Each capability's scenarios
become tests alongside its implementation; a task is checked only when its full
scope is complete. First-slice subsets do not complete an entire work package.

## Resume protocol

Durable state is these checkboxes plus one commit per task. To resume after an
interruption:

1. `git pull`, read this file top to bottom.
2. Run `just check-spec` and the recipes for implemented work. Run the full
   verify block as well and record named unimplemented gates separately; those
   gates remain failures and are not a reason to skip checks that can run.
3. Take the next dependency-ready task in the earliest incomplete delivery
   slice. Record partial scope in the slice progress notes without checking an
   unfinished task. Mark hardware-blocked work explicitly; a slice needing that
   measurement cannot be declared complete.
4. Implement, build, test, commit (`feat(<module>): <task>`), push, tick the box.

Verify block. Every routine action goes through `just`; a recipe is the single
definition of its command, and CI invokes the same recipes rather than repeating
them.

```
just build
just test
just examples
just check
```

`just check` runs everything that needs no device: the spec validation, the
capability index, layering, licence, determinism, binding parity, example
coverage, version consistency and the ABI diff. Gates whose implementing task is
not yet done fail and name that task, so the block cannot pass vacuously.
Device-dependent gates — parity and budgets — are `just gate-parity` and
`just gate-budgets`, run where the hardware exists.

## Delivery order

| Slice | Scheduled work | Acceptance |
|---|---|---|
| A — Paint on desktop and mobile | Foundation 1; minimal PNG IO from 2; channels/layers/history from 3; single-set mesh and picking from 4–5; minimal constant/image graph and WGSL/MSL plans from 6; CPU and host execution from 7–8; brush/eraser from 9–10; save/reopen and PNG export from 12; minimal C/Python/Rust/Swift boundaries from 14; fixture runner from 16; devices and numeric budgets from 17; real hosts 18.2; accounting/admission/recovery from 19; recovery records from 20.1–20.2 | Real desktop and mobile hosts paint, erase, undo, save/reopen and export the same fixture. Resident painting has no synchronous pixel readback. Latency, peak memory and CPU/GPU parity are measured on both devices. |
| B — Sustain reliable interaction | Complete 7–9, 17 and 19 for the slice workload; seam/tangent fixtures 9.17 and 11.12; snapshot and device-loss recovery; twenty-minute mobile workload | Sustained latency and memory meet declared ceilings; cancellation, pressure, suspend/resume and device-loss fixtures pass. |
| C — Author useful materials | Complete layer semantics 3, graph core 6, fill/mask tools 10, maps 11, packed export 12 and smart materials 13 | Layered material with generators and anchors adapts to another fixture model and exports correctly. |
| D — Professional workflows and v1 completion | Editable authoring 20; UDIM/atlases 4; remaining tools 10; complete formats 2, all graph nodes and targets 6; full bindings 14, CLI 15, examples 16 and release 18 | Every remaining requirement and task is complete, all applicable gates pass, and device coverage is reported. |

Layered PSD, particles, the full node catalogue, SPIR-V/HLSL and the optional
owned-GPU executor do not block slices A–C. Dependencies within each slice still
apply. Bindings, fixtures and tests grow with each implemented capability; they
are not deferred wholesale to groups 14–16. Unknown reference hardware or numeric
budgets block claiming slice A complete, not building its fixtures.

### Slice progress notes

2026-09-18: Task 1.1 established the strict C++20 static-library target and
native headless, Linux, macOS, Windows, iOS and Android presets. The task runner
and CI now share the same named recipes (1.1a–1.1b). Task 1.2 added the
twelve-module skeleton and an executable dependency/backend-isolation gate. Task
1.3 made `VERSION` authoritative for CMake, the C ABI query, the container writer
constant and all binding manifests. Task 1.4 added the permissive-only dependency
manifest, attribution checks and discovery of vendored/CMake dependencies. Tiled
image storage (1.5) now preserves 8/16/32-bit channel bytes, allocates sparse
tiles and tracks dirty tiles. The 64×64 default remains configurable pending the
task 3.9 history/upload measurement. Colour transforms (1.6) are next.
Task 1.6 defines linear Rec. 709 as the working space and verifies unclamped
sRGB conversion against fixed reference values. Test/sanitizer/determinism
scaffolding (1.7) now runs through CTest/Python and a dedicated ASan+UBSan
preset. The determinism registry fails empty categories until tasks 6.9, 12.1
and 12.10 add real outputs. Task 1.8 covers every color-management scenario,
including semantic defaults, LUT isolation, precision warnings, promoted
accumulation and deterministic dithering. Minimal slice-A PNG IO is next.

2026-09-18: The slice-A subset of tasks 2.1–2.4 and 2.7–2.9 now provides
signature-selected, memory-buffer PNG decode/encode, native 8/16-bit channels,
sRGB/caller/automatic declarations, extension mismatch reporting and limits
checked before pixel allocation. Those full work packages remain unchecked
until their other formats and scenarios land. Channels and texture sets are next.

Task 3.2 is complete: the built-in nine-channel preset is expressed through the
same extensible descriptors as custom semantics, channel precision is independent,
and disabled channels own no `TiledImage`. Texture-set identity and partitioning
(3.1) are next.

The slice-A subset of task 3.1 binds stable partition keys to named UV sets with
independent resolution, bit depth and channel storage. Identity does not depend
on insertion order or display name. Tasks 3.1 and 4.1 are complete: the in-memory
mesh view exposes validated attributes without writable access, requires exactly
one partition assignment per face, supports every specified partition source,
and derives the corresponding texture sets. The next dependency-ready slice-A
work is mesh acceleration for picking (5.1); full named-set rasterization keeps
task 4.2 open until the paint path exists.

Task 5.1 is complete: picking owns a flat, median-split BVH with bounded leaves,
iterative ray-candidate traversal, reuse on an unchanged mesh revision and a
strong rebuild on replacement. `MeshBinding` supplies the revision mechanism
used by every derived-state consumer. Picking rebuilds its spatial index, paint
invalidates its complete surface-map cache and mesh maps retain but report stale
bindings from the same changed revision, completing task 4.6. Task 5.2 is also
complete with documented column-major, clip-depth and top-left viewport
conventions, general matrix inversion, and tested perspective and orthographic world-space rays. Hit
records and exact triangle intersection (5.3) are also complete: the nearest
exact candidate reports every specified field, including independently computed
smooth and geometric normals, named-set UV and UDIM, stable texture-set identity
and per-face material ID. Task 5.4 uses an empty optional as a distinct normal
miss. Ordered nearest/all-hits behavior (5.5) is next.

Tasks 5.5 and 5.6 are complete. Nearest mode retains only the closest exact
intersection; all-hits mode materializes every exact intersection ordered by
world distance and triangle index. Backfaces are accepted by default or rejected
per call using the documented counter-clockwise winding and geometric-normal
rule. UV-space inverse picking (5.7) is next.

Task 5.7 is complete with a flat, named-UV BVH keyed by mesh revision. Exact 2D
barycentrics map a coordinate within the requested texture-set partition back to
surface position, both normals, triangle and the remaining surface metadata;
unowned coordinates are a normal miss. Task 5.8 adds revision-synchronized
point-to-AABB pruning and exact closest-point projection for triangle interiors,
edges, vertices and degenerate triangles. It returns the same complete hit record
and treats an out-of-range surface as a normal miss. Task 5.9 adds accelerated
screen rectangle and lasso queries plus exact world sphere and box queries.
Screen triangles are clip-volume constrained, partial edge or vertex coverage is
inclusive, and all results are sorted by triangle index. Task 5.10 assigns
equivalent-distance shared-edge and shared-vertex nearest hits to the lowest
triangle index, independent of BVH traversal; literal all-hits mode retains both
in index order. Task 5.11 resolves ordered nearest-hit batches with one optional
record per ray, callback-based cancellation and interval progress, and a
preflighted logical-memory ceiling. Cancellation reports processed work but
discards partial hit arrays. Task 5.12 labels and maps all fifteen picking
scenarios to headless tests, including bounded traversal over 2,097,152 indexed
triangles and cancellation of a 5,000-ray batch. The picking work package is
complete. Task 6.1 adds the device-independent material graph document: stable
node identities, ordered socket declarations and stored constants, canonical
links, deep cloning and comparison, and exact versioned text serialization.
The document owns exactly one output node; a `doc` adapter derives its ordered
inputs and defaults from registered channels, including the built-in nine.
Task 6.2 refuses self-links and links that would close a cycle before mutating
the document. Its typed diagnostic reports a deterministic closed node-ID path,
and deserialization independently rejects cyclic input with a linear CSR-backed
topological pass. Task 6.3 defines the exact identity, scalar/vector and
colour/vector coercion matrix with linear Rec. 709 luminance weights. Link edits
return their required emission coercion and any displaced connection; invalid
types and cycles are refused before transactional one-link-per-input replacement.
Deserialization enforces the same invariants. The built-in node catalogue (6.4)
adds all 52 specified input, texture, colour/filter and vector/math types as
versioned immutable declarations with typed sockets, property choices and a
`GraphNode` factory. The Blend declaration carries all twenty shared modes; Mix
Normal Map carries partial-derivative, whiteout and reoriented modes; scalar and
vector math operations have machine-readable and reference-document formulas.
Task 6.5 adds workspace-owned reusable subgraphs with explicit input/output
boundary nodes. Interface versions and sockets propagate transactionally to all
material and nested-group instances, preserving values and links only when name
and type remain compatible and reporting displaced links. Deterministic
dependency-path diagnostics refuse self and transitive recursion before node
creation. Task 6.6 validates graphs without invoking emission and returns
deterministic structured diagnostics for unconnected required inputs, unavailable
image resources and mesh maps, missing group definitions, and unreachable nodes.
Workspace reports retain material/group ownership; reverse-CSR reachability is
linear in nodes plus links. Task 6.7 adds instance-owned registries for versioned
host node declarations with checked CPU and target-specific emission callbacks,
determinism/resource/target metadata, pinned-input replay eligibility, and
reusable parity fixtures. Registry-aware validation names missing versions,
stale interfaces and unsupported targets; unknown node content round-trips
opaquely without callback execution. Task 6.8 vendors and attributes ArmorPaint's
Kongruent-derived minikong compiler, with the active common and WGSL state owned
by isolated contexts. Independent contexts compile concurrently, while wrapper
reuse starts with clean state and remains deterministic. Other retained backend
sources stay unlinked until their context adapters arrive in 6.11. Task 6.9 adds
reachability-driven WGSL expression emission with stable node/socket names,
complete nested-group qualification, code-generation coercions, attribution,
single-emission fan-out, and a byte-comparison determinism fixture. Pass-plan
task 6.10 adds versioned logical textures, mip/layer/tile subresources, explicit
load/store access and transitive hazard dependencies, derived submission
lifetimes, ordered binding and buffer layouts, target state, and validated
draw/dispatch commands without device handles. Task 6.11 links and isolates all
four retained Kong backends, exposes explicit split-text, unified-text, and
binary artifacts, validates SPIR-V externally, and names the available target
set when a request is unsupported. Task 6.12 consumes binding, dimension,
format, float-filtering and compute features while compiling premultiplied
layer stacks for every target. Stacks
that fit remain one render pass; larger stacks split greedily with explicit
intermediate generations and dependencies. Unsupported formats and dimensions
are refused, and unavailable float linear filtering produces a named nearest
workaround. Task 6.13 caches graph expressions and complete layer-stack
emissions by collision-free canonical content, target, normalized features and
versioned host-node semantics. Hits return one immutable stored result without
code generation, failures are not retained, and statistics expose entries,
hits and misses. Task 6.14 starts four different graph emissions together and
requires byte-identical serial results; a second four-thread fixture covers the
complete WGSL, MSL, SPIR-V and HLSL layer-stack shader and pass-plan path.
Task 6.15 emits cached lit previews and unlit inspection shaders for every
supplied channel across all four targets. Pass plans name cube environment
radiance and irradiance encodings, the split-sum BRDF lookup, camera,
environment controls and up to four analytic lights. The documented GGX model
also defines deterministic fallback lighting when environment maps are absent.
Task 6.16 closes both capability suites. Material-graph coverage adds portable
seeded Noise evaluation, one reference formula surface for all twenty Blend
modes, and a canonical device-independent material library whose stable preset
identities resolve after transfer. Complete material entry points emit WGSL,
MSL, validated SPIR-V and HLSL beside explicit stable pass plans and cache the
whole result by graph, target, features and resources. Eleven material-graph and
twenty-two shader-emission labeled CTests map every scenario; focused
determinism and licence tests are part of the shader label. Execution-side
Completion retirement now extends those named scenarios; executor image parity
and reference-host execution follow in tasks 7.5 and 18.2.

Task 7.1 defines an instance-owned executor registry with stable descriptors,
sorted discovery, explicit runtime availability and deterministic automatic,
explicit, pinned and `CTEX_EXECUTOR` selection. Unknown environment values keep
automatic policy and are reported. Structured fallback reports name the failure
and refuse to claim CPU fallback before recovery restoration. Task 7.2 adds the
always-available device-free CPU reference and requires every operation to carry
CPU semantics. Its independent homogeneous rasterizer owns depth, UV, coverage
and triangle buffers for the viewport, while its UV-space raster projects each
covered texel back to camera depth and screen position, including integer UV
tile origins.
Task 7.3 adds the host-executed state machine without exposing device handles or
moving pixels: submissions pin logical generations against an exact base
revision; completions validate every output and publish atomically; stale,
duplicate, failed and cancelled results never advance state. Cancellation keeps
resources alive through late GPU completion. Deterministic record recovery and
checkpoint-only recovery are admitted before publication, while device loss
restores the last commit before an explicit CPU fallback report.
Task 7.4 makes shader-emission features part of every executor descriptor and
rejects incomplete reports at registration. CPU and host routes publish their
own binding, dimension, format, filtering and compute limits; one checked seam
copies the selected executor report into layer-stack, material and preview
requests so the selected path uses one capability record. Task 7.5 declares
normalized integer parity at one code value unfiltered and two when filtered
(`1/255`, `2/255`, `1/65535`, `2/65535`). Floating-point parity uses
absolute-plus-relative bounds of `1e-6 + 1e-5 * magnitude`, widened to
`5e-6 + 5e-5 * magnitude` after filtering. The executable comparator refuses
shape mismatches and non-finite values and reports the first index, measured
deviation and allowed bound.
Task 7.6 commits three document, stroke, camera, material and UV-mesh cases
covering 8-bit, 16-bit, float, filtered, depth and coverage outputs. The CPU
renderer uses the reference rasterizer and shared blend formulas. The generic
gate measures every available executor, fails malformed or missing renderers and
names case, channel and deviation on drift. Compiled routes without a device are
printed as `unmeasured`, never counted as passing; the same `just gate-parity`
command runs in CI.
Task 7.7 adds a staged bounded-work contract to the CPU executor. Operations
declare item count plus shared and per-worker storage before execution. The
executor refuses overflow or an exceeded ceiling before allocating work storage,
uses no more than the requested workers, serializes monotonic interval progress,
and polls cancellation between bounded items. Only successful completion calls
the non-throwing commit boundary; cancelled staging is discarded and the
document remains unchanged.
Task 7.8 adds the first optional owned backend under
`CTEX_ENABLE_VULKAN_EXECUTOR`. Default builds use a no-dependency stub; enabled
builds use pinned Vulkan-Headers and Volk to dynamically discover a loader,
select a physical device deterministically, and own a headless instance, logical
device and graphics-plus-compute queue. Live limits and format support feed the
common executor descriptor. Missing loaders or devices remain compiled but
`device-unavailable`; an enabled CI build requires a Mesa software device. The
task 7.9 labeled suite maps all eighteen `execution-backends` scenarios to the
registry, CPU, host, bounded-work, Vulkan, capability, parity and layering
tests. Its 16K staged fill exercises the execution contract ahead of the paint
tool, and the enabled Vulkan recipe remains the real-device lifecycle check.
Task 8.1 adds one monotonic content sequence to each enabled channel and records
the latest channel sequence on every logical tile. Revision reads are metadata
only, sparse clear tiles remain allocation-free, and byte-identical writes do
not advance either value. Task 8.2 queries from a caller-held revision and
returns the complete row-major union of newer tile versions, coalesced to each
tile's latest revision and generation with explicit CPU residency. It moves no
pixels. Task 8.3 qualifies caller revisions with an epoch; controlled reset or
revision exhaustion advances that epoch while preserving pixels and tile
generations. Unknown or mismatched epochs return a full-resynchronization result
with no partial delta. Task 8.4 adds move-only asynchronous tile readbacks over
exact named versions and caller buffers. CPU-resident requests complete through
the same state model; host-resident requests accept an explicit completion
payload. Pending, cancelled, failed, stale and malformed operations publish no
bytes. Task 8.5 makes the payload contract explicit: visible dimensions, tight
row pitch, pixel stride, interleaved R/RG/RGB/RGBA order, native component type
and byte order, and separate per-tile caller buffers are carried on requests
and host completions. The direct-upload scenario proves the same buffer can
feed a texture upload without repacking. Host-selected format negotiation (8.6)
accepts an ordered host format list and an exact-only or conversion-enabled
policy. It reports the selected source, output and conversion, supports every
8/16-bit UNORM and float component pairing without changing channel count, and
reports invalid declarations or no common format explicitly. Snapshot pinning
(8.7) adds a ceiling-enforced pool, move-only explicitly releasable tokens,
unique physical-allocation accounting and copy-on-write CPU tiles. The
canonical delta query now admits and returns a token; a token-based readback
keeps revision R immutable while an edit publishes R+1 for the following
delta. Task 8.8 replaces the document-grid scan with one latest-change entry per
tile, keyed by revision. Current-cursor queries take an O(1) empty path; changed
queries traverse and radix-order only their coalesced candidates, with
`indexed_tiles_visited` making the scaling invariant directly testable. Task
8.9 adds a separate in-flight `PreviewResource` without a second transport
path: committed channels and previews return the same delta and snapshot types,
use the same layout and format negotiation, and enter the same asynchronous
tile readback. A pinned preview remains immutable across later preview edits,
and no preview write changes committed document pixels. Stable host cache
identities (8.10) add a structured key for each named render or compute pass.
The key combines resource kind, stable plan scope and pass identifier without
delimiter ambiguity, remains stable across plan reconstruction and pass
reordering, and lets a host cache compiled pipelines without pointer or ordinal
keys. Task 8.11 labels the eight transport CTests as one runnable suite and maps
all sixteen scenarios to current fixtures or their explicit later integration
tasks. Rust binding parity, save integration and numeric reference-device
budgets remain explicitly assigned to 14.13, 12.3–12.4 and 17.6 rather than
being simulated by the C++ suite. The host-transport milestone is otherwise
complete; painting begins with canonical input sample reconstruction (9.1).
Task 9.1 defines reconstruction version 1 with nanosecond timestamps, a 1 ms
fixed grid, `1e-6` positional tolerance, redundant time-linear sample removal
and the specified radius/time-constant stabilizer recurrence. Arc-length
spacing emits one canonical stamp sequence with complete resolved properties;
continuous tips add links and a closing endpoint sweep, while discrete-alpha
tips retain separated deposition events without implicit gap coverage.
Task 9.2 adds independently enabled, normalized piecewise-linear response
curves and output ranges for pressure-driven radius, opacity, hardness, flow
and rotation plus tilt-driven rotation and elongation. Missing pressure
evaluates at one, tilt magnitude and azimuth are explicit, pressure and tilt
interpolate with the canonical path, and each resolved radius determines the
following spacing interval. Deterministic jitter, taper and constraints (9.3)
use one documented modifier order. Stateless SplitMix64-derived values key all
five jitter targets by stroke seed, source ordinal and channel. Entry and exit taper
support independent stamp-count or path-distance spans over radius, opacity or
both. Straight-line and deterministic dominant-axis constraints precede the
existing timestamp stabilizer; grid snapping follows it. Task 9.4 expands
symmetry after all modifiers: any subset of object-origin X/Y/Z mirrors forms a
Cartesian product with evenly spaced radial copies around a selected axis.
Every copy transforms the complete coordinate frame, retains its source
ordinal, receives stable instance/final ordinals and has sweep links confined
to its own branch. Task 9.5 adds a separate external resolved-stroke boundary
that validates version, stamp fields, instance/source/final ordinal structure
and continuous or discrete sweep topology, then returns a field-equal copy.
It accepts no resolution settings and invokes no reconstruction, spacing,
mapping, stabilizer, constraint, taper, jitter or symmetry stage. Versioned
stroke presets (9.6) use a canonical exact-bit text format. Schema 2 covers all
current settings; schema 1 omits flow jitter and migrates it to the documented
zero default. Deserialization parses the header first, transactionally validates
the complete result, upgrades older input and names any unsupported newer
version. Texture-space swept coverage, falloff and coordinate modes (9.7) are
implemented independently of camera visibility. The surface raster carries
interpolated position, normal, UV and deterministic triangle ownership;
continuous tips cover branch-local swept segments with interpolated radius and
hardness, while transformed discrete tips do not synthesize gaps. Geometric
falloff uses the specified smoothstep complement, and material sampling exposes
UV, squared-normal triplanar and caller-framed planar coordinates. Explicit
rejection tests (9.8) apply default-on configurable depth and hit-normal angle
tests per coverage contribution plus per-operation geometric-normal backface
handling. Symmetry either supplies a transform-consistent depth context for
every instance or explicitly disables depth for derived instances, with that
choice reported. Precision-aware alpha discard produces the write mask while
retaining sub-threshold accumulation for later stamps. Task 9.9 emits exactly
one accepted coverage event per canonical stamp. Non-building deposition keeps
separate coverage and opacity-times-flow strength maxima; explicit build-up uses
the specified flow recurrence and opacity cap. Transactional contiguous batches
and idempotent ordinal replay make both modes independent of rendering frames
and repeated continuous-segment rasterization. Task 9.10 copies the channel at
stroke start and evaluates every later shade against that immutable snapshot,
using deposition strength and the shared canonical formula for all twenty blend
modes. Invalid updates are transactional, while a new stroke can snapshot the
previous result and accumulate normally. Task 9.11 multiplies every active-layer
mask and the optional colour-ID, geometry or polygon-fill, rectangle or lasso
screen, and UV-island selections into each canonical event before deposition.
Absent masks are identity, any zero excludes the texel, soft weights combine,
and aggregate coverage is rebuilt from the masked events. Revision-keyed UV
coverage, triangle and island caches (9.12) retain immutable per-texture-set
and per-tile bundles across operations. Source triangle indices survive
partition filtering, UV islands require both mesh-position and UV edge
continuity, and deterministic source order assigns their identities. A mesh
revision change clears every entry before lookup, while a UV-set change clears
all tiles for the logical partition; cache statistics make both reuse and
invalidation executable contracts. Deferred stroke-end seam dilation (9.13) is
implemented by a reusable one-to-four-component operation that finds the
nearest covered texel and extrapolates its directional gradient without feeding
generated gutter pixels back into the source. The radius defaults to two and
zero disables the operation; thin regions with no interior gradient use a
reported zero-gradient extrapolation. A stroke coordinator retains the latest
version of every dirtied UV tile across frames, identifies intermediate output
as provisional, and runs one idempotent final pass over all staged tiles.
Preview without commit (9.14) uses a copy-on-write image derived from the
document channel and keeps provisional, final, committed and cancelled states
explicit. Finalization runs seam dilation before freezing the preview. Commit
requires the original channel object and revision, refuses a stale document,
and publishes a pre-copied image whose tile payloads are byte-identical to the
final preview across 8-bit UNORM, 16-bit UNORM and float storage. Bounded paint
work scheduling (9.15) accepts exact half-open stamp footprints, expands them
only by the configured dilation radius, clips to the canvas, and invokes work
once per deduplicated row-major storage tile. Its report distinguishes total
canvas metadata, footprint count, candidate visits and the exact processed tile
set. Tests keep a short 16K stroke at four of 65,536 tiles and plan a one-tile
operation on maximum 32-bit canvas metadata without enumerating the grid. The
combined stroke-model and paint-engine scenario suites (9.16) label every
applicable headless CTest and map every specification scenario to its executable
evidence. A dedicated integration fixture proves that coalesced versus
one-sample input batches produce identical canonical stamps and build-up
deposition, while the same resolved sequence drives coverage, rejection and
deposition. History-dependent assertions are carried forward to task 3.9, and
mip/gutter/tangent assertions were explicitly carried forward to task 9.17.
Seam-aware filtering (9.17) makes blur, smear, derivative and mip-generation
footprints explicit and consumes caller-supplied surface-adjacent taps rather
than UV-neighbor guesses. Tangent-space vectors cross supplied frames through
object space and are renormalized, including mirrored handedness. Island-owned
padding expands the declared footprint per mip, never overwrites valid islands,
leaves contested gutter unassigned and reports affected island identifiers for
every unsupported mip level. Tangent generation remains scheduled in 11.12.
Brush and Eraser (10.1) compose the canonical mask, deposition and blending
stages without re-resolving stroke properties. Brush requires active material
data for every enabled layer channel, ignores material-only disabled channels
and shades all outputs from their stroke-start snapshots. Eraser applies the
same accepted strength proportionally to explicit layer-opacity or mask targets.
Fill (10.2) resolves all six scopes into inspectable normalized masks. Triangle
and island scopes use exact cached identities, UV-tile scope uses integer UV
ownership, selection preserves soft weights, and connected-by-angle performs a
deterministic breadth-first walk over explicit mesh adjacency and unit face
normals. Applying a fill intersects paint masks and rejection acceptance before
the shared enabled-channel shading path.
Clone (10.3) stores its source independently and samples an explicit immutable
source snapshot. Aligned mode applies the source-to-destination UV offset fixed
at stroke start, while fixed mode retains the source anchor for every accepted
destination texel. Both modes compose canonical rejection, masks, deposition
and stroke-start blending. Cross-texture-set requests are refused before
shading with both stable set identities in the diagnostic.
Blur and Smear (10.4) consume immutable stroke-start snapshots and explicit
surface-aware neighborhoods. Blur performs normalized horizontal and vertical
passes at its declared configurable radius. Smear follows supplied upstream
stroke-direction mappings and combines configurable drag strength with canonical
masked deposition. Tangent-space normals cross supplied seam frames and are
renormalized before encoding, while neither tool can feed its own output back
into the active stroke.
Decal and Stencil (10.5) add explicit surface-frame and screen-frame projection
paths. A retained decal owns its stable identities, original surface pick,
editable rotation and scales, revision and pinned material; transform edits do
not repick or rasterize, and rasterization is an explicit call. Stencil position,
rotation, scale and inversion resolve only from screen positions and constrain
the canonical masked deposition path. Project round-trip, tile invalidation and
undo/redo for editable entries remain scheduled together in 20.4.
Projection (10.6) applies pinned material images through explicit camera,
planar or triplanar mappings. Camera projection consumes a column-major current
view-projection matrix plus a required per-texel visibility result and clips to
the camera frustum. Planar projection uses a centered orthonormal frame and
finite extent. Triplanar projection repeats scaled world coordinates and blends
the three planes by squared normalized-surface-normal weights. All variants
compose material opacity, canonical paint masks, optional rejection and the
shared stroke-start enabled-channel shading path.
Text (10.7) accepts a stable host-supplied font containing global metrics and
deterministic per-scalar glyph coverage. It strictly decodes UTF-8, refuses
missing scalars, handles LF/CR/CRLF lines, applies em-relative tracking and
left/centre/right alignment, and combines overlapping glyph coverage without
draw-order dependence. The requested size is surface units per em and expands
the laid-out raster into a decal frame before canonical masks, optional
rejection and stroke-start channel blending. Persistent text resource identity,
save/reopen, invalidation and undo remain scheduled in 20.4.
Particle (10.8) uses a fixed 120 Hz swept collision simulation against the
revision-aware mesh spatial index. Count, lifetime, initial speed, mass, gravity,
friction, restitution and randomness all affect observable contacts, final
states or impulse-based deposit strength. A library-owned SplitMix64 stream
makes direction and speed perturbations exactly replayable from the declared
seed. Ordered contacts map only to matching texture-set, UDIM and triangle
texels, accumulate by the build-up union formula, and compose canonical masks,
optional rejection and stroke-start channel blending. Declared particle,
lifetime and collision limits bound simulation work.
Picker (10.9) resolves an existing surface hit against exactly one matching
texture-set and UDIM view, then reports every enabled channel's semantic,
component count and value in caller-declared order. Views and channel rasters
are validated before sampling, with missing and overlapping matches refused.
Optional per-texel material provenance is returned only when explicitly
supplied, so channel-only picks cannot accidentally select a material.
Colour-ID selection (10.10) compares a caller-picked colour against a validated
map using an explicit linear-RGB Euclidean tolerance and returns an owned binary
selection. Exact zero tolerance is never widened, and the result reports both
its selected texel count and a distinct matched-or-empty status. Named views of
the same selection serve paint restriction, mask-source and visibility-filter
consumers without copying or changing its meaning.
Selection (10.11) uses exact clipped screen rectangle and lasso queries as a
broad phase, then projects cached surface texels so only centers inside the
screen region are selected. Polygon selection reuses the canonical triangle,
UV-island and connected-by-angle expansion rules. Active selection masks feed
paint masking directly; validated stored masks own an independent copy. Screen
operations preserve traversal accounting and refuse stale surface caches.
Task 10.12 is complete. Its shared parameter-validation seam owns documented
defaults and ranges for base stroke spacing, radius, opacity, hardness,
rotation, elongation, flow and stabilization; it clamps finite out-of-range
values and records supplied and resolved values for downstream consumers.
Jitter amounts, taper floor, grid step and bounded radial symmetry use the same
contract. All pressure and tilt response-range endpoints are likewise bounded,
reported and then checked for valid ordering. Conditional taper spans resolve
against their selected unit, including zeroing disabled spans, while fractional
stamp counts remain invalid. Blur radius, smear strength and smear footprint
axes expose bounded resolved values and clamp reports through their tool
results. Fill and polygon selection share a bounded connected-surface angle and
propagate its clamp report. Colour-ID tolerance is bounded to the complete
normalized linear-RGB distance and reports its resolved value. Text tracking
and surface size have bounded values that drive layout and projection, with
combined clamp reporting on text-decal results. Particle count, lifetime,
speed, mass, gravity axes, friction, restitution and randomness return resolved
settings plus a complete clamp report. Decal placement/edit/raster entry points
and stencil mask/application now share bounded position, rotation and positive
scale controls while exposing the resolved transforms and clamp reports.
Planar extent axes plus triplanar scale and offsets are bounded and report every
resolution before sampling. Depth bias, normal-angle rejection and
format-dependent alpha discard now expose bounded resolved values and clamp
reports. Seam-dilation radius uses the same contract at direct, deferred,
preview-finalization and bounded-work entry points. The repository gate keeps
all 69 documented numeric controls tied to implementation identifiers and
explicit output-changing behaviour tests; categorical controls and structural
inputs remain validated by their owning operations.
Task 10.13 remains open because its complete scenario set includes work owned by
later roadmap items: parity through all language bindings in 14.9--14.13,
persistent editable surface paths in 20.4 and integrated symmetry behaviour
across every tool. Existing implemented-tool scenarios remain covered by their
focused tests rather than being treated as evidence for those missing routes.
Mesh-map sets (11.1) define the complete thirteen-kind inventory and bind each
map to one texture-set identity and named UV layout. Bindings validate their
kind-specific channel count without requiring the map resolution to equal the
texture-set resolution. A bind returns the single resolution-mismatch report,
and continuous maps remain readable through normalized bilinear sampling while
material and object identifiers use nearest sampling. Rebinding a kind replaces
its authoritative pixels explicitly.
The optional bake-provider seam (11.2) is a synchronous C-style callback table
with an opaque context, complete capability query and request callback. Requests
carry texture-set and UV identities plus the requested resolution, while a
wrapped control forwards monotonic progress and cancellation without allowing a
provider to publish terminal progress before its output validates. Successful
borrowed image views are bounds-checked, copied and bound transactionally;
cancellation, provider failure, malformed output and invalid callback status
publish nothing. Unsupported requests name the requested map and the provider's
complete advertised set without invoking its request callback. Revisioned
asynchronous publication remains scheduled in 11.13.
Missing-map preflight (11.3) accepts a consumer's complete required-map list and
returns a structured report carrying the consumer, texture-set identity and a
deduplicated stable list of every absent map. The throwing execution guard and
direct reads preserve that report in `MissingMeshMapsError`. They never bind a
placeholder, mutate the map set or return neutral samples; satisfied checks are
diagnostic-free.
Mesh-map staleness tracking (11.4) records the producing `MeshRevision` on every
binding and the current revision on each map set. Revision synchronization,
binding, requirement preflight and sampling all return structured staleness
with both revisions. Stale pixels remain bound and readable so the host can
present them with a warning; only an explicit current-revision replacement
clears the report. Bake requests carry the same revision and tag accepted output
with it.
External map import (11.5) requires concrete channel-meaning and colour-space
declarations, validates the meaning against the requested map, copies bounded
strided caller memory and publishes through the same transactional bind path as
provider output. RGB/RGBA vertex colours are converted from declared sRGB into
the linear working space while alpha and every non-colour data channel remain
numeric. The result reports the declaration, canonical storage space and whether
conversion occurred; malformed metadata or buffers publish nothing.
Normal-map convention handling (11.6) requires every tangent-space,
object-space and bent-normal binding to record OpenGL or DirectX green-channel
orientation while refusing the field on non-normal maps. Provider and external
import paths preserve the declaration. Reads expose the library's canonical
OpenGL encoding by applying `1 - green` to DirectX samples without changing
stored pixels or the other components; tangent-basis compatibility remains in
11.12.
Mesh-map generators (11.7) expose a stable queryable inventory for ambient
occlusion, curvature, thickness, position gradient, world-space direction,
dirt, edge wear and scratches. Each names its complete required-map set before
evaluation. CPU evaluation produces a caller-sized one-channel float mask,
fails with the existing structured missing-map report instead of substituting a
neutral value, and returns staleness for usable outdated inputs. Fixed baseline
formulas define the initial masks; 11.8 layers configurable bounded parameters,
clamp reports and cross-executor parity onto those formulas.
Generator parameter and determinism coverage (11.8) publishes every numeric
parameter's default, finite range and meaning through generator metadata.
Omitted values resolve to defaults; unknown, duplicate, empty and non-finite
inputs are refused; out-of-range values are clamped, used and reported together
with the complete resolved set. Strength and contrast apply to all generators,
with bounded dirt curvature weight, edge threshold, and scratch scale/width.
Repeated CPU evaluation is byte-stable, while an independent portable-host
formula for all eight generators is checked against the CPU result through the
declared filtered floating-point executor parity tolerance.
Mesh-map memory ownership (11.9) tracks each live map set through a generic
texture-set memory account, keeping the document module independent of maps.
Map-set, texture-set and document reports expose resident map pixels separately
from channel pixels and as a combined total. Binding and replacement update the
account transactionally; selective and bulk host release name removed maps and
bytes, cause subsequent consumers to receive the existing missing-map report,
and leave the texture document and its channel pixels intact. Account lifetime
also removes its contribution automatically.
Asynchronous bake publication (11.13) issues owning revision tokens that capture
the issuing session, map, resolution, texture-set and UV identities, mesh revision,
tangent-frame descriptor, bake-settings revision and a monotonically increasing
generation.
Only the latest uncancelled request whose complete identity remains current can
copy and bind provider output; superseded, cancelled, mesh-stale, settings-stale,
altered and duplicate completions publish nothing and return an explicit
disposition. A settings edit snapshots the complete map binding set as one undo
step. Accepted replacements remain grouped with that edit, while undo restores
both the prior settings revision and prior bindings and invalidates every late
request from the undone state.
The project-container foundation (12.1) defines a little-endian framed schema
whose fixed header exposes the library-derived schema version without decoding
the body. Known sections decode independently, while unknown kinds, versions
and tile encodings are reported and retained byte-for-byte for a later re-save.
The first known section stores named sparse tiled images with explicit channel,
clear-pixel, coordinate, edge-extent and compression metadata. Each occupied
tile is independently zlib-compressed; unallocated tiles consume no records,
and snapshot/restore helpers round-trip `TiledImage` pixels and layout. The
project-save determinism registry now compares complete sparse container bytes
across two clean runs.

## 1. Foundation

- [x] 1.1 CMake project, C++20, warnings as errors, presets for headless, macOS, Linux, Windows, iOS, Android
- [x] 1.1a `justfile` as the single task-runner entry point: build, test, format, examples, bench, clean, check, and one recipe per gate; unimplemented gates fail naming their task; prerequisites reported by name
- [x] 1.1b CI invokes the recipes rather than repeating their commands
- [x] 1.2 Module skeleton (`image`, `mesh`, `pick`, `graph`, `emit`, `doc`, `paint`, `maps`, `xport`, `io`, `exec`, `capi`) with the layering gate enforcing the dependency rule
- [x] 1.3 Single source of truth for the version; consumed by build, ABI query and all three binding manifests; version consistency gate
- [x] 1.4 Licence policy, attribution file, dependency audit gate covering vendored trees
- [x] 1.5 `image` module: tiled pixel buffers, formats, 8/16/32-bit channels, tile dirty tracking
- [x] 1.6 Colour transforms and the working space; headless transform tests against reference values
- [x] 1.7 Test harness, sanitizer job, determinism gate scaffolding
- [x] 1.8 `color-management` scenarios as tests

## 2. Image input and output

- [ ] 2.1 Decoders: PNG, JPEG, TGA, BMP, TIFF, OpenEXR, Radiance HDR, PSD
- [ ] 2.2 Content-based format detection and extension-mismatch reporting
- [ ] 2.3 Bit depth and channel preservation; documented expansion rules
- [ ] 2.4 Colour space on read: embedded profiles, caller declaration, the automatic rule
- [ ] 2.5 High dynamic range decoding without clamping
- [ ] 2.6 Layered sources: PSD layers and multi-part EXR, composited or per-layer
- [ ] 2.7 Encoders with per-format options and the impossible-combination refusal
- [ ] 2.8 Decoding from memory buffers
- [ ] 2.9 Untrusted input bounds: dimension validation before allocation, configurable ceiling, named refusals
- [ ] 2.10 Decoder fuzzing gate in CI
- [ ] 2.11 Cancellation, progress and bounded working memory for large decodes
- [ ] 2.12 Documented resampling filters with a stated default
- [ ] 2.13 `image-io` scenarios as tests

## 3. Document

- [x] 3.1 Texture sets: partitioning, per-set resolution and bit depth, stable identity
- [x] 3.2 Semantic channel descriptors, built-in preset, per-channel precision and enablement, and no storage for disabled channels
- [ ] 3.3 Layer stack: entry kinds, nesting rules and their refusals, ordering
- [ ] 3.4 Instances: reference semantics, own modulation, paint refusal, deletion policy, cycle refusal
- [ ] 3.5 Blend modes, with the formula table and a test per mode
- [ ] 3.6 Per-channel participation and effective opacity including group and mask chains
- [ ] 3.7 Compositing on the CPU reference, with the determinism test
- [ ] 3.8 Layer operations: create, duplicate, delete, reorder, reparent, clear, invert, merge, flatten, convert, apply mask — each atomic
- [ ] 3.9 Tile-scoped history, ownership-exchange restore, declared budget and its refusals
- [ ] 3.10 Transactions: grouping and byte-identical cancellation
- [ ] 3.11 `texture-document` scenarios as tests

## 4. Geometry input

- [x] 4.1 Mesh ingest interface, read-only guarantee, attribute description
- [ ] 4.2 Multiple UV sets; texture set binding to a named set
- [ ] 4.3 UDIM tiles: on-demand allocation, addressing, cross-tile writes
- [ ] 4.4 Atlases and their export-time regions
- [ ] 4.5 Overlap and coverage diagnostics
- [x] 4.6 Mesh revision; every derived structure keyed by it
- [ ] 4.7 Mesh replacement: identity matching, UV-change reporting, host-chosen policy
- [ ] 4.8 Declared mesh limits and their named refusals
- [ ] 4.9 `mesh-and-texture-sets` scenarios as tests

## 5. Picking

- [x] 5.1 Spatial acceleration structure, built once, reused, invalidated by mesh revision
- [x] 5.2 Ray construction from screen position for perspective and orthographic projections
- [x] 5.3 Hit record: position, both normals, UV, texture set, UDIM tile, triangle, barycentric, material id, distance
- [x] 5.4 Miss as a distinct outcome
- [x] 5.5 Occlusion policy: nearest hit and all-hits ordered
- [x] 5.6 Backface policy with the documented winding convention
- [x] 5.7 UV-space picking as the inverse of surface picking
- [x] 5.8 Surface snapping within a maximum distance
- [x] 5.9 Region queries: rectangle, lasso, sphere, box
- [x] 5.10 Deterministic resolution at shared edges and vertices
- [x] 5.11 Batched picking with cancellation and progress
- [x] 5.12 `picking` scenarios as tests

## 6. Graph and emission

- [x] 6.1 Graph document: nodes, links, sockets, serialization, comparison
- [x] 6.2 Edit-time cycle detection and its diagnostics
- [x] 6.3 Socket typing, coercion rules, one-link-per-input, refusal of non-coercible links
- [x] 6.4 Node catalogue: input, texture, colour and filter, vector and math
- [x] 6.5 Node groups, socket propagation, recursion refusal
- [x] 6.6 Graph validation independent of emission
- [x] 6.7 Host-registered node types with CPU and emission semantics, replay eligibility and parity fixtures; opaque preservation of unknown types
- [x] 6.8 Vendor Kong under `thirdparty/`, wrap its global state in a context object, attribute it
- [x] 6.9 Emission: result naming, group qualification, single-emission fan-out
- [x] 6.10 Pass plan: logical resource generations, subresource access, dependencies, lifetimes, bindings, layouts, draw/dispatch and state
- [x] 6.11 Target languages WGSL, MSL, SPIR-V, HLSL; unsupported-target refusal
- [x] 6.12 Feature-gated emission and layer-stack pass splitting at the binding budget
- [x] 6.13 Emission cache keyed by graph, target and feature set
- [x] 6.14 Concurrent emission test
- [x] 6.15 Preview shader with declared lighting inputs and a documented shading model; per-channel inspection shaders
- [x] 6.16 `material-graph` and `shader-emission` scenarios as tests

## 7. Execution

- [x] 7.1 Executor interface, enumeration, selection, environment pin, fallback reporting
- [x] 7.2 CPU reference executor: UV-space rasterization, its own depth and UV buffers, every operation
- [x] 7.3 Host-executed route: GPU-resident authority, completion tokens, atomic revision publication, stale-result rejection and recovery before fallback
- [x] 7.4 Device capability reporting feeding emission
- [x] 7.5 Declared parity tolerances per bit depth and for filtered values
- [x] 7.6 Parity fixture corpus and the CI gate, with unmeasured executors reported
- [x] 7.7 Cancellation, progress, worker bound, memory ceiling and its refusals
- [x] 7.8 Optional owned-GPU executor (first backend), behind a build flag
- [x] 7.9 `execution-backends` scenarios as tests

## 8. Host transport

- [x] 8.1 Channel and per-tile revisions, advancing on change
- [x] 8.2 Delta query since a caller-held revision; completeness and coalescing
- [x] 8.3 Stale-revision detection and the full-resynchronization signal
- [x] 8.4 Explicit asynchronous tile readback into caller-owned buffers; no implicit readback on delta queries
- [x] 8.5 Declared, stable memory layout; direct-upload test
- [x] 8.6 Format negotiation and the host-owned conversion decision
- [x] 8.7 Releasable, budgeted snapshot tokens pin resource versions between query and readback
- [x] 8.8 Delta query cost independent of document tile count
- [x] 8.9 Preview transport through the same mechanism
- [x] 8.10 Stable identities for host-cached resources
- [x] 8.11 `host-transport` scenarios as tests

## 9. Painting

- [x] 9.1 Versioned canonical sample reconstruction, timestamp-based stabilization, spacing, continuous sweeps and discrete alpha tips
- [x] 9.2 Pressure and tilt mapping with response curves; no-pressure devices at full pressure
- [x] 9.3 Deterministic jitter, taper, stabilizer, constraints
- [x] 9.4 Symmetry planes and radial symmetry, emitted within one stroke
- [x] 9.5 Externally resolved stamp ingestion
- [x] 9.6 Versioned stroke presets and their refusals
- [x] 9.7 Paint engine: swept coverage, falloff, coordinate modes
- [x] 9.8 Depth, angle and backface rejection; alpha discard
- [x] 9.9 Separate non-building coverage and build-up deposition formulas; batching and frame-rate fixtures
- [x] 9.10 Blending against the stroke-start snapshot, all modes
- [x] 9.11 Masking inputs and their intersection
- [x] 9.12 Cached coverage, triangle identity and UV island maps, keyed by mesh revision
- [x] 9.13 UV seam dilation, extrapolating, deferred to stroke end
- [x] 9.14 Preview without commit, and the preview-equals-commit test
- [x] 9.15 Bounded work reporting
- [x] 9.16 `stroke-model` and `paint-engine` scenarios as tests
- [x] 9.17 Seam adjacency, tangent-aware filters and derivatives, mip/gutter limits, mirrored-UV and minification fixtures

## 10. Tools

- [x] 10.1 Brush and Eraser
- [x] 10.2 Fill: all six scopes
- [x] 10.3 Clone, aligned and fixed, with the cross-set refusal
- [x] 10.4 Blur and Smear over a stroke-start snapshot
- [x] 10.5 Decal and Stencil; persistent editable decals through editable-authoring
- [x] 10.6 Projection, planar and triplanar
- [x] 10.7 Text with UTF-8 and supplied fonts
- [x] 10.8 Particle with deterministic seeding
- [x] 10.9 Picker across every enabled channel
- [x] 10.10 Colour ID selection with tolerance and its empty-selection reporting
- [x] 10.11 Selection tool: rectangle, lasso, polygon fill; storable as a mask
- [x] 10.12 Parameter validation at every entry point; the no-inert-parameter audit
- [ ] 10.13 `paint-tools` scenarios as tests

## 11. Mesh maps

- [x] 11.1 Map set definition, per-set binding, resolution mismatch reporting
- [x] 11.2 Bake provider interface: capability query, request, progress, cancellation
- [x] 11.3 Missing-map reporting with no neutral substitution
- [x] 11.4 Staleness tracking against the mesh revision
- [x] 11.5 External map import with declared channel meaning and colour space
- [x] 11.6 Normal map convention recording and conversion on read
- [x] 11.7 Generators: AO, curvature, thickness, position gradient, direction, dirt, edge wear, scratches
- [x] 11.8 Generator parameter validation and cross-executor determinism
- [x] 11.9 Map memory accounting and host-driven release
- [x] 11.10 CyberRemesherAndUV provider binding, as an example rather than a dependency
- [x] 11.11 `mesh-maps` scenarios as tests
- [x] 11.12 Tangent-frame descriptors, supplied/generated tangent policy, normal-map basis validation and mirrored handedness tests
- [x] 11.13 Asynchronous bake revision tokens, stale-result rejection and coordinated settings/map undo tests

## 12. Input and output

- [x] 12.1 Container format: schema, versioning, backward-open reading, tile storage
- [x] 12.2 Referenced and packed resources; missing-resource reporting
- [x] 12.3 Atomic save; deterministic writing
- [x] 12.4 Autosave, recovery enumeration, non-blocking snapshot
- [x] 12.5 Standalone asset import and export, self-contained packaging
- [ ] 12.6 Untrusted input bounds and the container fuzzing gate
- [ ] 12.7 Export presets: token vocabulary, derived tokens, built-in preset set
- [ ] 12.8 Export formats, bit depths, and the refusal of impossible combinations
- [ ] 12.9 Export scopes, layer scopes, filename pattern and collision refusal
- [ ] 12.10 Padding, export resolution, dry run, machine-readable report, in-memory export
- [ ] 12.11 `project-io` and `texture-export` scenarios as tests

## 13. Smart materials

- [ ] 13.1 Smart material serialization: stack fragment plus exposed parameters
- [ ] 13.2 Derived versus model-specific content and its reporting
- [ ] 13.3 Exposed parameter binding across many entries
- [ ] 13.4 Smart masks with independent instances
- [ ] 13.5 Anchor points, ordering rule, cycle refusal, dependency-ordered evaluation
- [ ] 13.6 Portable resource resolution and self-contained packaging
- [ ] 13.7 Shelf and library enumeration with metadata and thumbnails
- [ ] 13.8 Versioned presets and their refusals
- [ ] 13.9 One-step application and origin recording
- [ ] 13.10 `smart-materials` scenarios as tests

## 14. Bindings

- [ ] 14.1 C ABI: prefix, export map, opaque handles, result codes, diagnostics
- [ ] 14.2 Caller-owned buffers with two-call sizing
- [ ] 14.3 Versioned descriptors and the implausible-size refusal
- [ ] 14.4 ABI version query, stability rules, symbol and descriptor diff gate
- [ ] 14.5 Threading contract documentation and the two-document concurrency test
- [ ] 14.6 Host log sink and the English-plus-codes diagnostic rule
- [ ] 14.7 Host allocator callbacks
- [ ] 14.8 Full-surface coverage gate
- [ ] 14.9 Python binding, numpy-native, typed exceptions, wheel packaging
- [ ] 14.10 Swift package with a system target, idiomatic layer, automatic handle lifetime
- [ ] 14.11 Rust `-sys` and safe crates, typed errors, `Send`/`Sync` matching the contract
- [ ] 14.12 Host-executed route and host transport reachable from all three bindings
- [ ] 14.13 Binding parity gate
- [ ] 14.14 `c-abi` and `language-bindings` scenarios as tests

## 15. Command line

- [ ] 15.1 Binary skeleton, subcommand dispatch, argument validation before any work
- [ ] 15.2 `export`, `bake-request`, `apply`, `run`, `info`, `validate`
- [ ] 15.3 Distinct exit codes per outcome class
- [ ] 15.4 Machine-readable reports; quiet mode; diagnostics on the error stream
- [ ] 15.5 Executor selection by flag and environment; fallback reporting
- [ ] 15.6 Budget flags and their refusals
- [ ] 15.7 Interrupt handling with no partial files
- [ ] 15.8 Help completeness gate
- [ ] 15.9 Determinism test across repeated runs
- [ ] 15.10 CLI smoke tests on every desktop platform in CI
- [ ] 15.11 `cli-headless` scenarios as tests

## 16. Examples

- [ ] 16.1 Fixture assets: meshes with UVs and UDIM layouts, a mesh map set, alphas, images, fonts — with provenance recorded
- [ ] 16.2 Runner (`examples/run_all.py`) with assert, compare and update modes
- [ ] 16.3 One numbered example per capability, each asserting its result
- [ ] 16.4 Committed outputs and the CI comparison at stated tolerances
- [ ] 16.5 Example coverage gate over capabilities
- [ ] 16.6 Determinism: explicit seeds, identical output across runs
- [ ] 16.7 End-to-end example: mesh and maps through layers, graph, smart material, export
- [ ] 16.8 Host-executed route example with a software stand-in
- [ ] 16.9 Examples runnable against any executor as a parity check
- [ ] 16.10 Published gallery in the documentation
- [ ] 16.11 `examples` scenarios as tests

## 17. Performance gate

- [ ] 17.1 Declare the reference devices with their full configuration
- [ ] 17.2 Budget table as a repository document updated by the gate
- [ ] 17.3 Budgets for stamp, stroke, composite, emission, generator, delta query, tile readback, smart material, export
- [ ] 17.4 Memory budgets alongside time budgets
- [ ] 17.5 Scaling verification: a stamp costs what it touches, not what the canvas holds
- [ ] 17.6 Absolute-floor comparison; unreachable budgets reported rather than passed
- [ ] 17.7 Batch attribution so a batch cannot stand in for the operation it names
- [ ] 17.8 Coverage counted over what the gate decides
- [ ] 17.9 Unmeasured cases reported, never substituted
- [ ] 17.10 Regression detection against the recorded baseline
- [ ] 17.11 `device-gate` scenarios as tests
- [ ] 17.12 End-to-end input-to-visible median/p95/p99 budgets and pipeline-stage measurements on both reference hosts
- [ ] 17.13 Twenty-minute mobile benchmark, final-five-minute budgets and pressure/suspend/device-loss fixtures
- [ ] 17.14 Transfer-byte and synchronous-wait instrumentation; ordinary resident paint/undo has zero synchronous pixel readbacks

## 18. Release

- [ ] 18.1 Platform packages with header, library, licence, attribution; smoke test each
- [ ] 18.2 Desktop WGSL and mobile MSL reference hosts in slice A, built in CI, run on named devices with residency-traffic and input-to-visible instrumentation
- [ ] 18.3 Reproducible build verification and documentation of any unavoidable variance
- [ ] 18.4 `build-packaging` scenarios as tests
- [ ] 18.5 After all delivery slices and groups 1–20 pass, archive this change and fold its requirements into `openspec/specs/`

## 19. Resource residency

- [ ] 19.1 Complete allocation accounting with shared physical allocation identity, host device descriptors and pinned/in-flight resource reporting
- [ ] 19.2 CPU/GPU/backing/temporary ceilings, bounded admission and tiled work scheduling
- [ ] 19.3 Sparse constant tiles, derived-cache eviction, lossless authored-tile backing storage and reload
- [ ] 19.4 Host preview-quality policy with unchanged authored precision and export results
- [ ] 19.5 Quiesce, durable checkpoint notification, suspension deadlines and recovery revision reporting
- [ ] 19.6 `resource-residency` scenarios as tests

## 20. Editable authoring

- [ ] 20.1 Versioned operation records, pinned input assets and checkpoint storage; expose through C ABI and bindings and round-trip in project-io
- [ ] 20.2 Same-resolution recovery versus resolution-independent replay eligibility; clone/blur/smear source snapshots and checkpoint-only policy
- [ ] 20.3 Atomic undoable resize with explicit replay/resample/cancel policy and mixed-layer fixtures
- [ ] 20.4 Persistent editable decals, text and surface paths; parameter editing, invalidation, save/reopen and undo
- [ ] 20.5 Reprojection preflight, distance/angle/visibility limits, ambiguity and hole policy, tangent conversion and cancellation
- [ ] 20.6 `editable-authoring` scenarios as tests
