# Changelog

All notable changes to this project are documented here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the project follows
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

Foundation implementation is in progress.

### Added

- Founding specification `bootstrap-v1-cybertexel`: 24 capabilities, 332
  requirements, 400 scenarios, 222 tasks.
- `just` as the single task-runner entry point for building, testing,
  formatting and every gate, specified in `build-packaging` rather than left as
  an undocumented convention. CI invokes the same recipes a contributor runs.
- Repository scaffolding: OpenSpec validation and the capability index gate in
  CI, lint configuration, contributor documentation, a roadmap with the
  decisions taken and the questions still open, and the third-party attribution
  table the licence audit checks against.
- Strict C++20 build presets, the enforced module skeleton, and a unified
  library/binding version with a public C ABI query.
- A dependency licence manifest and gate covering vendored trees, CMake-fetched
  sources and CMake packages.
- Sparse tiled pixel storage for one-to-four-channel 8-bit, 16-bit and
  floating-point formats, with tile dirty tracking.
- Monotonic content revisions on every enabled channel and logical tile, with
  metadata-only reads and no revision change for byte-identical writes.
- Complete metadata-only channel delta queries from a caller-held revision,
  coalescing repeated tile changes while retaining latest revision, generation
  and residency records.
- Epoch-qualified revision cursors with explicit full-resynchronization results
  after reset, preserving pixels and per-tile generations without returning a
  misleading partial delta.
- Move-only asynchronous tile readback operations for CPU- and host-resident
  versions, publishing exact named-tile payloads into caller buffers only after
  successful validation and completion.
- Stable tile payload descriptors declaring visible dimensions, row pitch,
  interleaved channel order, component representation and separate per-tile
  buffers, with direct texture upload requiring no intermediate repack.
- Linear Rec. 709 working-space and sRGB transfer functions with unclamped HDR
  conversion.
- Native and sanitizer test presets plus a registry-driven determinism gate that
  refuses empty output categories.
- An always-available CPU reference executor with a universal CPU operation
  contract, independently clipped and depth-tested viewport depth/UV buffers,
  and UV-space texel rasterization carrying projected depth and screen position.
- A host-executed submission and completion protocol with logical resource
  ownership/state, in-flight generation retention, output validation, atomic
  revision publication, stale and cancelled result rejection, asynchronous
  recovery admission, and device-loss restoration before CPU fallback.
- Executor-owned device capability reports for shader binding budget, texture
  limits and formats, floating-point filtering and compute availability, plus a
  checked seam that feeds the selected executor's report into every emission
  request kind.
- Executable cross-executor parity policy with numeric unfiltered and filtered
  tolerances for 8-bit UNORM, 16-bit UNORM and floating-point values, including
  absolute/relative comparison and first-failure diagnostics.
- A committed executor corpus spanning documents, strokes, cameras, materials,
  UV rasterization, depth, coverage and all declared value classes, with a CI
  gate that names backend drift and reports unavailable executors as unmeasured.
- Staged bounded CPU execution with cooperative cancellation, monotonic progress,
  enforced worker limits, pre-allocation memory-ceiling refusal, and atomic
  commit only after complete success.
- An optional owned Vulkan executor behind `CTEX_ENABLE_VULKAN_EXECUTOR`, using
  pinned Vulkan-Headers and Volk, with deterministic device selection, private
  instance/device/queue ownership, and physical-device capability reporting.
- A labeled execution-backend scenario suite mapping every OpenSpec scenario to
  default-build coverage, including 16K staged cancellation and the backend
  isolation regression.
- Semantic input-colour defaults, preview-only 3D LUTs, structured precision
  warnings, promoted height accumulation and deterministic ordered dithering.
- Slice-A memory-buffer PNG decoding and encoding with 8/16-bit preservation,
  content detection, colour metadata, extension diagnostics and pre-allocation limits.
- Extensible semantic channel descriptors, the nine-channel metallic/roughness
  preset, independent precision overrides and allocation-free disabled channels.
- Texture-set documents with partition/UV-derived stable identity and independent
  per-set resolution, precision and channel storage.
- Read-only in-memory mesh ingest with validated attributes, named UV buffers,
  total face partitioning, and texture-set derivation for material, object,
  submesh, and explicit-face sources.
- A flat CPU picking BVH with bounded leaves, sublinear candidate traversal,
  mesh-revision reuse, and rebuild-on-replacement behavior.
- CPU world-space ray construction for perspective and orthographic cameras,
  with documented matrix, clip-depth, and top-left viewport conventions.
- Exact BVH-backed picking with complete world position, smooth and geometric
  normals, named UV, texture-set, UDIM, triangle, barycentric, material and
  distance fields, plus a distinct non-error miss result.
- Default nearest and ordered all-hits ray occlusion, with per-call backface
  acceptance or rejection under documented counter-clockwise winding.
- Revision-keyed named-UV spatial indices and inverse UV-to-surface picking,
  constrained by texture-set partition with explicit unowned-coordinate misses.
- BVH-backed surface snapping within a maximum distance, with exact closest-point
  handling for triangle interiors, edges, vertices and degenerate triangles.
- Accelerated screen rectangle, lasso, world sphere and world box triangle
  queries with exact intersection filtering and deterministic result ordering.
- Deterministic lowest-index ownership for equivalent-distance nearest hits on
  shared mesh edges and vertices, with literal ordered behavior in all-hits mode.
- Ordered nearest-hit batches with explicit cancellation and memory-limit
  outcomes, interval progress callbacks, and no exposed partial results.
- A labeled headless picking scenario suite, including bounded BVH traversal on
  a 2,097,152-triangle indexed mesh.
- A device-independent material graph document with stable nodes, sockets and
  links; deep comparison; exact versioned serialization; and channel-derived
  output constants.
- Edit-time material graph cycle refusal with typed, deterministic path
  diagnostics and cyclic-input rejection during deserialization.
- Explicit material graph socket coercions, typed incompatibility diagnostics,
  and transactional one-link-per-input replacement reporting.
- A 52-type built-in material node catalogue with stable declarations, typed
  sockets, constrained properties, complete blend and normal modes, and
  formula-bearing scalar/vector math operations.
- Portable deterministic Noise evaluation, one shared reference formula for all
  twenty Blend modes, and canonical material libraries with stable preset
  identity, thumbnails, transfer, and independent graph instantiation.
- Reusable material node groups with workspace-owned subgraphs, transactional
  socket propagation across materials and nested groups, compatible value/link
  preservation, removed-link reporting, and typed recursion-path diagnostics.
- Emission-independent material graph validation with structured diagnostics for
  required inputs, image resources, mesh maps, group references, and unreachable
  nodes, including workspace ownership and linear reverse-CSR reachability.
- Instance-owned host node registries with checked CPU and target-specific
  emission callbacks, deterministic replay eligibility, parity fixtures,
  registry-aware validation, and lossless preservation of unknown node types.
- Vendored Kongruent minikong with context-owned compiler and WGSL backend state,
  deterministic wrapper reuse, and concurrent independent compilation.
- Reachability-driven WGSL expression emission with deterministic node and group
  names, emission-time coercion, attribution comments, and fan-out memoization.
- Context-isolated Kong target selection for WGSL, MSL, binary SPIR-V, and HLSL,
  including deterministic cross-target concurrency, target inventory, explicit
  unsupported-target diagnostics, and `spirv-val` coverage.
- Validated device-independent render and compute pass plans with versioned
  logical textures, tile-aware subresource hazards, derived lifetimes, explicit
  bindings/layouts, render state, and draw or dispatch commands.
- Feature-gated layer-stack shader emission for WGSL, MSL, SPIR-V, and HLSL,
  with deterministic binding-budget pass splitting, carried intermediates,
  format and dimension refusals, and a reported nearest-filter workaround for
  devices without floating-point linear filtering.
- Thread-safe graph and layer-stack emission caches keyed by collision-free
  canonical content, target, host-node semantics, and normalized device
  features, with immutable results, hit/miss statistics, and failure isolation.
- Four-thread graph and cross-target layer-stack emission fixtures that compare
  complete concurrent results with serial baselines.
- Cached WGSL, MSL, SPIR-V, and HLSL material preview emission with documented
  GGX metallic/roughness lighting, cube environment metadata, analytic lights,
  deterministic fallback lighting, and unlit per-channel inspection shaders.
- Complete material-graph shader entry points for WGSL, MSL, SPIR-V, and HLSL,
  with explicit resource bindings, stable pass layouts, capability workarounds,
  target/feature-aware immutable caching, and deterministic artifacts.
- Labeled material-graph and shader-emission scenario suites mapping every
  OpenSpec scenario to native, external-validation, policy, or determinism tests.
- Executor interface and registry with deterministic discovery, explicit
  availability, automatic/explicit/environment selection, pinned defaults, and
  checked recovery-aware fallback reports.
- Kong cube-texture lowering across all four retained shader targets.
- README architecture diagram and a current, implementation-scoped feature list.

### Changed

- Prioritize a real desktop/mobile painting slice before the full v1 catalogue;
  retain task IDs and bring reference hosts, bindings and budgets forward.
- Specify GPU-resident completion, asynchronous snapshots, recovery, full resource
  accounting and mobile lifecycle behavior.
- Separate brush coverage from build-up deposition and specify sample invariance,
  seam filtering and tangent-frame compatibility.
- Add extensible channel descriptors, CPU semantics for custom nodes, versioned
  bake requests, replay eligibility and persistent editable authoring.
