# CyberTexel example gallery

These artifacts are produced by the numbered Python examples and compared with
the committed references by `just examples`. Binary and report outputs require
an exact byte match. PNG previews are compared as decoded pixels using the
maximum absolute channel error declared in `examples/output_tolerances.json`.

## `01_version_and_project_container`

Creates a canonical empty `.ctex` project through the generated Python C ABI.
It covers `build-packaging`, `c-abi`, `examples`, `language-bindings` and
`project-io` and publishes the
[empty project](../examples/outputs/01_version_and_project_container/empty.ctex)
and its
[container summary](../examples/outputs/01_version_and_project_container/summary.json).

## `02_image_io`

Decodes the checked-in color fixture, asserts named pixels and channel means,
enlarges it with NumPy, then encodes and decodes the preview through CyberTexel.
It covers `image-io` and `language-bindings`. Its declared pixel tolerance is
zero.

![Red and slate checker preview](../examples/outputs/02_image_io/checker_preview.png)

The machine-readable [image summary](../examples/outputs/02_image_io/summary.json)
records the source and preview shapes and asserted channel means.

## `03_mesh_map_generator`

Loads the checked UV quad into a texture document, imports the ambient-occlusion
fixture as a mesh map and evaluates the built-in mask generator at the texture
set resolution. It covers `mesh-and-texture-sets`, `mesh-maps` and
`texture-document`.

![Generated ambient-occlusion mask](../examples/outputs/03_mesh_map_generator/ambient_occlusion_mask.png)

The [generator summary](../examples/outputs/03_mesh_map_generator/summary.json)
records mesh counts, source/output resolutions and asserted mask statistics.

## `04_software_host_execution`

Emits a real WGSL material and pass plan, validates the render target and draw
contract, executes the constant material with a display-free software stand-in,
then publishes the host-resident generation with recovery evidence and no pixel
readback. It covers `execution-backends`, `host-transport`, `material-graph`,
`resource-residency` and `shader-emission`.

![Software-host material preview](../examples/outputs/04_software_host_execution/material_preview.png)

The committed [pass plan](../examples/outputs/04_software_host_execution/pass_plan.json)
and [execution summary](../examples/outputs/04_software_host_execution/summary.json)
make the resource contract and residency result inspectable.

## `05_color_management`

Queries the working space and normal-channel precision policy, converts a full
sRGB ramp to linear Rec. 709, accumulates height before quantization and renders
the deterministic ordered-dither pattern. It covers `color-management`.

![sRGB ramp, linear ramp and ordered dither](../examples/outputs/05_color_management/transfer_and_dither.png)

The [colour summary](../examples/outputs/05_color_management/summary.json)
records the reference midpoint, precision recommendation and quantized values.

## `06_uv_picking`

Builds the deterministic UV acceleration index, queries one hit on each quad
triangle, asserts interpolated positions and barycentrics, and verifies that an
outside coordinate is a distinct miss. It covers `picking`.

![UV diagonal with one hit on each triangle](../examples/outputs/06_uv_picking/uv_hits.png)

The [picking summary](../examples/outputs/06_uv_picking/summary.json) records the
resolved texture-set identity, triangle indices and world positions.

## `07_editable_operation_record`

Creates a resolution-independent brush operation with a deterministic seed,
pinned brush-alpha bytes and channel metadata, inserts it into a canonical
project, extracts it and requires byte-identical recovery. It covers
`editable-authoring`. Checkpoint-bearing records additionally require their
named tiled image to exist in the project, as enforced by the native API.

The gallery retains the canonical [operation record](../examples/outputs/07_editable_operation_record/stroke.operation),
[project container](../examples/outputs/07_editable_operation_record/editable.ctex),
[record inventory](../examples/outputs/07_editable_operation_record/record_report.json),
[project inventory](../examples/outputs/07_editable_operation_record/project_report.json)
and [round-trip summary](../examples/outputs/07_editable_operation_record/summary.json).

## `08_brush_stroke`

Resolves three timestamped surface samples into a continuous swept stroke,
rasterizes its geometric coverage and non-building deposition, then applies an
orange base-colour material to the stroke-start snapshot through the atomic
brush tool. It covers `stroke-model`, `paint-engine` and `paint-tools`.

![Resolved brush stroke](../examples/outputs/08_brush_stroke/brush_stroke.png)

The [paint summary](../examples/outputs/08_brush_stroke/summary.json) records the
resolved stamp and segment counts together with coverage and write statistics.

## `09_end_to_end_material_export`

Carries the UV fixture and its ambient-occlusion map through a texture set and
generated mask, embeds a canonical material graph in a smart material, applies
the material as a document layer, and exports the shaded base colour through the
native texture-export pipeline. It covers `smart-materials` and `texture-export`
and is the worked end-to-end pipeline example.

![Exported smart material](../examples/outputs/09_end_to_end_material_export/fixture_body_BaseColor.png)

The [pipeline summary](../examples/outputs/09_end_to_end_material_export/summary.json)
records the material application, model size, generated filename and decoded
output shape.

## `10_headless_script_pipeline`

Creates a small project, invokes the installed wheel's isolated headless script
runner, verifies that script output is diagnostic-only, and reopens the result
to prove the roughness-channel edit persisted. It covers `cli-headless`.

The gallery retains the edited [project container](../examples/outputs/10_headless_script_pipeline/scripted.ctex)
and the checked [headless summary](../examples/outputs/10_headless_script_pipeline/summary.json).

## `11_device_gate_policy`

Exercises the `device-gate` decision vocabulary with deterministic policy
fixtures: a named reference result may pass or fail, while an unnamed machine,
an absent device and a value below the absolute floor remain informational,
unmeasured and unreachable. It explicitly makes no hardware performance claim.

The [policy summary](../examples/outputs/11_device_gate_policy/summary.json)
records all five asserted decisions and the native version used.

## `12_host_transport_readback`

Paints two texels, asks what changed since a revision cursor, pins the answer as
a budgeted snapshot, reads the tile memory layout the host must upload into, and
completes an asynchronous readback with exactly those tile payloads. A pending
readback publishes nothing, and an unknown channel raises a typed exception
carrying the native diagnostic code. It covers `host-transport`, `paint-engine`,
`texture-document` and `c-abi`, and needs no device.

The [transport summary](../examples/outputs/12_host_transport_readback/host_transport.json)
records both revision cursors, the tile count and byte sizes, the refused
pending readback and the diagnostic code.
