# Tasks: bootstrap-v1-cybertexel

Ordered by dependency. Groups 1–4 unlock everything else. Each group ends with
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
cargo test --manifest-path rust/Cargo.toml
just gate-layering just gate-licence just gate-parity just gate-determinism
```

## 1. Foundation

- [ ] 1.1 CMake project, C++20, warnings as errors, presets for headless, macOS, Linux, Windows, iOS, Android
- [ ] 1.2 Module skeleton (`image`, `graph`, `emit`, `doc`, `paint`, `maps`, `io`, `exec`, `capi`) with the layering gate enforcing the dependency rule
- [ ] 1.3 Single source of truth for the version; consumed by build, ABI query and all three binding manifests; version consistency gate
- [ ] 1.4 Licence policy, attribution file, dependency audit gate covering vendored trees
- [ ] 1.5 `image` module: tiled pixel buffers, formats, 8/16/32-bit channels, tile dirty tracking
- [ ] 1.6 Colour transforms and the working space; headless transform tests against reference values
- [ ] 1.7 Test harness, sanitizer job, determinism gate scaffolding

## 2. Document

- [ ] 2.1 Texture sets: partitioning, per-set resolution and bit depth, stable identity
- [ ] 2.2 Channel set with per-set enablement and no storage for disabled channels
- [ ] 2.3 Layer stack: entry kinds, nesting rules and their refusals, ordering
- [ ] 2.4 Blend modes, with the formula table and a test per mode
- [ ] 2.5 Per-channel participation and effective opacity including group and mask chains
- [ ] 2.6 Compositing on the CPU reference, with the determinism test
- [ ] 2.7 Layer operations: create, duplicate, delete, reorder, reparent, clear, invert, merge, flatten, convert, apply mask — each atomic
- [ ] 2.8 Tile-scoped history, ownership-exchange restore, declared budget and its refusals
- [ ] 2.9 Transactions: grouping and byte-identical cancellation
- [ ] 2.10 `texture-document` scenarios as tests

## 3. Geometry input

- [ ] 3.1 Mesh ingest interface, read-only guarantee, attribute description
- [ ] 3.2 Multiple UV sets; texture set binding to a named set
- [ ] 3.3 UDIM tiles: on-demand allocation, addressing, cross-tile writes
- [ ] 3.4 Atlases and their export-time regions
- [ ] 3.5 Overlap and coverage diagnostics
- [ ] 3.6 Spatial acceleration structure, reuse and invalidation
- [ ] 3.7 Mesh replacement: identity matching, UV-change reporting, host-chosen policy
- [ ] 3.8 Declared mesh limits and their named refusals
- [ ] 3.9 `mesh-and-texture-sets` scenarios as tests

## 4. Graph and emission

- [ ] 4.1 Graph document: nodes, links, sockets, serialization, comparison
- [ ] 4.2 Edit-time cycle detection and its diagnostics
- [ ] 4.3 Socket typing, coercion rules, one-link-per-input, refusal of non-coercible links
- [ ] 4.4 Node catalogue: input, texture, colour and filter, vector and math
- [ ] 4.5 Node groups, socket propagation, recursion refusal
- [ ] 4.6 Graph validation independent of emission
- [ ] 4.7 Host-registered node types; opaque preservation of unknown types
- [ ] 4.8 Vendor Kong under `thirdparty/`, wrap its global state in a context object, attribute it
- [ ] 4.9 Emission: result naming, group qualification, single-emission fan-out
- [ ] 4.10 Pass plan: resources, bindings, layouts, draw, state — complete and ordered
- [ ] 4.11 Target languages WGSL, MSL, SPIR-V, HLSL; unsupported-target refusal
- [ ] 4.12 Feature-gated emission and layer-stack pass splitting at the binding budget
- [ ] 4.13 Emission cache keyed by graph, target and feature set
- [ ] 4.14 Concurrent emission test
- [ ] 4.15 Preview and per-channel inspection shaders
- [ ] 4.16 `material-graph` and `shader-emission` scenarios as tests

## 5. Execution

- [ ] 5.1 Executor interface, enumeration, selection, environment pin, fallback reporting
- [ ] 5.2 CPU reference executor: UV-space rasterization, its own depth and UV buffers, every operation
- [ ] 5.3 Host-executed route: the contract, ownership declarations, result validation
- [ ] 5.4 Device capability reporting feeding emission
- [ ] 5.5 Declared parity tolerances per bit depth and for filtered values
- [ ] 5.6 Parity fixture corpus and the CI gate, with unmeasured executors reported
- [ ] 5.7 Cancellation, progress, worker bound, memory ceiling and its refusals
- [ ] 5.8 Optional owned-GPU executor (first backend), behind a build flag
- [ ] 5.9 `execution-backends` scenarios as tests

## 6. Painting

- [ ] 6.1 Stroke model: samples to stamps, spacing, swept segments
- [ ] 6.2 Pressure and tilt mapping with response curves; no-pressure devices at full pressure
- [ ] 6.3 Deterministic jitter, taper, stabilizer, constraints
- [ ] 6.4 Symmetry planes and radial symmetry, emitted within one stroke
- [ ] 6.5 Externally resolved stamp ingestion
- [ ] 6.6 Versioned stroke presets and their refusals
- [ ] 6.7 Paint engine: swept coverage, falloff, coordinate modes
- [ ] 6.8 Depth, angle and backface rejection; alpha discard
- [ ] 6.9 Per-stroke coverage accumulation; flow separate from opacity
- [ ] 6.10 Blending against the stroke-start snapshot, all modes
- [ ] 6.11 Masking inputs and their intersection
- [ ] 6.12 Cached coverage, triangle identity and UV island maps, with invalidation
- [ ] 6.13 UV seam dilation, extrapolating, deferred to stroke end
- [ ] 6.14 Preview without commit, and the preview-equals-commit test
- [ ] 6.15 Bounded work reporting
- [ ] 6.16 `stroke-model` and `paint-engine` scenarios as tests

## 7. Tools

- [ ] 7.1 Brush and Eraser
- [ ] 7.2 Fill: all six scopes
- [ ] 7.3 Clone, aligned and fixed, with the cross-set refusal
- [ ] 7.4 Blur and Smear over a stroke-start snapshot
- [ ] 7.5 Decal and Stencil, editable until commit
- [ ] 7.6 Projection, planar and triplanar
- [ ] 7.7 Text with UTF-8 and supplied fonts
- [ ] 7.8 Particle with deterministic seeding
- [ ] 7.9 Picker across every enabled channel
- [ ] 7.10 Colour ID selection with tolerance and its empty-selection reporting
- [ ] 7.11 Selection tool: rectangle, lasso, polygon fill; storable as a mask
- [ ] 7.12 Parameter validation at every entry point; the no-inert-parameter audit
- [ ] 7.13 `paint-tools` scenarios as tests

## 8. Mesh maps

- [ ] 8.1 Map set definition, per-set binding, resolution mismatch reporting
- [ ] 8.2 Bake provider interface: capability query, request, progress, cancellation
- [ ] 8.3 Missing-map reporting with no neutral substitution
- [ ] 8.4 Staleness tracking against mesh revision
- [ ] 8.5 External map import with declared channel meaning and colour space
- [ ] 8.6 Normal map convention recording and conversion on read
- [ ] 8.7 Generators: AO, curvature, thickness, position gradient, direction, dirt, edge wear, scratches
- [ ] 8.8 Generator parameter validation and cross-executor determinism
- [ ] 8.9 Map memory accounting and host-driven release
- [ ] 8.10 CyberRemesherAndUV provider binding, as an example rather than a dependency
- [ ] 8.11 `mesh-maps` scenarios as tests

## 9. Input and output

- [ ] 9.1 Container format: schema, versioning, backward-open reading, tile storage
- [ ] 9.2 Referenced and packed resources; missing-resource reporting
- [ ] 9.3 Atomic save; deterministic writing
- [ ] 9.4 Autosave, recovery enumeration, non-blocking snapshot
- [ ] 9.5 Standalone asset import and export, self-contained packaging
- [ ] 9.6 Untrusted input bounds and the fuzzing gate
- [ ] 9.7 Export presets: token vocabulary, derived tokens, built-in preset set
- [ ] 9.8 Export formats, bit depths, and the refusal of impossible combinations
- [ ] 9.9 Export scopes, layer scopes, filename pattern and collision refusal
- [ ] 9.10 Padding, export resolution, dry run, machine-readable report, in-memory export
- [ ] 9.11 `project-io` and `texture-export` scenarios as tests

## 10. Smart materials

- [ ] 10.1 Smart material serialization: stack fragment plus exposed parameters
- [ ] 10.2 Derived versus model-specific content and its reporting
- [ ] 10.3 Exposed parameter binding across many entries
- [ ] 10.4 Smart masks with independent instances
- [ ] 10.5 Anchor points, ordering rule, cycle refusal, dependency-ordered evaluation
- [ ] 10.6 Portable resource resolution and self-contained packaging
- [ ] 10.7 Shelf and library enumeration with metadata and thumbnails
- [ ] 10.8 Versioned presets and their refusals
- [ ] 10.9 One-step application and origin recording
- [ ] 10.10 `smart-materials` scenarios as tests

## 11. Bindings

- [ ] 11.1 C ABI: prefix, export map, opaque handles, result codes, diagnostics
- [ ] 11.2 Caller-owned buffers with two-call sizing
- [ ] 11.3 Versioned descriptors and the implausible-size refusal
- [ ] 11.4 ABI version query, stability rules, symbol and descriptor diff gate
- [ ] 11.5 Threading contract documentation and the two-document concurrency test
- [ ] 11.6 Host allocator callbacks
- [ ] 11.7 Full-surface coverage gate
- [ ] 11.8 Python binding, numpy-native, typed exceptions, wheel packaging
- [ ] 11.9 Swift package with a system target, idiomatic layer, automatic handle lifetime
- [ ] 11.10 Rust `-sys` and safe crates, typed errors, `Send`/`Sync` matching the contract
- [ ] 11.11 Host-executed route reachable from all three bindings
- [ ] 11.12 Binding parity gate
- [ ] 11.13 Examples per binding, run in CI
- [ ] 11.14 `c-abi` and `language-bindings` scenarios as tests

## 12. Release

- [ ] 12.1 Platform packages with header, library, licence, attribution; smoke test each
- [ ] 12.2 Reference host exercising the host-executed route on a real device API, built in CI
- [ ] 12.3 Benchmarks with declared floors on a named reference device
- [ ] 12.4 Reproducible build verification and documentation of any unavoidable variance
- [ ] 12.5 `build-packaging` scenarios as tests
- [ ] 12.6 Archive this change; fold its requirements into `openspec/specs/`
