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
