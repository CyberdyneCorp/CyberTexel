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
- Linear Rec. 709 working-space and sRGB transfer functions with unclamped HDR
  conversion.
- Native and sanitizer test presets plus a registry-driven determinism gate that
  refuses empty output categories.
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
- Reusable material node groups with workspace-owned subgraphs, transactional
  socket propagation across materials and nested groups, compatible value/link
  preservation, removed-link reporting, and typed recursion-path diagnostics.
- Emission-independent material graph validation with structured diagnostics for
  required inputs, image resources, mesh maps, group references, and unreachable
  nodes, including workspace ownership and linear reverse-CSR reachability.
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
