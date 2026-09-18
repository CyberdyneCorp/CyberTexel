# Design: bootstrap-v1-cybertexel

## Context

Three reference points define the problem space.

- **Substance Painter** (Adobe, closed, subscription, desktop-only). The
  standard the work is measured against. Its durable ideas are not the brushes:
  they are *texture sets* (one material per UV space per mesh partition),
  *mesh maps* (a baked map set every generator reads), *smart materials and
  smart masks* (a material that re-derives itself on a new model because it is
  parameterised by mesh maps rather than by pixels), *anchor points* (one layer
  referencing another's output as a mask source) and *channel-packing export
  presets*. Behavioural reference only, from public documentation — no code, no
  assets, no format.
- **ArmorPaint** (zlib, C, `armory3d/armorpaint`, specified in full at
  `/Users/leonardoaraujo/work/armorpaint/openspec/`). A working, permissively
  licensed, GPU-resident implementation of the hard half: texture-space paint
  rasterization, a node graph that compiles to shader source, an 18-mode blend
  set that exists identically in generated shader text and in a merge shader,
  and a GPU undo ring. We may read it, cite it and vendor from it.
- **The sibling engines.** ClayCore (`clay_*`, C++20, SDF/voxel/mesh sculpting)
  and CyberRemesherAndUV (`cyber_*`, C++20, retopo/UV/bake). Both have already
  declared texturing out of scope and named this library's inputs.

`openspec/specs/` is empty. This change is the founding spec for v1.

## Goals / Non-Goals

**Goals**

- One engine, one document, four consumption paths: Rust/`wgpu` desktop,
  Swift/Metal iPad, Python scripting, headless CLI.
- Pure C++20 with no UI toolkit and — in the host-executed path — **no owned GPU
  device**.
- Feature parity with ArmorPaint's paint pipeline and with Substance Painter's
  layer, mesh-map and smart-material model.
- Deterministic and testable headless: the CPU reference executor defines
  correct results and runs in CI on a machine with no GPU.
- The seams to the sibling engines stay a *format and an interface*, never a
  build dependency.

**Non-Goals** — see the proposal's Non-Goals section. The one worth restating
here because it shapes every other decision: **CyberTexel does not own a
renderer.**

## Decision 1 — The library does not own the host's GPU

This is the decision everything else follows from, and ClayCore already
established the pattern in `docs/06-host-gpu-previews.md`: when a host has its
own device, hand it data and let it draw. ClayCore offers a tape (route 1) or a
brick atlas (route 2) and notes its own dialect "does not target WGSL".

ClaySpaceDesktop renders with `wgpu 24`. A paint library that owned a
Metal/Vulkan device would be fighting the app's renderer for the same textures
across two APIs, and the interop cost would be paid on every stroke.

So CyberTexel offers **three execution routes**; the host-executed contract and
CPU reference are mandatory:

| Route | Who owns the device | Used by |
|---|---|---|
| **Host-executed** (primary) | The host | ClaySpaceDesktop (`wgpu`), a Swift/Metal shell |
| **CPU reference** (mandatory) | Nobody — plain memory | CI, tests, Python, headless CLI |
| **Owned GPU** (optional) | CyberTexel | A CLI that wants speed without a host renderer |

In the host-executed route the library produces, per operation, a **pass plan**:
shader source in the host's language, the render targets and their formats, the
uniform and texture bindings in declared order, the draw call and its vertex
layout. The host runs it. The library never sees a device handle.

This is exactly what ArmorPaint's `parser_material.c` already does — it emits
shader *text* and a resource list; the device work is the caller's. We are
keeping that architecture and removing the part where the caller happens to be
in the same binary.

## Decision 2 — The CPU reference defines correctness, with a stated tolerance

Both sibling engines hold this rule (`compute-acceleration`,
`evaluation-backends`) and both are right to. CyberTexel adopts it with one
honest amendment.

ArmorPaint's paint pass reads the *live viewport's* depth and UV g-buffers.
A CPU reference therefore cannot be "the same code without a GPU" — it must
rasterize its own depth and UV buffers from the mesh and camera. That is
tractable (UV-space triangle raster plus a per-texel capsule test) and it is
what makes headless testing possible at all, but it means the CPU path is a
*reimplementation*, not a fallback, and reimplementations drift.

The mitigation is a **parity fixture** rather than a promise: a committed corpus
of documents, strokes and cameras, rendered by every executor, compared per
texel against the CPU result within a declared tolerance, gated in CI. A backend
that cannot meet the tolerance is reported, not silently accepted.

Tolerance is per channel and stated in `execution-backends` rather than left to
the reader, because texture filtering, rasterization fill rules and float
contraction all differ legitimately between devices.

## Decision 3 — Shader emission, not shader interpretation

A material node graph can be evaluated two ways: interpret the graph per texel,
or compile it to a shader once. ArmorPaint compiles, and it is right — a graph
of thirty nodes evaluated per texel at 4K is 500 million interpreter steps per
channel.

We take ArmorPaint's architecture wholesale, including its three non-obvious
properties:

1. **One memo list does three jobs.** The record of already-emitted result
   variables is a common-subexpression cache for fan-out, a soft cycle guard
   (the name is recorded *before* the node's expression is generated, so a cycle
   terminates against a forward reference instead of recursing forever), and a
   per-node dedup hook.
2. **Group-qualified variable names.** Mangled with the name and id of every
   enclosing group plus the node's own id, so identically named nodes in
   different groups cannot collide.
3. **Coercion at codegen, not at edit time.** The editor allows any output into
   any input; a scalar broadcasts to a vector, a vector reduces by luminance or
   by `.x`. This is what keeps the graph UI simple and the rules in one place.

**Kong is the shader backend.** It is zlib-licensed, ~17k lines, depends on libc
plus a vendored `stb_ds`, and already emits HLSL, SPIR-V, MSL **and WGSL**. It
is vendored under `thirdparty/kong` and attributed. Writing a fifth emitter
backend for a shading language it does not target is the extension point, not a
rewrite.

We do **not** inherit ArmorPaint's global compiler state: Kong was a one-shot
CLI tool and ArmorPaint works around that with a snapshot/restore around every
call. We wrap it in a context object at the vendoring seam instead, so
concurrent emission from two threads is a supported operation rather than a
hazard.

## Decision 4 — The undo model, and its cost stated up front

ArmorPaint's undo is a ring of full-resolution GPU texture snapshots with an
O(1) pointer swap to restore. It is elegant and it is expensive: at 16K the
budget collapses to a single step, and ArmorPaint compensates by silently
clamping `undo_steps` to 1.

We keep the pointer-swap restore — it makes undo and redo the same operation and
needs no second snapshot — and change two things:

- **Tiles, not whole textures.** A stroke touches a bounded region; the snapshot
  is the set of tiles it touched. This is the single biggest deviation from
  ArmorPaint and it is what makes a 16K document usable with a real history.
- **A declared budget rather than a silent clamp.** The host sets a memory
  ceiling; the library reports how many steps that buys and refuses to exceed
  it, naming the ceiling. A budget that silently becomes one step is a bug
  report waiting to happen.

Non-pixel edits (rename, opacity, blend mode, reorder, graph edits) stay cheap
command records, as in ArmorPaint.

## Decision 5 — Texture sets, and what a "document" is bound to

Substance Painter's texture set is the right unit and ArmorPaint's per-object
layer masking is the weaker version of it. A texture set is *(mesh partition, UV
set)* and owns its own resolution, its own layer stack and its own mesh maps.
UDIM tiles are a partition of one texture set's UV space, not separate sets.

This resolves an ambiguity ArmorPaint leaves open: there, a layer has both an
`object_mask` and a `uv_map` index, and the interaction between them is
implicit. Here, the binding is the texture set's, and a layer within a set needs
no mesh binding at all.

## Decision 6 — Mesh maps are consumed, never baked

`mesh-maps` defines the map set (world/tangent normal, ambient occlusion,
curvature, thickness, position, world-space direction, material ID, object ID,
UV density) and a `ctex_bake_provider` interface of C callbacks. CyberTexel ships
**no baker**.

CyberRemesherAndUV already bakes normal, AO, height, colour, curvature and
cavity through an editable cage with a texel ceiling. Six issues filed against
it cover the gap (bent normal, thickness, position, ID maps, and the provider
binding itself). Until a provider is attached, generators that need a missing
map report it by name rather than producing a plausible-looking flat result —
the failure mode that makes a smart material look subtly wrong on a new model
instead of loudly broken.

## Decision 7 — Symbol prefix `ctex_`, not `cyber_texel_`

CyberRemesherAndUV's `capi/cyber_capi.symbols` exports the wildcard `_cyber_*`.
ClaySpaceDesktop links both libraries. A second library exporting into the
`cyber_` namespace would make that export map ambiguous, so CyberTexel takes a
distinct prefix: `ctex_*`, matching ClayCore's `clay_*` in spirit and length.

## Decision 8 — Per-tile revisions are what make Decision 1 affordable

A committed tile is a logical resource version, not necessarily a CPU buffer.
In the host-executed route its authoritative pixels remain on the host's GPU;
CPU-authored tiles can be uploaded through the same revision protocol. The core
tracks resource identities, generations and access requirements without device
handles. Hosts translate plans into API synchronization and report completion.

A successful completion against the expected base revision publishes the new
tile versions atomically. Stale and cancelled completions cannot publish. Old
versions remain pinned while history, snapshots or in-flight passes reference
them. Undo exchanges logical versions and requires no synchronous readback.

Delta queries identify changed versions and residency. They do not implicitly
fetch pixels. CPU access, save and export use explicit asynchronous readback of
an immutable, releasable snapshot. The snapshot's retained versions count toward
the resource budget. An unchanged delta query is independent of canvas size.

Before commit, recovery must be possible from a checkpoint plus deterministic
operation records with pinned inputs, or from an asynchronously completed pixel
checkpoint. Device loss restores the last committed revision before CPU fallback.
A durable save is a separate milestone: mobile suspension reports whether the
newest checkpoint reached backing storage before the host's deadline.

## Decision 9 — Examples are the test suite, not a demonstration tier

ClayCore's `examples` capability makes the gallery a CI gate. We take the same
approach and go one step further: the examples are Python, they drive the same
binding the integration suite uses, and they are the project's end-to-end check.
There is no separate tier of scripts that merely illustrate.

That has a cost worth accepting deliberately — an example must assert, not print,
and its committed output must be updated in the same commit as any change that
alters it. In exchange, a capability without an example fails a gate, and a
change that breaks a picture fails CI rather than being found when somebody next
looks.

`cli-headless`'s `run` subcommand executes the same Python, so an operation
reachable from an example is reachable from a pipeline without a new flag.

## Decision 10 — Resource budgets cover the whole engine

`resource-residency` accounts for document tiles, composites, maps, history,
recovery, pinned snapshots and temporaries. Shared CPU/GPU allocations count once
in physical totals. Sparse tiles, reconstructible cache eviction and lossless
backing storage bound residency; tiled undo alone cannot do that. Hosts provide
storage and lifecycle signals, and choose preview degradation policies. Authored
resolution and precision never silently change to fit a device. Residency metadata
and operation records belong to `doc`, tile storage to `image`, lifecycle and
completion transport to `xport`, and durable checkpoints to `io`. These are
capabilities within the existing module graph, not new backend dependencies.

## Decision 11 — Separate coverage, deposition and input reconstruction

Non-building brushes take maximum geometric coverage; build-up brushes accumulate
deposition using the formula in `paint-engine`. Opacity caps the result. Continuous
and discrete-alpha tips are independent choices. Versioned canonical sampling
makes deposition independent of render frames and batching. Stabilization uses
timestamps rather than input callback counts; omitted path detail cannot be
reconstructed by a sampling-invariance promise.

Surface-continuous filters use seam adjacency and tangent-frame conversion.
Padding has a declared mip range and insufficient gutters are diagnosed rather
than hidden by a universal two-texel promise.

## Decision 12 — Editable content has explicit replay limits

`editable-authoring` retains eligible operation records and checkpoints. Decals,
text and surface paths remain editable after committing a gesture. Resizing
chooses replay or resampling explicitly. Snapshot-dependent operations pin their
inputs or remain checkpoint-only. Reprojection keeps the source until an atomic
commit and reports ambiguous or unmapped regions. Recovery records and editable
history share versioned operation semantics but have independent retention needs.

Channel descriptors carry semantic IDs, precision, defaults and blending/export
policies. The nine built-in channels form a preset. Custom nodes require CPU
semantics as well as emission so extensibility does not bypass parity or recovery.
The C ABI exposes these descriptors before being frozen; a convenience C++20
RAII surface may wrap the same handles without promising a stable C++ binary ABI.

## Decision 13 — Validate the product with a thin real-device slice

Task groups are capability inventories, not a demand to complete every decoder
and graph node before painting. The delivery slices in `tasks.md` govern order.
Desktop WGSL and mobile MSL hosts, minimal bindings, numeric device budgets and
recovery fixtures land with the first painting workflow. Full catalogue breadth
follows. No task is marked complete merely because its first-slice subset works.

## Risks

| Risk | Mitigation |
|---|---|
| The CPU reference executor drifts from the GPU path | Parity fixture in CI with a declared per-channel tolerance; a failing backend is reported, not skipped |
| Host-executed route pushes too much work onto the host | The pass plan is complete and ordered — resources, bindings, draw, vertex layout; a reference `wgpu` host lives in `examples/` and is CI-built |
| Kong's global state under concurrent emission | Wrapped in a context object at the vendoring seam; a concurrency test is a release gate |
| Tiled undo is materially harder than whole-texture undo | The tile grid is one dimension of the document; restore is still a swap, per tile. Budget accounting is a spec requirement with a test |
| Smart materials are the largest unproven surface | They sit on mesh maps and the node graph, both of which land first; `smart-materials` is the last group in the task plan |
| No baker means v1 demos poorly | The repository carries a fixture map set so examples run with no CyberRemesher present |
| Image decoders are the largest hostile-input surface | Every decoder is fuzzed in CI, allocations are bounded by a configurable ceiling, and declared dimensions are validated before allocation |
| Per-tile revision bookkeeping costs more than it saves on small documents | The delta query's cost is a budgeted operation in `device-gate`; if the bookkeeping dominates on small documents the budget shows it |
| Examples doubling as the test suite makes them brittle | Tolerances are stated per comparison, seeds are explicit, and outputs are updated in the same commit as the change that alters them |

## Module layering

Enforced by a build gate, following ClayCore's `build-packaging` precedent:

```
image   -> (nothing)                 pixel buffers, formats, colour spaces,
                                     decoders and encoders
mesh    -> (nothing)                 mesh views, UV sets, partitions, revisions
pick    -> mesh                      rays, hit records, snapping, region queries
graph   -> image                     node documents, no shading language
emit    -> graph, image              shader text + pass plans; no device
doc     -> image, mesh, graph        texture sets, layers, history, tile
                                     revisions
paint   -> doc, emit, pick           tools and stroke application
maps    -> image, doc                mesh maps and the provider interface
xport   -> doc                       revisions, delta queries, tile readback
io      -> doc, image                container format and texture export
exec    -> emit, image               executors; the only module that may
                                     touch a device, and it may not be
                                     depended on by any of the above
capi    -> everything
```

No core module other than the `capi` composition boundary may depend on `exec`,
and no module below `capi` may depend on a backend. A cycle is a build failure, not a review comment.
