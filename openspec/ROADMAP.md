# Roadmap

The authority on what is done is `openspec/changes/bootstrap-v1-cybertexel/tasks.md`
and its checkboxes. This file is the milestone view and the running log of
decisions taken and questions still open.

## Status

Implementation started. 24 capabilities, 332 requirements, 409 scenarios and
222 tasks, 166 done. Foundation and the complete headless color-management
scenario suite are green. Image input now detects and decodes PNG, JPEG, TGA,
BMP, baseline TIFF, flat OpenEXR, Radiance HDR and flattened PSD from caller
memory, with mismatch reporting, 8/16-bit preservation and hostile-input
ceilings. Extensible channel
descriptors and sparse per-channel enablement are also complete. Document layers
now resolve independent channel participation against texture-set storage and
multiply effective opacity through nested groups and direct/group mask chains.
The device-free CPU compositor now evaluates resolved layer content bottom to
top with isolated/Pass Through groups and byte-identical repeat output.
Transactional layer operations now publish structural and resolved-content
changes only after validation, byte-budget preflight and any required
appearance-preservation comparison.
Texture-set pixel history now declares channel/tile targets before editing,
retains only changed physical tiles under a reported byte ceiling, evicts the
oldest undo steps when required, and performs symmetric undo/redo by exchanging
exact storage owners without pixel copies. Stale targets and steps larger than
the complete ceiling are refused before any restore or edit, respectively.
Texture-set transactions now isolate repeated declared-tile writes and atomic
layer operations from live state, publish them as one mixed undo step, and make
cancel or abandonment an exact discard. Reversible layer-command deltas retain
only changed entries, removals and changed ordering; tile, layer and history
revision checks refuse stale commits or restores before partial publication.
All 38 texture-document scenarios now map to a checked, labeled headless suite.
Layer metadata edits made through a transaction are retained as reversible
commands with zero pixel-history bytes.
Read-only in-memory mesh ingest now
validates attributes and total, non-overlapping face partitions, from which the
document derives stable UV-bound texture sets with independent storage.
Texture sets can now opt into sparse UDIM storage with standard checked
addressing, UV-addressed cross-border writes, on-demand channel allocation and
separate logical-occupancy and physical-memory reporting.
Documents can group texture sets into validated, non-overlapping atlas regions;
the export catalogue and planner retain and deterministically scale those
regions while producing one output per atlas.
Picking now reuses a flat CPU BVH and rebuilds it when its mesh revision changes.
Perspective and orthographic screen positions also produce documented
world-space rays without a GPU. Exact nearest intersections now return complete
tool-facing hit records, while background rays return a distinct normal miss.
Picking now also supports default nearest and distance-ordered all-hits
occlusion, with documented per-call backface rejection.
Named-UV BVHs now provide the inverse 2D-to-surface picking path without a
linear face scan and rebuild on mesh revision changes.
Surface snapping now uses the same revision-keyed world-space BVH to find the
closest triangle point within a caller-supplied distance, including edge,
vertex, and degenerate-triangle cases, and returns the complete hit record.
Region selection now covers screen rectangles and lassos plus world spheres and
axis-aligned boxes, with BVH pruning, exact triangle tests, documented inclusive
partial coverage, deterministic ordering, and no GPU dependency.
Nearest ray hits on shared edges and vertices now use a documented lowest-index
ownership rule with regression coverage; literal all-hits queries continue to
report each adjacent triangle in deterministic order.
Batched nearest picking now preserves sample order, reports interval progress,
polls cancellation between rays, discards partial results on cancellation, and
preflights a declared logical-memory ceiling before allocating output.
All fifteen picking scenarios now map to one labeled headless test suite. Its
acceleration fixture contains 2,097,152 indexed triangles and verifies that a
query reaches one bounded leaf rather than scanning the mesh.
The material graph now has a device-independent document with stable node IDs,
ordered sockets and links, cloning and structural comparison, and canonical
versioned serialization. Its single output is derived from registered document
channels, including all nine metallic/roughness defaults.
Links that would introduce a directed cycle are now refused atomically with a
typed, deterministic node-path diagnostic. Cyclic serialized graphs are also
rejected by a linear topological validation pass.
Socket links now use an explicit coercion matrix with linear Rec. 709 luminance
weights. Non-coercible links name both types, and connecting to an occupied input
transactionally replaces and reports the prior link without edit-time conversion.
The built-in graph catalogue now declares all 52 specified input, texture,
colour/filter and vector/math nodes with stable schemas. Blend and normal modes
are complete, and scalar/vector math operations carry documented formulas.
Reusable node groups now own editable subgraphs with explicit boundaries.
Interface edits propagate atomically across materials and nested groups while
preserving compatible values and links; recursive placement names and refuses
the complete cycle before mutation.
Material graphs and workspaces now validate independently of emission, returning
owned structured diagnostics for required inputs, missing resources/groups and
unreachable nodes through a linear reverse-reachability pass.
Hosts can register versioned material node declarations in isolated registries
with checked CPU and per-target emission callbacks. Registrations declare
determinism, resource dependencies, targets and parity fixtures; unknown types
round-trip opaquely while registry-aware validation marks them non-emittable.
ArmorPaint's Kongruent-derived minikong compiler is now pinned and attributed.
Its active parser, IR, token-cache, built-in-type and WGSL backend state is owned
by isolated contexts, allowing deterministic wrapper reuse and concurrent
compilation across independent contexts.
Reachability-driven WGSL expression emission now gives every intermediate a
stable node/socket-derived name, qualifies it by every enclosing group instance,
inserts declared socket coercions at use sites, and memoizes fan-out so a node is
emitted once. A byte-comparison fixture now covers shader-emission determinism.
Device-independent pass plans now name versioned logical textures and tile-aware
subresources, reject uninitialized access and unordered hazards, derive
submission lifetimes, and carry the complete binding, layout, render-state, and
draw/dispatch data a host needs without exposing a device handle.
Kong target selection now emits split WGSL/HLSL text, a unified MSL module, or
binary SPIR-V from the same headless API. Each backend owns per-context state,
cross-target compilation is concurrent and deterministic, unknown targets name
the complete available set, and SPIR-V output passes the external validator.
Feature-gated layer-stack emission now consumes the device binding, dimension,
format, filtering, and compute declaration. It emits premultiplied source-over
shaders for every target, keeps fitting stacks in one pass, deterministically
splits larger stacks with carried intermediate generations, and reports a
nearest-filter workaround when float linear filtering is unavailable.
Graph and layer-stack emission results now use collision-free canonical cache
keys covering content, target, host-node semantics, and every normalized device
feature. Hits return one immutable stored result without running code generation;
failed emission is not retained, and cache accounting is observable.
Texture export now plans and encodes deterministic multi-output manifests with
independent bilinear resolution, shared extrapolating UV padding, dry-run and
JSON reports, progress and cancellation, and self-describing in-memory buffers.
All thirty-three project-I/O and texture-export scenarios now have a checked
evidence matrix and named scenario suites. Smart materials now have canonical
ordered stack-fragment serialization with embedded graphs and typed, ranged,
display-grouped parameters. Derived entries retain definitions without cached
raster output, while model-specific painted layers and masks preserve validated
pixels and appear in an inspectable content report. Exposed parameter binding
now targets stable graph inputs and properties across many entries, with typed,
ranged, atomic updates and inert-target refusal. Derived-only smart masks now
instantiate on layers or groups with deep-copied graphs, origin metadata and
independent typed parameter state. Layers and masks can now expose their
composited output as graph-input anchors. References are persisted canonically,
must point upward, reject inert targets and cycles with typed diagnostics, and
produce a dependency-ordered plan containing only affected consumers. Smart
material images, fonts and other resources now use validated stable identities,
ordered shelf search paths and per-input missing status without substitution.
Canonical manifests can embed every dependency for shelf-independent sharing.
A unified library now validates named shelves and enumerates all seven preset
kinds with stable identity, kind, version, display name, sorted tags and an
embedded tiled thumbnail. Global identities resolve unambiguously to standalone
packages. Smart material schemas 1–5 now migrate through documented defaults,
including explicit read-only preservation of pre-binding parameters and inferred
image resources. All seven shelf kinds reject named future versions before
resolution. Smart materials and masks now instantiate as independent,
parameterised texture-set fragments with remapped stable entry identities,
per-entry preset origin, inspectable mixed-content reports, and one history step
per application regardless of fragment size. The complete smart-materials
scenario matrix now maps all fifteen requirements to labeled executable evidence
and fails when the spec or evidence drifts. The C ABI now has a strict-C header,
an opaque document handle, stable result categories, exception containment,
per-thread diagnostics, and a prefix-only shared-library export surface on every
desktop platform. Bulk C results now use caller-owned buffers with null-buffer
sizing, exact required sizes and atomic too-small refusal; ordered texture-set
identities provide executable coverage. Size-first C descriptors now read only
fully covered fields, default appended texture-set precision for older callers,
and refuse truncated or future-sized layouts before document mutation. ABI
versioning now has a pre-handle query tied to `VERSION`, documented same-major
stability rules, and a CI gate over exported symbols, declarations, descriptors,
enumerations, and the platform export list. The per-entry-point threading
contract now permits concurrent calls on distinct documents, requires external
serialization on one document, and keeps diagnostics thread-local. A synchronized
two-document fixture exercises both state and diagnostic isolation. A
silent-by-default process-wide host sink now routes categorized logs above its
configured severity threshold, while every C failure exposes both English prose
and an append-only machine-readable code. Host allocator callbacks validate
callback pairs, preserve allocator provenance per opaque document, and report
failure or misalignment by stable code. A core PMR boundary routes every
persistent allocation reachable through the current C surface, including the
document index, texture-set identity and descriptor fields, shared state,
channel metadata, mesh bindings and picking indexes. Full-surface coverage
(14.8) has a requirement-granular manifest and tested checker over all 24
capabilities. Implemented slices now cover texture-set channels, headless colour
management, read-only revisioned meshes, partition-derived texture sets,
bounded image decode/encode, canonical strokes, paint-engine primitives and
copy-on-write paint previews. The latest slice exposes the complete CPU picking
family: allocator-owned acceleration structures that rebuild after mesh
replacement, perspective and orthographic rays, full hit records, nearest and
all-hit occlusion, backface policy, UV picking, surface snapping, four region
queries, and bounded cancellable batches with progress and traversal cost. The
texture-export slice now exposes data-driven and built-in presets, the complete
token vocabulary, all spatial and layer scopes, filename and resolution
planning, padding, dry runs, JSON reports, progress, cancellation, registered
channels and caller-owned in-memory output delivery. The gate correctly remains
red. The project-container slice adds fixed-header schema probing,
deterministic empty-container creation, bounded canonical open/re-save with a
machine-readable inventory, opaque future-content preservation and atomic
filesystem publication. Standalone asset export and installation select exact
dependencies, optionally pack external resources, and resolve referenced
resources through ordered caller search paths. All byte and report outputs
retain the caller-owned two-call contract. The smart-material authoring slice
adds canonical validation and migration, mixed-content reporting, typed
parameter fan-out, anchor editing with cycle refusal, and dependency-ordered
evaluation planning. Portable resource manifests now support ordered search-path
resolution, explicit missing-input reports and self-contained packaging.
Material and mask fragments can now be instantiated transactionally, inspected
and edited as ordinary entries with retained origin metadata, and removed as a
single undo step. Named shelves enumerate every preset kind with metadata and
thumbnails, while stable preset identities resolve to standalone packages.
Host-transport revision cursors and per-tile versions are now queryable through
coalesced indexed deltas, with explicit stale-epoch resynchronization and
history reset. Budgeted snapshot pools now pin exact delta versions, expose
memory pressure, negotiate readback formats and publish stable direct-upload
layouts before copying named CPU tiles into caller-owned buffers. In-flight
paint previews now use that same versioned snapshot path. Executor discovery,
device capabilities, selection and recovery-aware fallback reporting are also
public. Host-executed submissions, logical resource ownership, validated
completion, cancellation, recovery-gated publication and device-loss reporting
are now public without exposing device handles. Staged CPU execution now exposes
cooperative cancellation and progress, worker limits and pre-allocation memory
ceilings through the C boundary. Numeric parity tolerances and detailed array
comparison are also public. Registry-backed parity fixture runs now compare
available routes to the CPU reference and report per-case drift or unavailable
devices through immutable caller-readable results. Image decoding and explicit
destination-channel expansion now preserve component precision across the C
boundary. Bounded image resampling now exposes selectable nearest and default
pixel-centred bilinear filters while recording the resolved choice. Brush and
Eraser are now public through the canonical deposited-stroke pipeline. Fill now
resolves all six scopes from cached surface-map data and shades every enabled
channel atomically. Clone now exposes aligned and fixed immutable snapshot
mapping with explicit cross-set refusal. Blur and Smear now expose immutable
stroke-start filtering through explicit surface-aware neighborhoods and mappings.
Stencil now resolves screen-anchored transformable masks for the canonical
deposition pipeline. Decal now explicitly rasterizes retained placements and pinned materials through
resolved surface frames. Projection now applies pinned materials through camera,
planar and triplanar mappings with explicit weighted samples. Text now exposes
supplied fonts, length-delimited UTF-8, alignment, tracking and projected decal
output. Particle now exposes deterministic seeded mesh collisions, ordered
contacts and mapped paint output. Picker now exposes ordered enabled-channel
sampling and optional material provenance for one matching texture view.
Colour-ID selection now exposes explicit tolerance, clamp and matched-or-empty
reporting beside its reusable selection mask. Selection now exposes clipped
screen regions and all polygon expansion modes as caller-owned masks. The full
tool inventory and all 69 audited numeric controls now expose discoverable
contextual defaults, bounds and shared clamp results. The complete built-in
material-node catalogue now exposes all 52 schemas, property choices and
documented math formulas, while preset inspection exposes schema migration and
forward-version refusal. Canonical material graph documents now expose the
nine-channel output, typed node edits, complete comparison, link coercion and
replacement, atomic cycle refusal and pre-emission validation. Reusable group
workspaces now expose editable material and group subgraphs, interface
propagation with compatible stored-value preservation, and pre-mutation direct
or transitive recursion refusal with complete cycle diagnostics. Host node
registries now expose versioned declarations, CPU and emission callbacks,
determinism, dependencies, supported targets and executable parity fixtures.
Registered nodes participate in documents, groups and validation; unknown nodes
round-trip opaquely and are named as non-emittable. Canonical material libraries
now save named graphs with stable identities and thumbnail resources, order
presets independently of insertion and resolve the same graph after transfer.
All material-graph requirements are now mapped through C. Material shader
emission now crosses the boundary for all four targets with declared feature
sets, deterministic artifacts, stable bindings and complete logical-resource
pass plans. Concurrent calls over distinct graphs and targets match their serial
results. Layer stacks now cross the boundary as one-pass or deterministic split
compositing with packed per-pass artifacts and carried intermediates. A shared,
observable cache serves byte-identical material and layer-stack results keyed by
content, target and device features. Lit previews now cross C with complete
environment texture contracts, uniform layouts, analytic lights and deterministic
fallback lighting; every channel has an unlit inspection route. Inspectable
material emission now exposes stable reused variable names and structured
attribution through C, including qualified workspace group paths and binary
companion metadata. The vendored Kongruent backend identity, licence and pinned
revisions are also queryable. Shader emission is fully mapped. Independent CPU viewport and
UV-space rasterization now crosses C with bounded atomic output, making the
reference implementation directly usable without host-rendered depth or UV
buffers. Alongside the public parity gate and enforced backend layering,
execution-backends is fully mapped. Pinned committed and preview snapshots now
start opaque asynchronous CPU or host-device readbacks through C. Pending host
requests retain their snapshot, validate the whole completion batch before
publishing caller buffers, and expose complete, cancelled and failed terminal
states. Public mesh-map sets now expose external import, per-texture-set
identity, missing and stale input reporting, supplied/generated tangent frames,
normal conventions, sampling, memory accounting and host-driven release. The
same boundary now exposes deterministic generator inventory and evaluation,
synchronous host bake-provider callbacks, and versioned asynchronous completion
tokens with atomic settings/map undo. Mesh-maps is fully mapped. Exactly 41
runtime requirements remained without C ABI evidence at that point. Periodic
autosave sessions now expose revision coalescing, worker status, explicit flush,
recovery discovery and bounded recovery reads through C. Exactly 40 runtime
requirements remained without C ABI evidence at that point. Partition-scoped
UV overlap diagnostics now expose sorted affected faces and deterministic work
counts through C. Exactly 39 runtime requirements remained without C ABI
evidence at that point. Resolution-aware unit-square diagnostics now expose
covered and uncovered texels, normalized uncovered area, bounded work and
out-of-range faces. Exactly 38 runtime requirements remain without C ABI
evidence at that point. Painted-document mesh replacement now exposes stable
per-set matching, UV-change reports, host policy selection, stale-plan refusal
and atomic keep/clear publication while deferring requested reprojection without
mutation. The complete decoded-format set is now mapped to its public memory
decode operation. Exactly 36 runtime requirements remain without C ABI evidence;
those gaps must be implemented before task 14.8 can close.
Sparse UDIM storage now crosses C through backward-compatible texture-set
descriptors, logical occupancy declaration and enumeration, atomic absolute-UV
write batches and caller-owned pixel readback. Exactly 35 runtime requirements
remain without C ABI evidence.
Atlas creation and inspection now cross C with validated existing-set
membership, non-overlapping regions and caller-owned identity buffers. Exactly
34 runtime requirements remain without C ABI evidence.
Owned resolved-raster snapshots and all twelve atomic layer-operation kinds now
cross C with non-mutating sizing, bounded candidate validation, appearance
checks and joint stack/snapshot publication. Together with the intervening C
ABI slices, exactly 22 runtime requirements remain without C ABI evidence.
Allocator-owned texture-set transactions now stage declared pixel writes,
layer operations and metadata/graph edits behind one stale-checked commit.
Layer-only history retains zero pixel bytes, reducing the exact C ABI gap to 21
runtime requirements.
Aggregate and per-texture-set document memory reporting now includes channel,
history and bound-map residency plus a deterministic pre-save size estimate.
The exact C ABI gap is now 20 runtime requirements.
Every supported flat image decoder now runs in the sanitizer-backed,
deterministic CI fuzz campaign, with Docker providing the identical Linux gate
on macOS. Decoder dependency revisions and licence texts remain enforced by the
licence audit. The exact C ABI gap is now 18 runtime requirements.
Unified resource accounting now reports every required ownership category and
residency role through stable physical identities, including shared CPU/GPU,
pinned and in-flight allocations plus API-independent host device descriptors.
The exact C ABI gap is now 17 runtime requirements.
Budget admission now reserves fixed and tiled working sets against independent
CPU, GPU, backing-store and temporary ceilings, with deterministic eligible
cache eviction and atomic refusal. The exact C ABI gap is now 16 runtime
requirements.
Sparse authored tiles now require successful lossless host backing before
resident release, reload by exact generation for flat and UDIM storage, and
refuse pinned or failed writes. Cache admission invokes physical-release
callbacks before accounting removal. The exact C ABI gap is now 15 runtime
requirements.
Host-ordered preview-quality admission now selects and reports explicit
resolution/resource/deferred-work choices, falling back to cache release and
then atomic over-budget refusal without access to authored or export state. The
exact C ABI gap is now 14 runtime requirements.
Suspension now closes resource admission before cancellation, reports drain and
checkpoint completion against one host deadline, embeds the durable revision in
the atomic recovery container, and reopens admission only on explicit resume.
The exact C ABI gap is now 13 runtime requirements.
The complete resource-residency scenario matrix now maps all five scenarios to
the C++ and strict-C accounting, backing, readback-cancellation and recovery
fixtures. A checked `resource-residency-scenario` label and task-runner recipe
fail when the specification, evidence names or suite membership drift.
The first language binding now packages the C ABI as a platform Python wheel
for Python 3.10 onward. Its idiomatic layer moves decoded images, channel
snapshots, meshes, mesh maps and generated masks through NumPy arrays, retains
native diagnostic codes in typed exceptions, and checks the bundled ABI at
import. Linux, macOS and Windows CI build and smoke-test isolated wheels at the
minimum and current supported Python versions.
The SwiftPM package now resolves the installed C ABI through a system-library
target and relocatable pkg-config metadata. Its macOS 13+/iOS 16+ value layer
checks ABI compatibility, throws typed diagnostic-bearing errors and releases a
shared document handle exactly once when its last Swift value leaves scope.
CI runs the macOS XCTest suite and cross-builds both native and Swift layers for
iOS arm64 through a final executable link.
The Rust workspace now separates native-building raw declarations into
`cybertexel-sys` from an idiomatic safe `cybertexel` crate. Safe documents own
their handles, return typed diagnostic-bearing errors and are deliberately
`Send` but not `Sync`; a unit assertion and compile-fail test enforce that
threading contract. An audit confines wrapper `unsafe` to one documented FFI
boundary, and CI builds and tests both crates on Linux, macOS and Windows.
All three bindings now expose the host-executed lifecycle: real shader artifacts
and JSON pass plans, typed logical-resource submission and recoverable
completion, retained host-resident generations, pinned channel snapshots and
explicit pending-to-complete host readback. Each binding test proves that
pending buffers remain unreadable and publish atomically only after the host
returns the exact requested tile payloads.
This maps cross-binding transport reachability in the C coverage inventory and
reduces the remaining task-14.8 gap to 12 runtime requirements.
Existing public project-resource and mesh-replacement operations also prove the
referenced-by-default mesh path, correcting the task-14.8 gap to 11 runtime
requirements.
The public delta-query and explicit-readback operations are also the exact host
transport paths that task 17 will benchmark. Mapping their C surface reduces the
task-14.8 gap to 10 requirements without claiming the deferred numeric device
gate.
Versioned operation records now have a canonical bounded core format and C ABI.
They own pinned input bytes, name raster checkpoint dependencies, and round-trip
through the project container without consulting mutable shelf content. Task
20.1 remains open until all three language bindings expose this surface. The
matching public-surface mapping reduces the exact task-14.8 gap to 9 runtime
requirements.
Replay assessment now consumes explicit supported algorithm-version ranges,
preserves checkpoints for unknown versions, requires pinned source snapshots
for clone/blur/smear replay, and admits retained record/checkpoint bytes through
the resource ledger. This completes task 20.2 and reduces the task-14.8 gap to 8
runtime requirements; atomic resize policy remains task 20.3.
Snapshot capture and periodic autosave now retain canonical operation records,
pinned inputs and their raster checkpoint images through atomic recovery.
Recovered-container assessment reports every unsupported algorithm without
substitution and whether checkpoint fallback survived. Mapping this public path
reduces the task-14.8 gap to 7 runtime requirements.
Persistent editable decals, text and surface paths now retain placement,
material and attachment source data in the texture set. Edits atomically
invalidate the union of dependent tiles, create one undo step and remain
explicitly rasterized. Canonical project assets and strict-C save/reopen tests
complete task 20.4 and reduce the task-14.8 gap to 6 runtime requirements.
Mesh-replacement plans now retain both mesh states through a bounded,
cancellable reprojection preflight. Per-texel and editable-attachment mappings
report holes and ambiguity before explicit policy selection; commit stages every
channel, converts tangent normals, rejects stale source revisions and publishes
the replacement atomically. This completes task 20.5 and reduces the task-14.8
gap to 5 runtime requirements.

## Milestones

Task IDs remain stable; `tasks.md` schedules their subsets in delivery order.
A full work package stays unchecked until all its requirements are met.

| # | Milestone | Delivery slice | Done when |
|---|---|---|---|
| 1 | **Paint on desktop and mobile** | A | Real WGSL desktop and MSL mobile hosts paint/erase a model, undo, save/reopen and export PNG through the public boundary; residency, parity, memory and visible latency are measured |
| 2 | **Sustain reliable interaction** | B | Resource ceilings, seam correctness, snapshot consistency, cancellation, recovery and a twenty-minute mobile workload pass |
| 3 | **Author useful materials** | C | Layers, masks, generators, anchors and packed export produce a reusable material on a second fixture model |
| 4 | **Complete professional workflows** | D | Editable paths/text/decals, replay/resampling, reprojection, UDIM, remaining tools/formats/targets, bindings and release gates pass |

The first milestone validates the founding architecture. The reference hosts,
minimal C ABI and bindings, numeric budgets and fixtures arrive with painting.
No claim about mobile responsiveness or memory efficiency is satisfied by a
headless CPU test or a build-only mobile package.

## Decisions taken

**2026-09-18 — Material authoring is CyberTexel's, not ClayCore's or
CyberRemesherAndUV's.** ClayCore's `decide-surface-colour` record already
concluded it owns colour as a field property and not material authoring, and
CyberRemesherAndUV's founding design lists texturing as a non-goal. Reopening
either was rejected in favour of a third library. Recorded in `proposal.md`.

**2026-09-18 — The host owns the GPU device.** ClaySpaceDesktop renders with
`wgpu`; a library owning a Metal or Vulkan device would fight it for the same
textures across two APIs. CyberTexel emits shader source and a pass plan
instead. ClayCore's `docs/06-host-gpu-previews.md` set the precedent. Design
decision 1.

**2026-09-19 — Vulkan is the first optional owned-GPU backend.** It is disabled
by default and owns a headless instance, device and queue only for consumers
without a host renderer. Pinned Vulkan-Headers and Volk keep the build auditable;
software Vulkan exercises the enabled configuration in CI. Design decision 1,
task 7.8.

**2026-09-18 — Per-tile revisions are mandatory, not an optimization.**
Revisions identify logical resource versions. CPU-authored edits upload only
changed tiles; host-executed edits remain resident on the same device. Explicit
asynchronous snapshots serve CPU access, save and export. Design decision 8,
capability `host-transport`.

**2026-09-18 — Undo is tile-scoped with a declared budget.** ArmorPaint's ring
of whole-texture snapshots is elegant and collapses to a single step at 16K,
which it handles by silently clamping the configured step count. We snapshot the
tiles a stroke touched and refuse to exceed a host-declared ceiling by name.
Design decision 4.

**2026-09-20 — The v1 default tile remains 64×64.** A one-tile history step on
a 16384-square channel retains one physical tile and uses the same coordinate
and revision unit already consumed by host delta publication. Custom sizes stay
available, but storage, history and upload use one tile granularity. Tasks 1.5
and 3.9.

**2026-09-18 — Examples are the end-to-end test suite.** Python, asserting
rather than printing, outputs committed and compared in CI. No separate tier of
illustrative scripts. Design decision 9, capability `examples`.

**2026-09-18 — No live link.** Pushing textures into a running Unreal, Unity or
Blender session is a host feature; `texture-export`'s in-memory delivery gives a
host what it needs and the library should not own a socket. Permanent non-goal.

**2026-09-18 — Brush node graphs deferred.** ArmorPaint drives brush parameters
per dab from a node graph; `stroke-model` offers jitter, taper and pressure
response as fixed features instead. The constraint v1 carries is that stamp
resolution stays separable enough to put a graph in front of it later.

**2026-09-18 — Validate desktop and mobile painting before catalogue breadth.**
The review moved real hosts and numeric interaction budgets into slice A,
added resource residency and editable authoring, and clarified GPU completion,
recovery, brush deposition, tangent frames and extensible channels. No code or
performance result is implied by these requirements. Design decisions 8, 10–13.

**2026-09-19 — Binding budgets count all shader resource slots.** Layer-stack
passes reserve one slot for their shared sampler; a continuation also reserves
one for the carried intermediate. Packing is greedy in bottom-to-top order and
therefore emits the fewest sequential passes without exceeding the declared
budget. Task 6.12 and capability `shader-emission`.

**2026-09-19 — Emission cache keys retain canonical bytes.** Full canonical
content avoids treating a truncated digest collision as identity. Device format
lists are normalized as sets, while target, every feature field, and versioned
host-node semantics remain explicit key dimensions. Task 6.13 and capability
`shader-emission`.

**2026-09-19 — Concurrent emission is verified against serial baselines.** Four
distinct graphs start together through one cache, and four layer stacks compile
simultaneously across WGSL, MSL, SPIR-V and HLSL. Exact result comparison covers
graph programs, target shaders and device-independent pass plans without a
timing-dependent assertion. Task 6.14 and capability `shader-emission`.

**2026-09-19 — Preview lighting uses a reproducible metallic/roughness
contract.** The shader uses GGX distribution and masking, Schlick Fresnel,
Lambertian energy partition, split-sum image-based specular and explicit linear
Rec. 709 encodings. Radiance cubes use roughness-indexed mips; irradiance cubes
store the cosine integral before division by pi. Missing environments select a
fixed sky/ground fallback, while channel inspection remains entirely unlit.
Task 6.15 and capability `shader-emission`.

## Open questions

These are unresolved and should be answered by the task that first depends on
them rather than drifting.

1. **Parity tolerances.** `execution-backends` requires them stated per bit depth
   and for filtered values. The numbers do not exist yet; slice-A task 7.5 sets them, and
   setting them too loose makes the gate decorative.
2. **Reference devices.** `device-gate` requires at least one desktop and one
   tablet, named with full configuration. Which machines, and who owns them for
   CI, is not settled. Resolve in slice A through task 17.1 before recording any performance claim.
3. **Additional Kong targets.** Tasks 6.8 and 6.11 moved the common compiler and
   all four target backends to context-owned state without a process-wide lock.
   Their artifact shapes are now explicit: split WGSL/HLSL text, unified MSL,
   and binary SPIR-V.
4. **Instance deletion policy.** `texture-document` allows either refusing the
   deletion of a referenced entry or converting its instances to independent
   copies, and makes it the caller's choice. Whether hosts actually want the
   choice, or whether one behaviour should simply be the rule, is open. Task 3.4.
5. **UV density normalization.** Absolute texels-per-unit or relative to the
   set's mean — CyberRemesherAndUV issue #89 raises the same question on the bake
   side. The two repositories should answer it identically.
6. **Recovery and backing storage.** Choose checkpoint cadence, compression,
   backing-store quotas and the maximum recovery replay time in slice A. Measure
   them alongside painting latency; asynchronous work still consumes bandwidth.
7. **Tangent basis and seam tolerance.** Pin the generation algorithm and version
   with the bake provider and fixture assets before accepting normal-map parity.
8. **Initial mobile matrix.** Select the first physical tablet for slice A and
   the Android device/API coverage required before v1 release. An iPad result
   cannot stand in for Android runtime validation.
9. **Material profiles.** The extensible channel contract is in v1. A complete
    OpenPBR profile is follow-up scope requiring explicit shading/export mappings
    and conformance tests; it is not claimed by adding a coat-weight descriptor.

## Dependencies on CyberRemesherAndUV

Tracked as [issue #86](https://github.com/CyberdyneCorp/CyberRemesherAndUV/issues/86)
and its sub-issues. `mesh-maps` consumes what they produce; none of them is a
build dependency in either direction.

| Their issue | What CyberTexel needs it for |
|---|---|
| #87 object-space normal, bent normal, thickness, position | Generators and smart masks |
| #88 material and object ID | Colour-ID selection, polygon fill |
| #89 world-space direction, UV density | Directional and scale-locked generators |
| #90 bake padding | Maps that do not seam at island borders |
| #91 UDIM-aware baking | Parity with `mesh-and-texture-sets` |
| #92 output above 4096 | Maps that match a 8K or 16K document |
| #93 bake provider interface | The seam itself |

Until #93 lands, `mesh-maps` is exercised with the fixture map set committed for
the examples.
