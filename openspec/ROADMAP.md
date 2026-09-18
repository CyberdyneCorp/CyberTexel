# Roadmap

The authority on what is done is `openspec/changes/bootstrap-v1-cybertexel/tasks.md`
and its checkboxes. This file is the milestone view and the running log of
decisions taken and questions still open.

## Status

Pre-implementation. 22 capabilities, 304 requirements, 368 scenarios, 204 tasks,
0 done. No source tree exists yet; task 1.1 creates it.

## Milestones

| # | Milestone | Task groups | Done when |
|---|---|---|---|
| 1 | **It builds and it checks itself** | 1, 2 | Headless CMake build, module layering gate, colour transforms, every image decoder fuzzed |
| 2 | **A document exists** | 3, 4, 5 | Texture sets, the layer stack with all blend modes, tile-scoped undo, mesh ingest, picking |
| 3 | **Materials compile** | 6, 7 | Node graph to WGSL/MSL/SPIR-V/HLSL, pass plans, the CPU reference executor, the parity gate |
| 4 | **A host can draw it** | 8 | Per-tile revisions, delta queries, tile readback; a `wgpu` reference host renders a document |
| 5 | **Painting works** | 9, 10 | Strokes, the full tool set, masking, seam dilation, preview equals commit |
| 6 | **It is usable end to end** | 11, 12 | Mesh maps and generators, the container format, export presets, padding |
| 7 | **It is reusable** | 13, 14, 15 | Smart materials, the three bindings at parity, the CLI |
| 8 | **It is honest about itself** | 16, 17, 18 | Examples as the end-to-end check, budgets on named devices, platform packages |

Milestone 4 is the one that validates the founding architecture. Until a real
host renders a real document through a pass plan and a delta query, "the library
owns no GPU device" is a claim rather than a result.

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

**2026-09-18 — Per-tile revisions are mandatory, not an optimization.** The
consequence of the previous decision: without them a host re-uploads whole
channels per dab and the architecture is slower than one that owned a device.
Design decision 8, capability `host-transport`.

**2026-09-18 — Undo is tile-scoped with a declared budget.** ArmorPaint's ring
of whole-texture snapshots is elegant and collapses to a single step at 16K,
which it handles by silently clamping the configured step count. We snapshot the
tiles a stroke touched and refuse to exceed a host-declared ceiling by name.
Design decision 4.

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

## Open questions

These are unresolved and should be answered by the task that first depends on
them rather than drifting.

1. **Tile size.** `texture-document` requires a documented tile size and does not
   pick one. It trades undo granularity against per-tile bookkeeping and against
   a host's upload efficiency. Decide with a measurement in task 3.9, not by
   taste.
2. **Parity tolerances.** `execution-backends` requires them stated per bit depth
   and for filtered values. The numbers do not exist yet; task 7.5 sets them, and
   setting them too loose makes the gate decorative.
3. **Reference devices.** `device-gate` requires at least one desktop and one
   tablet, named with full configuration. Which machines, and who owns them for
   CI, is not settled. Task 17.1.
4. **Kong's concurrency wrapper.** Design decision 3 says wrap its global state in
   a context object rather than snapshot and restore around every call as
   ArmorPaint does. Whether the vendored source tolerates that cleanly is
   unverified. Task 6.8; if it does not, the fallback is ArmorPaint's approach
   plus a serialization point, and that should be recorded here.
5. **Instance deletion policy.** `texture-document` allows either refusing the
   deletion of a referenced entry or converting its instances to independent
   copies, and makes it the caller's choice. Whether hosts actually want the
   choice, or whether one behaviour should simply be the rule, is open. Task 3.4.
6. **UV density normalization.** Absolute texels-per-unit or relative to the
   set's mean — CyberRemesherAndUV issue #89 raises the same question on the bake
   side. The two repositories should answer it identically.

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
