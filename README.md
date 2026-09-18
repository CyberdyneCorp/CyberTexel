# CyberTexel

Portable, headless **C++20 3D texture-painting and PBR material-authoring
engine** — the stage that begins where `sculpt -> retopo -> UV -> bake` ends.

Paint onto a model's UV space with a stroke engine that sweeps rather than
stamps; stack layers, groups, masks and filters across nine PBR channels with
twenty blend modes; author materials as a node graph that compiles to **shader
source** rather than to an interpreter; re-derive smart materials on a new model
from its mesh maps; and export channel-packed texture sets for any engine from a
preset that is pure data.

The library **does not own a GPU device** in its primary path. It emits shader
source and an ordered pass plan, and the host runs them on the device it already
has — so a Rust/`wgpu` desktop app, a Swift/Metal iPad app, a Python script and a
headless CLI all drive the same engine without interop.

**Status.** Pre-implementation. The specification lives in [`openspec/`](openspec/);
the founding change is
[`openspec/changes/bootstrap-v1-cybertexel/`](openspec/changes/bootstrap-v1-cybertexel/)
— proposal, design, sixteen capability specs and the task plan. Nothing is built
yet; `openspec/specs/` fills as the change is delivered and archived.

## Where it sits

| Repository | Owns |
|---|---|
| [ClayCore](https://github.com/CyberdyneCorp/ClayCore) | Form — SDF, voxel and mesh sculpting. Colour as a *field* property; explicitly not material authoring. |
| [CyberRemesherAndUV](https://github.com/CyberdyneCorp/CyberRemesherAndUV) | Topology and parameterization — quad remeshing, retopology, UVs, and **baking**. |
| **CyberTexel** | Appearance — what the surface *looks like*. Layers, materials, paint, export. |
| [ClaySpaceDesktop](https://github.com/CyberdyneCorp/ClaySpaceDesktop) | The desktop host that composes all three. |

There is **no build or link dependency** between CyberTexel and its siblings.
The seams are a format and an interface, following CyberRemesherAndUV's
`pipeline-bridge` precedent: CyberTexel defines the mesh-map set it consumes and
a provider interface; CyberRemesherAndUV implements the baking behind it.

## Capabilities

Sixteen, specified before any code exists:

| | |
|---|---|
| `texture-document` | Layers, groups, masks, filters, channels, blend modes, tile-scoped undo with a declared budget |
| `stroke-model` | Spacing, pressure and tilt, deterministic jitter, taper, stabilizer, constraints, symmetry |
| `paint-engine` | Texture-space rasterization, swept coverage, rejection tests, coverage accumulation, seam dilation |
| `paint-tools` | Brush, Eraser, Fill, Clone, Blur, Smear, Decal, Stencil, Projection, Text, Particle, Picker, Colour ID, Selection |
| `material-graph` | Node document, catalogue, groups, typing and coercion, validation |
| `shader-emission` | Graph and stack to WGSL/MSL/SPIR-V/HLSL plus an ordered pass plan |
| `execution-backends` | Host-executed, CPU reference and owned-GPU routes, with a parity gate |
| `mesh-and-texture-sets` | Mesh and UV ingest, UV sets, UDIM, atlases, mesh replacement |
| `mesh-maps` | The map set, the bake provider interface, generators |
| `smart-materials` | Smart materials and masks, anchor points, shelves, versioned presets |
| `color-management` | Working space, per-channel semantics, LUTs, bit-depth policy |
| `texture-export` | Channel-packing presets, formats, scopes, padding, reports |
| `project-io` | Container format, versioning, packing, autosave and recovery |
| `c-abi` | `ctex_*`, opaque handles, versioned descriptors, result codes |
| `language-bindings` | Python, Swift and Rust, held at parity with the C ABI |
| `build-packaging` | Layering, licence, test, determinism and ABI gates |

## Prior art

Two products define the problem, and one of them we may read.

**Substance Painter** (Adobe, closed) is the behavioural reference for texture
sets, mesh maps, smart materials, smart masks, anchor points and channel-packing
export presets. Public documentation only — no code, no assets, no format.

**[ArmorPaint](https://github.com/armory3d/armorpaint)** (zlib) is a working,
permissively licensed implementation of the hard half, and its behaviour is
specified in full at
`openspec/specs/` in that project. CyberTexel takes its architecture directly:
UV-as-clip-space rasterization so one fragment is one texel; a swept capsule
instead of dab spacing; blending against a stroke-start snapshot so every blend
mode works while painting; a per-stroke coverage mask so self-overlap does not
darken; extrapolating seam dilation; and a node graph that emits shader text.
Its **Kong** compiler (zlib) already targets WGSL — the language `wgpu` hosts
speak and the one ClayCore's own dialect does not reach.

Where CyberTexel deliberately diverges: **tile-scoped undo** with a declared
memory budget rather than a ring of whole-texture snapshots that silently
collapses to one step at 16K; **texture sets** as the binding unit rather than
per-layer object masks; **edit-time cycle detection** in the graph rather than a
forward reference that fails at shader-compile time; and **no owned renderer**.

## Licence

MIT. See [LICENSE](LICENSE). Third-party components are permissively licensed
and recorded in the attribution file; the dependency audit is a release gate.
