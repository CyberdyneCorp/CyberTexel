# Main features

The current implementation provides:

- Versioned [canonical stroke reconstruction](stroke-reconstruction.md)
  from strictly timestamped 3D samples, with redundant-sample invariance, a
  documented fixed stabilization grid and recurrence, radius-relative spacing,
  continuous sweep links, separated discrete-alpha tip events, and independent
  pressure/tilt response curves, deterministic jitter, entry/exit taper, and
  straight-line, dominant-axis, and grid constraints, plus object-plane and
  radial symmetry within one resolved stroke. Hosts with their own stroke
  engine can ingest validated resolved stamps without reapplying modifiers;
  named [stroke presets](stroke-presets.md) serialize canonically with
  explicit schema migration and future-version refusal.
- Camera-independent [texture-space paint coverage](paint-coverage.md)
  with interpolated surface geometry, gap-free continuous swept capsules,
  transformed discrete tips, specified hardness falloff, and UV, squared-normal
  triplanar, or caller-framed planar material coordinates. The separate
  [paint rejection stage](paint-rejection.md) provides default-on depth and
  angle tests, per-operation backface handling, explicit symmetry depth policy,
  and precision-aware alpha discard without losing accumulated strength.
  Canonical [paint deposition](paint-deposition.md) separates non-building
  coverage maxima from build-up flow recurrence and is invariant to frame,
  batch, or repeated-segment rasterization. The
  [paint shading stage](paint-blending.md) evaluates all twenty shared blend
  modes against an immutable stroke-start snapshot rather than partially painted
  output. Typed [paint masks](paint-masking.md) intersect active-layer,
  colour-ID, geometry/polygon, screen, and UV-island restrictions before those
  canonical events enter deposition. A revision-keyed
  [paint surface-map cache](paint-surface-cache.md) reuses immutable
  coverage, exact source-triangle, and UV-island maps across operations and
  invalidates them on mesh replacement or UV-set changes. Configurable
  [UV seam dilation](paint-seam-dilation.md) extends directional gradients
  into texture gutters and is deferred until every dirtied tile reaches stroke
  finalization. [Seam-aware filtering](seam-aware-filtering.md) consumes
  explicit surface adjacency, transforms tangent-space vectors across mirrored
  frames, and reports mip levels whose island gutters are insufficient. Isolated
  [paint preview sessions](paint-preview.md) use
  copy-on-write channel storage, refuse stale commits, and publish the exact
  finalized preview—including dilation—as the committed result. The
  [bounded-work scheduler](paint-bounded-work.md) expands exact stamp
  footprints by that dilation radius, processes only the resulting deduplicated
  storage tiles, and reports the full ordered tile set. Tool-level
  [Brush and Eraser](brush-and-eraser.md) compose the canonical mask,
  deposition and stroke-start blending stages across enabled channels or layer
  opacity/mask values. The [Fill tool](fill-tool.md) resolves whole-set,
  exact-triangle, connected-angle, UV-island, UV-tile and selection scopes from
  cached surface identities and explicit mesh adjacency. The
  [Clone tool](clone-tool.md) copies an immutable source snapshot using
  aligned or fixed UV sampling and explicitly refuses cross-texture-set clones.
  [Blur and Smear](blur-and-smear.md) use surface-aware sampling over an
  immutable stroke-start snapshot, including tangent-frame-correct normal maps.
  [Decal and Stencil](decal-and-stencil.md) provide editable surface-frame
  material projection and transformable, invertible screen-space restrictions.
  The [Projection tool](projection-tool.md) applies material images through
  a current-view camera with explicit visibility, a finite planar frame, or
  repeat-addressed normal-weighted triplanar mapping.
  The [Text tool](text-tool.md) strictly decodes UTF-8, lays out supplied
  deterministic glyph coverage with tracking and line alignment, and projects
  the result as a size-aware material decal.
  The [Particle tool](particle-tool.md) runs fixed-step, seeded mesh
  collision simulation with configurable physical response and deposits its
  ordered contacts through texture-set-aware canonical paint shading.
  The [Picker tool](picker-tool.md) resolves a surface hit to its exact
  texture-set/UDIM texel, returns every enabled channel, and optionally returns
  explicit per-texel material provenance.
  [Colour-ID selection](colour-id-selection.md) creates an explicit binary
  region using a configurable linear-RGB tolerance, including an observable
  empty result, for paint, mask, and visibility consumers.
  The [Selection tool](selection-tool.md) maps screen rectangles, lassos,
  triangles, UV islands, and connected-by-angle polygon regions into active or
  independently stored texture-space masks.
- Sparse tiled image storage for one-to-four-channel 8-bit, 16-bit, and
  floating-point pixels, with tile-level dirty tracking and monotonic channel
  and per-tile [content revisions](host-transport-revisions.md), plus
  epoch-qualified coalesced delta queries carrying residency and generation
  metadata, explicit full-resynchronization signaling, and asynchronous named
  tile readback into caller-owned buffers with a declared direct-upload memory
  layout, host-controlled format negotiation, and budgeted copy-on-write
  snapshot tokens for consistent query-to-readback synchronization. Delta
  queries use a change-proportional revision index rather than scanning the
  document tile grid. Isolated in-flight preview resources use these same
  delta, snapshot, layout, format, and asynchronous readback contracts without
  modifying committed document pixels.
- Linear Rec. 709 and sRGB colour transforms, semantic input policies,
  preview-only 3D LUTs, ordered dithering, and promoted-precision operations.
- Memory-buffer decoding for PNG, JPEG, TGA, BMP, baseline TIFF, OpenEXR,
  Radiance HDR, and PSD, including named PSD layers and multipart OpenEXR parts
  in composited or individual mode, with content-based detection, extension-
  mismatch reporting, allocation limits, 8/16-bit preservation where carried,
  and unclamped floating-point HDR input. Memory encoding covers PNG, JPEG,
  TGA, baseline TIFF, and flat OpenEXR.
- A versioned, forward-preserving [project container](project-container.md)
  with a probeable header, independently compressed sparse tile storage, and
  portable referenced or packed image, font, map, and mesh resources with
  explicit missing-resource reports. Saves are atomically published and
  byte-identical for unchanged projects; [copy-on-write snapshots and periodic
  autosave](project-autosave.md) keep compression and filesystem work off
  the paint thread, quiesce against host lifecycle deadlines, and expose restart
  recovery candidates with their durable revision. The same container
  supports [referenced or self-contained standalone asset packages](standalone-assets.md)
  for materials, smart content, brushes, presets, and node groups, plus
  canonical [versioned operation records](operation-records.md) with pinned
  replay inputs and raster checkpoint dependencies.
- Data-only [texture export presets](export-presets.md) with a documented
  channel-token vocabulary, exact metallic/roughness derivations, registered
  channel and named mesh-map addressing, and six built-in PBR packing
  conventions. Memory encoders cover PNG, JPEG, TGA, TIFF and OpenEXR with an
  explicit 8/16/32-bit compatibility matrix and typed refusal of impossible
  combinations. Pure [export planning](export-planning.md) composes
  texture-set, UDIM, atlas, and layer scopes with predictable filename tokens
  and preflight collision refusal. [Texture export execution](texture-export.md)
  adds bilinear output sizing, shared extrapolating UV padding, dry-run and JSON
  manifests, cancellable progress, and self-describing in-memory encoded
  buffers. The complete pipeline is also available through the stable
  [C ABI](c-abi.md#texture-export) using caller-owned callback delivery.
- Extensible semantic channels and a nine-channel metallic/roughness PBR preset,
  with independent precision and allocation-free disabled channels.
- Texture-set documents derived from mesh partitions and named UVs, with stable
  identities, independent channel storage, and a canonical ordered
  [layer stack](layer-stack.md). Entries explicitly distinguish paint,
  fill, group, mask, filter, instance and editable authoring kinds; validated
  group nesting and single-target attachments are transactional, and fill-graph
  edits invalidate derived content by revision. Instances dynamically reference
  preceding content while owning opacity, blend, channel and mask modulation;
  direct painting, cycles and ambiguous source deletion are explicitly refused.
  The same stack validates all twenty formula-defined blend modes against the
  shared material-graph CPU reference, with Pass Through restricted to groups.
  Per-channel participation now resolves against texture-set enablement and
  computes effective opacity through nested groups and direct/group mask chains,
  refusing incomplete or invalid per-texel mask inputs. The device-free
  [CPU compositor](layer-compositing.md) evaluates that stack bottom to top,
  including isolated/Pass Through groups, instances, resolved filters, channel
  blending policies and independent coverage, with byte-identical repeat output.
  [Atomic layer operations](layer-operations.md) cover create, subtree
  duplicate/delete, reorder, reparent, clear, invert, merge down/group, flatten,
  paint/fill conversion and mask application; destructive bakes are recomposited
  and refused if they do not preserve the documented appearance. Separate
  [tile-scoped history](tile-history.md) declares channel/tile write sets,
  retains only changed tiles under a reported byte ceiling, and performs
  symmetric undo/redo by exchanging exact storage owners with zero pixel copies.
  [Texture-set transactions](transactions.md) isolate repeated pixel, bulk
  region or full-channel writes, and layer operations until commit, collapse
  them into one mixed undo step, and make cancellation an exact discard with no
  live revision change.
- Validated read-only [mesh ingest](mesh-ingest.md), reusable flat CPU
  acceleration structures, overlap and coverage diagnostics, and two-phase mesh
  replacement that reports per-set UV changes before the host chooses to keep,
  clear, or defer content for reprojection. Sparse [UDIM storage](udim-tiles.md)
  provides checked standard addressing, UV-addressed cross-border batch writes,
  on-demand physical allocation, and occupied-tile memory reporting. Validated
  [atlas regions](atlas-regions.md) group texture sets into deterministic,
  region-aware export outputs.
- Revision-aware [mesh-map sets](mesh-maps.md) with named missing-map and
  staleness reports, asynchronous stale-safe bake tokens with coordinated undo,
  a host-supplied bake-provider seam, and transactional
  external strided-buffer import with explicit channel meaning and colour space,
  recorded OpenGL/DirectX normal conventions normalized on read, and eight
  built-in mask generators with queryable map requirements, bounded parameters,
  clamp reports, cross-executor parity fixtures, document-level memory
  accounting, and host-driven selective or bulk release.
- Perspective and orthographic ray picking, ordered occlusion, configurable
  backface policy, inverse UV picking, surface snapping, region selection,
  deterministic shared-boundary ownership, and bounded cancellable batches.
- A device-independent material graph with canonical serialization, cloning,
  comparison, edit-time cycle diagnostics, typed socket coercion, and atomic
  one-link-per-input replacement. Its
  [built-in node catalogue](material-node-catalogue.md) declares all input,
  texture, colour/filter, vector, and math node schemas, while reusable
  [node groups](material-node-groups.md) propagate interface changes and
  refuse recursive placement. Emission-independent
  [graph validation](material-graph-validation.md) reports required inputs,
  missing resources, missing groups, and unreachable nodes. Versioned
  [host node types](host-node-types.md) provide checked CPU/emission
  callbacks, replay eligibility, parity fixtures, and lossless opaque fallback.
  Portable seeded-noise and shared twenty-mode blend formulas support
  conformance fixtures, while canonical [material libraries](material-library.md)
  move stable, named graph presets between machines.
- Canonical [smart-material serialization](smart-material-serialization.md)
  preserves ordered layer, group, mask, filter and generator fragments with
  embedded graphs and typed, ranged, display-grouped exposed parameters. It
  keeps derived content definition-only while preserving and reporting
  model-specific painted pixels, and one atomic parameter update can drive
  input sockets and node properties across multiple entries. Historical schemas
  migrate with documented content, binding, anchor, and resource defaults;
  future versions are refused before a destination can change.
- Reusable [smart masks](smart-masks.md) package derived mask, generator
  and filter graphs for layers or groups; every instance gets independent,
  typed parameter state without mutating the preset or sibling instances.
  [Preset application](preset-application.md) deep-copies material and mask
  fragments into editable texture-set entries, records per-entry origin and
  model-specific content evidence, and removes any-size fragments in one undo.
- A unified [preset shelf library](preset-library.md) enumerates materials,
  smart materials, smart masks, brushes, stroke presets, generators, and export
  presets with stable identities, metadata, sorted tags, embedded thumbnails,
  and per-kind format-version refusal before resolution.
  [Scenario coverage](smart-material-scenarios.md) maps every smart-material
  requirement to the labeled executable evidence that enforces it.
- A stable [C ABI foundation](c-abi.md) with a prefix-only shared-library
  export surface, opaque document handles, distinct integer result categories,
  exception containment, and allocation-free per-thread diagnostics. Bulk
  results use atomic caller-owned buffers with null-buffer sizing queries; stable
  texture-set IDs exercise the complete two-call contract. Size-first texture-set
  descriptors accept older prefixes with documented defaults and refuse
  implausibly large layouts before mutation. An explicit ABI version query and
  [compatibility baseline](c-abi.md#abi-version-and-compatibility) reject
  same-major symbol, signature, descriptor, enum, or export drift. The same
  public header is compiled by a strict C11 consumer test. Its documented
  threading contract permits concurrent work on distinct documents, requires
  caller serialization per shared handle, and keeps diagnostics thread-local.
  A silent-by-default host log sink provides severity/category routing and
  stable diagnostic codes alongside English messages for host localization.
  Host allocator callbacks retain provenance per opaque handle and back all
  persistent storage reachable through the current C surface, so the process
  default can change without mismatched destruction.
- A [Python 3.10+ binding](python-binding.md) that bundles the stable C ABI
  in Linux, macOS and Windows wheels, checks ABI compatibility at import, maps
  native failures to typed exceptions, and exchanges decoded images, channel
  snapshots, meshes, mesh maps and generated masks as NumPy arrays.
- A [SwiftPM binding](swift-binding.md) for macOS 13 and iOS 16 that resolves
  the installed C ABI through a system-library target, presents document and
  texture-set values with thrown typed errors, and releases shared native
  handles automatically and exactly once.
- Separate [Rust raw and safe crates](rust-binding.md):
  `cybertexel-sys` builds the native C ABI, while `cybertexel` owns document
  handles, returns typed diagnostic-bearing errors, confines `unsafe` to one
  audited boundary, and encodes the per-document threading contract by making
  `Document` movable between threads (`Send`) but not shareable (`Sync`).
- A shared [binding host-execution workflow](binding-host-transport.md)
  that emits real shader source and JSON pass plans, records host-resident
  completion without pixel transfer, and makes pinned asynchronous readback
  explicit in Python, Swift and safe Rust.
- An isolated [Kong shader compiler context](kong-backend.md) that compiles
  Kong source deterministically to WGSL, MSL, binary SPIR-V, or HLSL, supports
  concurrent independent targets, and reports unsupported requests explicitly.
- Deterministic [material graph emission](graph-emission.md) with
  node-derived result names, complete nested-group qualification, emission-time
  coercion, attribution and fan-out. Complete cached material entry points
  produce WGSL, MSL, validated SPIR-V or HLSL beside a stable pass plan.
- Validated, device-independent [pass plans](pass-plans.md) with logical
  resource generations, mip/layer/tile access ranges, dependency hazards,
  derived lifetimes, explicit bindings and layouts, render state, and
  draw/dispatch commands, plus structured stable identities for host-cached
  render and compute pipelines.
- Feature-gated [layer-stack emission](feature-gated-emission.md) for all
  four shader targets, with deterministic binding-budget pass splitting,
  carried intermediates, format/dimension checks, and reported float-filtering
  fallback.
- Collision-free [emission caches](emission-cache.md) keyed by canonical
  graph or layer content, shader target, host-node semantics, and normalized
  device features, returning immutable identical source and pass plans on hits.
  Four-thread fixtures verify graph, target-compiler, cache, shader, and pass-plan
  results against serial emission.
- Cross-target [material preview shaders](preview-shading.md) with a
  documented GGX metallic/roughness model, explicit environment and analytic
  light contracts, defined fallback lighting, and unlit inspection for every
  supplied channel.
- Instance-owned [executor discovery and selection](executor-selection.md)
  with stable enumeration, runtime availability, explicit and `CTEX_EXECUTOR`
  process defaults, deterministic automatic policy, and recovery-aware fallback
  reports.
- An always-available [CPU reference executor](cpu-reference-executor.md)
  with a CPU-semantics contract for every operation, homogeneous camera clipping,
  deterministic depth-tested viewport rasterization, and independent UV-space
  texel rasterization with owned depth, UV, coverage, and triangle buffers.
- [Bounded executor work](execution-control.md) with cooperative
  cancellation, serialized progress, real CPU worker limits, pre-allocation
  memory admission, and commit-only-on-success staging.
- A device-free [host execution protocol](host-execution.md) with explicit
  resource ownership and state, completion-token lifetime tracking, validated
  host outputs, atomic revision publication, cancellation, stale-result
  rejection, recovery-before-publication, and device-loss recovery before CPU
  fallback.
- Unified [resource accounting](resource-accounting.md) with stable
  physical allocation identities, independent CPU/GPU/backing-store roles,
  pinned and in-flight visibility, seven ownership categories, and
  API-independent host device descriptors. Atomic reservations enforce
  separate ceilings, schedule bounded tile batches, and evict only eligible
  reconstructible caches when required for progress. Sparse authored tiles can
  be evicted only after a host confirms lossless backing, then transparently
  reload by exact generation across flat and UDIM channel storage. The checked
  [resource-residency scenario matrix](resource-residency-scenarios.md)
  maps accounting, bounded admission, eviction, quality policy and suspension
  recovery to one labeled suite.
- Executor-owned [device capability reports](executor-capabilities.md) for
  binding budget, texture limits and formats, float filtering, and compute,
  wired directly into layer-stack, material, and preview emission requests.
- Numeric [cross-executor parity tolerances](executor-parity.md) for
  normalized 8-bit and 16-bit channels and absolute-plus-relative floating-point
  channels, with separately bounded filtered values and measured diagnostics.
- A committed document/stroke/camera/material parity corpus and CI gate that
  measures every available executor against the CPU reference and explicitly
  reports compiled but unavailable routes as unmeasured.
- An optional [owned Vulkan executor](vulkan-executor.md), disabled by
  default, with pinned loader/header dependencies, deterministic physical-device
  selection, owned headless instance/device/queue lifetime, and live capability
  reporting.
- Strict C++20 builds, sanitizer coverage, OpenSpec validation, dependency
  layering checks, licence auditing, deterministic-output gates, and labeled
  [material-graph](material-graph-scenarios.md),
  [shader-emission](shader-emission-scenarios.md),
  [execution-backend](execution-backend-scenarios.md), and
  [host-transport](host-transport-scenarios.md),
  [resource-residency](resource-residency-scenarios.md),
  [texture-document](texture-document-scenarios.md),
  [image I/O](image-io-scenarios.md),
  [project-I/O and texture-export](io-scenarios.md),
  [mesh and texture sets](mesh-and-texture-sets-scenarios.md), plus combined
  [stroke-model and paint-engine](paint-scenarios.md) scenario suites.

Every item above is reachable through the C ABI and the three bindings, and each
capability's OpenSpec scenarios are mapped to a labeled, executable suite. The
remaining release platforms and measured performance gaps are tracked in the
[roadmap](../openspec/ROADMAP.md).
