# Tasks: bootstrap-v1-cybertexel

Ordered by dependency. Groups 1–5 unlock everything else. Each group ends with
its capability's spec scenarios turned into tests; a group is not done until
those tests run in CI.

## Resume protocol

Durable state is these checkboxes plus one commit per task. To resume after an
interruption:

1. `git pull`, read this file top to bottom.
2. Run the verify block below; it must be green before new work starts.
3. Take the first unchecked task in group order. Skip only tasks whose entire
   scope is hardware-blocked, and mark them so.
4. Implement, build, test, commit (`feat(<module>): <task>`), push, tick the box.

Verify block:

```
cmake --preset headless && cmake --build --preset headless
ctest --preset headless
python -m pytest python/tests
python examples/run_all.py --check
cargo test --manifest-path rust/Cargo.toml
just gate-layering gate-licence gate-parity gate-determinism gate-budgets
```

## 1. Foundation

- [ ] 1.1 CMake project, C++20, warnings as errors, presets for headless, macOS, Linux, Windows, iOS, Android
- [ ] 1.2 Module skeleton (`image`, `mesh`, `pick`, `graph`, `emit`, `doc`, `paint`, `maps`, `xport`, `io`, `exec`, `capi`) with the layering gate enforcing the dependency rule
- [ ] 1.3 Single source of truth for the version; consumed by build, ABI query and all three binding manifests; version consistency gate
- [ ] 1.4 Licence policy, attribution file, dependency audit gate covering vendored trees
- [ ] 1.5 `image` module: tiled pixel buffers, formats, 8/16/32-bit channels, tile dirty tracking
- [ ] 1.6 Colour transforms and the working space; headless transform tests against reference values
- [ ] 1.7 Test harness, sanitizer job, determinism gate scaffolding
- [ ] 1.8 `color-management` scenarios as tests

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

- [ ] 3.1 Texture sets: partitioning, per-set resolution and bit depth, stable identity
- [ ] 3.2 Channel set with per-set enablement and no storage for disabled channels
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

- [ ] 4.1 Mesh ingest interface, read-only guarantee, attribute description
- [ ] 4.2 Multiple UV sets; texture set binding to a named set
- [ ] 4.3 UDIM tiles: on-demand allocation, addressing, cross-tile writes
- [ ] 4.4 Atlases and their export-time regions
- [ ] 4.5 Overlap and coverage diagnostics
- [ ] 4.6 Mesh revision; every derived structure keyed by it
- [ ] 4.7 Mesh replacement: identity matching, UV-change reporting, host-chosen policy
- [ ] 4.8 Declared mesh limits and their named refusals
- [ ] 4.9 `mesh-and-texture-sets` scenarios as tests

## 5. Picking

- [ ] 5.1 Spatial acceleration structure, built once, reused, invalidated by mesh revision
- [ ] 5.2 Ray construction from screen position for perspective and orthographic projections
- [ ] 5.3 Hit record: position, both normals, UV, texture set, UDIM tile, triangle, barycentric, material id, distance
- [ ] 5.4 Miss as a distinct outcome
- [ ] 5.5 Occlusion policy: nearest hit and all-hits ordered
- [ ] 5.6 Backface policy with the documented winding convention
- [ ] 5.7 UV-space picking as the inverse of surface picking
- [ ] 5.8 Surface snapping within a maximum distance
- [ ] 5.9 Region queries: rectangle, lasso, sphere, box
- [ ] 5.10 Deterministic resolution at shared edges and vertices
- [ ] 5.11 Batched picking with cancellation and progress
- [ ] 5.12 `picking` scenarios as tests

## 6. Graph and emission

- [ ] 6.1 Graph document: nodes, links, sockets, serialization, comparison
- [ ] 6.2 Edit-time cycle detection and its diagnostics
- [ ] 6.3 Socket typing, coercion rules, one-link-per-input, refusal of non-coercible links
- [ ] 6.4 Node catalogue: input, texture, colour and filter, vector and math
- [ ] 6.5 Node groups, socket propagation, recursion refusal
- [ ] 6.6 Graph validation independent of emission
- [ ] 6.7 Host-registered node types; opaque preservation of unknown types
- [ ] 6.8 Vendor Kong under `thirdparty/`, wrap its global state in a context object, attribute it
- [ ] 6.9 Emission: result naming, group qualification, single-emission fan-out
- [ ] 6.10 Pass plan: resources, bindings, layouts, draw, state — complete and ordered
- [ ] 6.11 Target languages WGSL, MSL, SPIR-V, HLSL; unsupported-target refusal
- [ ] 6.12 Feature-gated emission and layer-stack pass splitting at the binding budget
- [ ] 6.13 Emission cache keyed by graph, target and feature set
- [ ] 6.14 Concurrent emission test
- [ ] 6.15 Preview shader with declared lighting inputs and a documented shading model; per-channel inspection shaders
- [ ] 6.16 `material-graph` and `shader-emission` scenarios as tests

## 7. Execution

- [ ] 7.1 Executor interface, enumeration, selection, environment pin, fallback reporting
- [ ] 7.2 CPU reference executor: UV-space rasterization, its own depth and UV buffers, every operation
- [ ] 7.3 Host-executed route: the contract, ownership declarations, result validation
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
- [ ] 8.4 Tile readback into caller-owned buffers
- [ ] 8.5 Declared, stable memory layout; direct-upload test
- [ ] 8.6 Format negotiation and the host-owned conversion decision
- [ ] 8.7 Snapshot consistency between query and readback
- [ ] 8.8 Delta query cost independent of document tile count
- [ ] 8.9 Preview transport through the same mechanism
- [ ] 8.10 Stable identities for host-cached resources
- [ ] 8.11 `host-transport` scenarios as tests

## 9. Painting

- [ ] 9.1 Stroke model: samples to stamps, spacing, swept segments
- [ ] 9.2 Pressure and tilt mapping with response curves; no-pressure devices at full pressure
- [ ] 9.3 Deterministic jitter, taper, stabilizer, constraints
- [ ] 9.4 Symmetry planes and radial symmetry, emitted within one stroke
- [ ] 9.5 Externally resolved stamp ingestion
- [ ] 9.6 Versioned stroke presets and their refusals
- [ ] 9.7 Paint engine: swept coverage, falloff, coordinate modes
- [ ] 9.8 Depth, angle and backface rejection; alpha discard
- [ ] 9.9 Per-stroke coverage accumulation; flow separate from opacity
- [ ] 9.10 Blending against the stroke-start snapshot, all modes
- [ ] 9.11 Masking inputs and their intersection
- [ ] 9.12 Cached coverage, triangle identity and UV island maps, keyed by mesh revision
- [ ] 9.13 UV seam dilation, extrapolating, deferred to stroke end
- [ ] 9.14 Preview without commit, and the preview-equals-commit test
- [ ] 9.15 Bounded work reporting
- [ ] 9.16 `stroke-model` and `paint-engine` scenarios as tests

## 10. Tools

- [ ] 10.1 Brush and Eraser
- [ ] 10.2 Fill: all six scopes
- [ ] 10.3 Clone, aligned and fixed, with the cross-set refusal
- [ ] 10.4 Blur and Smear over a stroke-start snapshot
- [ ] 10.5 Decal and Stencil, editable until commit
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

## 18. Release

- [ ] 18.1 Platform packages with header, library, licence, attribution; smoke test each
- [ ] 18.2 Reference host exercising the host-executed route and host transport on a real device API, built in CI
- [ ] 18.3 Reproducible build verification and documentation of any unavoidable variance
- [ ] 18.4 `build-packaging` scenarios as tests
- [ ] 18.5 Archive this change; fold its requirements into `openspec/specs/`
