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
needed by this consumer, but task 4.6 remains open until paint and mesh-map
derived structures also use that revision. Task 5.2 is also complete with
documented column-major, clip-depth and top-left viewport conventions, general
matrix inversion, and tested perspective and orthographic world-space rays. Hit
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
is next.

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
- [ ] 4.6 Mesh revision; every derived structure keyed by it
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
- [ ] 6.4 Node catalogue: input, texture, colour and filter, vector and math
- [ ] 6.5 Node groups, socket propagation, recursion refusal
- [ ] 6.6 Graph validation independent of emission
- [ ] 6.7 Host-registered node types with CPU and emission semantics, replay eligibility and parity fixtures; opaque preservation of unknown types
- [ ] 6.8 Vendor Kong under `thirdparty/`, wrap its global state in a context object, attribute it
- [ ] 6.9 Emission: result naming, group qualification, single-emission fan-out
- [ ] 6.10 Pass plan: logical resource generations, subresource access, dependencies, lifetimes, bindings, layouts, draw/dispatch and state
- [ ] 6.11 Target languages WGSL, MSL, SPIR-V, HLSL; unsupported-target refusal
- [ ] 6.12 Feature-gated emission and layer-stack pass splitting at the binding budget
- [ ] 6.13 Emission cache keyed by graph, target and feature set
- [ ] 6.14 Concurrent emission test
- [ ] 6.15 Preview shader with declared lighting inputs and a documented shading model; per-channel inspection shaders
- [ ] 6.16 `material-graph` and `shader-emission` scenarios as tests

## 7. Execution

- [ ] 7.1 Executor interface, enumeration, selection, environment pin, fallback reporting
- [ ] 7.2 CPU reference executor: UV-space rasterization, its own depth and UV buffers, every operation
- [ ] 7.3 Host-executed route: GPU-resident authority, completion tokens, atomic revision publication, stale-result rejection and recovery before fallback
- [ ] 7.4 Device capability reporting feeding emission
- [ ] 7.5 Declared parity tolerances per bit depth and for filtered values
- [ ] 7.6 Parity fixture corpus and the CI gate, with unmeasured executors reported
- [ ] 7.7 Cancellation, progress, worker bound, memory ceiling and its refusals
- [ ] 7.8 Optional owned-GPU executor (first backend), behind a build flag
- [ ] 7.9 `execution-backends` scenarios as tests

## 8. Host transport

- [ ] 8.1 Channel and per-tile revisions, advancing on change
- [ ] 8.2 Delta query since a caller-held revision; completeness and coalescing
- [ ] 8.3 Stale-revision detection and the full-resynchronization signal
- [ ] 8.4 Explicit asynchronous tile readback into caller-owned buffers; no implicit readback on delta queries
- [ ] 8.5 Declared, stable memory layout; direct-upload test
- [ ] 8.6 Format negotiation and the host-owned conversion decision
- [ ] 8.7 Releasable, budgeted snapshot tokens pin resource versions between query and readback
- [ ] 8.8 Delta query cost independent of document tile count
- [ ] 8.9 Preview transport through the same mechanism
- [ ] 8.10 Stable identities for host-cached resources
- [ ] 8.11 `host-transport` scenarios as tests

## 9. Painting

- [ ] 9.1 Versioned canonical sample reconstruction, timestamp-based stabilization, spacing, continuous sweeps and discrete alpha tips
- [ ] 9.2 Pressure and tilt mapping with response curves; no-pressure devices at full pressure
- [ ] 9.3 Deterministic jitter, taper, stabilizer, constraints
- [ ] 9.4 Symmetry planes and radial symmetry, emitted within one stroke
- [ ] 9.5 Externally resolved stamp ingestion
- [ ] 9.6 Versioned stroke presets and their refusals
- [ ] 9.7 Paint engine: swept coverage, falloff, coordinate modes
- [ ] 9.8 Depth, angle and backface rejection; alpha discard
- [ ] 9.9 Separate non-building coverage and build-up deposition formulas; batching and frame-rate fixtures
- [ ] 9.10 Blending against the stroke-start snapshot, all modes
- [ ] 9.11 Masking inputs and their intersection
- [ ] 9.12 Cached coverage, triangle identity and UV island maps, keyed by mesh revision
- [ ] 9.13 UV seam dilation, extrapolating, deferred to stroke end
- [ ] 9.14 Preview without commit, and the preview-equals-commit test
- [ ] 9.15 Bounded work reporting
- [ ] 9.16 `stroke-model` and `paint-engine` scenarios as tests
- [ ] 9.17 Seam adjacency, tangent-aware filters and derivatives, mip/gutter limits, mirrored-UV and minification fixtures

## 10. Tools

- [ ] 10.1 Brush and Eraser
- [ ] 10.2 Fill: all six scopes
- [ ] 10.3 Clone, aligned and fixed, with the cross-set refusal
- [ ] 10.4 Blur and Smear over a stroke-start snapshot
- [ ] 10.5 Decal and Stencil; persistent editable decals through editable-authoring
- [ ] 10.6 Projection, planar and triplanar
- [ ] 10.7 Text with UTF-8 and supplied fonts
- [ ] 10.8 Particle with deterministic seeding
- [ ] 10.9 Picker across every enabled channel
- [ ] 10.10 Colour ID selection with tolerance and its empty-selection reporting
- [ ] 10.11 Selection tool: rectangle, lasso, polygon fill; storable as a mask
- [ ] 10.12 Parameter validation at every entry point; the no-inert-parameter audit
- [ ] 10.13 `paint-tools` scenarios as tests

## 11. Mesh maps

- [ ] 11.1 Map set definition, per-set binding, resolution mismatch reporting
- [ ] 11.2 Bake provider interface: capability query, request, progress, cancellation
- [ ] 11.3 Missing-map reporting with no neutral substitution
- [ ] 11.4 Staleness tracking against the mesh revision
- [ ] 11.5 External map import with declared channel meaning and colour space
- [ ] 11.6 Normal map convention recording and conversion on read
- [ ] 11.7 Generators: AO, curvature, thickness, position gradient, direction, dirt, edge wear, scratches
- [ ] 11.8 Generator parameter validation and cross-executor determinism
- [ ] 11.9 Map memory accounting and host-driven release
- [ ] 11.10 CyberRemesherAndUV provider binding, as an example rather than a dependency
- [ ] 11.11 `mesh-maps` scenarios as tests
- [ ] 11.12 Tangent-frame descriptors, supplied/generated tangent policy, normal-map basis validation and mirrored handedness tests
- [ ] 11.13 Asynchronous bake revision tokens, stale-result rejection and coordinated settings/map undo tests

## 12. Input and output

- [ ] 12.1 Container format: schema, versioning, backward-open reading, tile storage
- [ ] 12.2 Referenced and packed resources; missing-resource reporting
- [ ] 12.3 Atomic save; deterministic writing
- [ ] 12.4 Autosave, recovery enumeration, non-blocking snapshot
- [ ] 12.5 Standalone asset import and export, self-contained packaging
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
