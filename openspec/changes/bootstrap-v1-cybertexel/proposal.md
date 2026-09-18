# Proposal: bootstrap-v1-cybertexel

## Why

The Cyberdyne geometry pipeline is `sculpt -> retopo -> UV -> bake` and then it
stops. ClayCore forms the surface, CyberRemesherAndUV retopologises it, unwraps
it and bakes maps onto it — and there is nothing that turns those maps into a
finished, exportable PBR material. An artist leaving ClaySpace today leaves with
triangles and a normal map, and has to open Substance Painter to finish the job.

Both existing engines closed this door deliberately, and named where it should
open instead:

- ClayCore's `decide-surface-colour` record (2026-08-17) concludes **"claycore
  owns colour as a field property; it does not own material authoring"**, and
  states PBR channels are a declared non-goal for painting because *"what they
  mean is a texture, and a texture needs UVs"*.
- CyberRemesherAndUV's founding design lists **"Sculpting, painting, or
  texturing beyond baking outputs"** as an explicit non-goal.

Neither decision was drift, so neither should be reopened. What is missing is a
third library, and its shape is already constrained by those two: it consumes
UVs and baked maps rather than producing them, it owns no geometry kernel, and
it must run inside a host that already owns a GPU device.

The reference implementations are **Substance Painter** (closed, subscription,
desktop-only, the de-facto standard) and **ArmorPaint** (zlib, C, GPU-resident,
whose paint pipeline we have specified in full and may borrow from directly).
Neither is available as an embeddable library. CyberTexel is that library.

## What Changes

- Create a portable **C++20 texture-painting and material-authoring engine**
  with no UI toolkit, no window, and no owned GPU device in the host-executed
  path — consumable from a Rust/`wgpu` desktop app, a Swift/Metal iPad app, a
  Python script, or a headless CLI.
- Own a **texture document**: a layer stack of PBR channel sets with groups,
  masks, filters, fill layers, blend modes, per-object and per-UV-set binding,
  and a bounded undo history whose cost is stated rather than discovered.
- Implement **texture-space paint rasterization** as ArmorPaint does it — the
  mesh rendered with UVs as clip-space position so one fragment is one texel,
  each texel tested against a screen-space swept capsule reconstructed from a
  depth buffer. No dab spacing, no gaps on a fast stroke, and correct results
  for texels the camera cannot see.
- Implement the **tool set** both reference products share: brush, eraser, fill
  (object / face / angle / UV island), clone, blur, smear, decal, stencil,
  projection, text, particle, colour picker, colour-ID mask and geometry mask.
- Implement a **material node graph** that compiles to shader source rather than
  to an interpreter — the ArmorPaint architecture, whose Kong compiler already
  emits **WGSL**, the one shading language ClayCore's own dialect explicitly
  does not target and exactly what `wgpu` hosts speak.
- Implement **smart materials, smart masks, anchor points and generators** —
  the Substance Painter capability that makes a material library reusable across
  models, built on the mesh maps CyberRemesherAndUV bakes.
- Implement **texture export** with the channel-packing preset model: a preset
  is data, so Unreal, Unity, Unigine, Minecraft and X-Plane conventions cost no
  engine code.
- Expose a **stable C ABI** (`ctex_*`) with versioned descriptors, and
  **Python, Swift and Rust bindings** held at parity with it, so ClaySpaceDesktop
  and a future mobile shell consume the same surface the test suite exercises.
- Establish **build and packaging**: CMake + presets, permissive-only
  dependencies, CI gates, platform packages, one source of truth for the version.

## Capabilities

### New Capabilities

- `texture-document`: The layer stack — layers, groups, masks, filters, fill
  layers, PBR channels, blend modes, per-object and per-UV-set binding, and the
  undo history with its stated memory ceiling.
- `stroke-model`: What a stroke is before it lands on anything — spacing,
  pressure and tilt response, deterministic jitter, taper, stabilizer, ruler and
  grid constraints, symmetry planes, and the ingestion path for stamps resolved
  by a host's own stroke engine.
- `paint-engine`: The texture-space rasterizer — swept-capsule coverage, hardness
  falloff, depth and angle rejection, alpha discard, the per-stroke coverage mask
  that prevents self-overlap darkening, coordinate modes, and UV seam dilation.
- `paint-tools`: The tools the strokes drive, their parameters, defaults and
  the masking rules every one of them inherits.
- `material-graph`: The node document — catalogue, sockets, groups, validation,
  coercion rules, and the presets a graph can be saved as.
- `shader-emission`: Graph and layer stack to shader source and a pass plan —
  the emitted dialect, the four target languages, the declared resources, and the
  caching and invalidation rules.
- `execution-backends`: Where the passes actually run — the CPU reference
  executor that defines correctness, optional GPU executors, the host-executed
  route where the library never touches a device, and the parity gate between
  them.
- `mesh-and-texture-sets`: Mesh and UV ingest — UV sets, UDIM tiles, texture
  sets, atlases, per-object masks, and the rules for a mesh that is replaced
  under a document that has already been painted.
- `mesh-maps`: Consuming baked maps rather than baking them — the map set, the
  interface a baker implements, the CyberRemesherAndUV binding, and the
  behaviour when a required map is absent.
- `smart-materials`: Reusable material and mask presets — smart materials, smart
  masks, anchor points, generators, and the resource resolution that makes a
  shelf portable between machines.
- `color-management`: Colour spaces in, through and out — the working space, per
  channel policy, LUTs, and the bit-depth rules that keep a normal map from
  banding.
- `texture-export`: Getting texture sets out — presets, channel packing, formats,
  bit depths, padding, and the per-object, per-UDIM and per-atlas scopes.
- `project-io`: The document container — schema, versioning, packed assets,
  autosave and crash recovery.
- `c-abi`: The boundary every host crosses and the rules that keep it crossable.
- `language-bindings`: Python, Swift and Rust surfaces, and the parity rule that
  keeps them from becoming convenience subsets.
- `build-packaging`: How the library is built, layered, gated, versioned and
  shipped.

### Modified Capabilities

_None — greenfield repository; `openspec/specs/` is empty._

## Impact

- **New codebase**: everything under this repo (`src/`, `include/`, `capi/`,
  `python/`, `swift/`, `rust/`, `tests/`, `examples/`).
- **Dependencies**: permissive-only (MIT/BSD/Apache-2.0/MPL-2.0/zlib). The two
  candidates worth naming are **Kong** (zlib, ~17k lines, libc-only, already
  emits HLSL/SPIR-V/MSL/WGSL) vendored as the shader backend, and ArmorPaint
  itself (zlib) as a source of directly reusable algorithm code. Both clear the
  policy CyberRemesherAndUV enforces.
- **Sibling repos**: no build or link dependency on ClayCore or
  CyberRemesherAndUV. The seams are a format and an interface, following
  `pipeline-bridge`'s precedent. Six issues are filed against
  CyberRemesherAndUV for the map and preset work that belongs on its side.
- **Hosts**: ClaySpaceDesktop gains a third vendored engine bound the same way
  as the other two (`cybertexel-sys` + a safe wrapper). A future mobile shell
  consumes the Swift package.

## Non-Goals

**Permanently out of scope** — these belong to a sibling and will not get a
capability here:

- Geometry authoring of any kind: sculpting, remeshing, UV creation, mesh
  editing. CyberTexel reads a mesh and its UVs and never writes them.
- **Executing** bakes. CyberTexel defines the map set it consumes and the
  interface a baker implements; CyberRemesherAndUV does the baking.
- Owning a window, an event loop, a UI toolkit or an application shell.
- Adobe `.sbsar` ingestion — a proprietary format behind a licence we cannot
  meet.

**Deferred to a later version** — v1 must not preclude them:

- Generative/neural nodes. ArmorPaint ships them; they are a separable concern
  and a large dependency surface. The node graph must stay extensible enough to
  add them without reshaping.
- A path-traced preview. Hosts render; if one wants a reference path tracer it
  is a separate library, not a capability here.
- Sharing one document between two hosts concurrently.
