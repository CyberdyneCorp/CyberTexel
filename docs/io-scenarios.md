# Project I/O and texture export scenario coverage

`just test-project-io-scenarios` and `just test-texture-export-scenarios` run
the CTests carrying the corresponding scenario label. The `io-scenario-matrix`
test also compares this document with both OpenSpec capabilities, so adding or
renaming a scenario without recording executable evidence fails the suite.

Some project-container scenarios intentionally name a later cross-capability
task. That records the implemented container-side behavior and the missing
domain-side dependency separately; it does not claim that deferred behavior is
already present.

## Project I/O

| OpenSpec scenario | Executable evidence | Covered behavior |
| --- | --- | --- |
| Round trip | `project-container`, `standalone-asset`, `operation-record`, `c-abi-operation-record` | Tiled pixels, resources, standalone assets, versioned operation records with pinned inputs and raster checkpoints, and opaque sections round-trip exactly; complete layer/history/editable objects extend this schema with tasks 3.9 and 20.2–20.4 |
| Newer file on an older build | `project-container` | Unknown newer sections and tile/resource encodings are reported, retained and re-saved byte-for-byte |
| Probing a file | `project-container`, `version-consistency` | The fixed header exposes the shared schema version without decoding the body |
| Sparse document | `project-container`, `project-autosave` | A 16K image with isolated painted tiles serializes only those occupied tiles and exact edge extents |
| Sharing a project | `project-container`, `standalone-asset` | Packed image, font, map and mesh resources resolve without their external paths |
| Missing unpacked resource | `project-container`, `standalone-asset` | Opening succeeds, missing identifiers are reported and dependent standalone installation is refused atomically |
| Interrupted save | `project-container` | Failed serialization/publication and an incomplete sibling temporary file leave the prior project intact |
| Recovering after a crash | `project-autosave`, `c-abi-project-autosave` | Restart discovery enumerates the newest atomically published recovery document, preserves operation records and required raster checkpoints, reports unsupported replay versions, and rejects malformed candidates |
| Autosave does not block | `project-autosave` | Capture pins copy-on-write tile versions while later painting continues; compression and publication run on the worker |
| Reopening after the mesh moved | `project-container` | A missing referenced mesh is reported through the resource identity; accepting a replacement is completed by task 4.7 |
| Importing a material file | `standalone-asset` | A material package installs into a library without replacing the open project and refuses partial dependency installation |
| Sharing a smart material | `standalone-asset` | Every asset kind, including smart material, exports self-contained with its transitive pixel/resource dependencies and imports without external files |
| Before saving a large document | `mesh-maps`, `project-autosave` | Per-texture-set channel/map bytes and snapshot-retained pixel bytes are queryable without saving; history and full backing/encoded estimates complete with tasks 3.9 and 19.1 |
| Declared size exceeds the file | `project-container` | Truncated declared section, record and payload sizes are rejected before declared-size allocation |
| Fuzzing gate | `just fuzz-project-container` | A deterministic 20,000-input libFuzzer campaign runs with ASan/UBSan and bounded parser allocations |
| Reproducible save | `project-container-determinism` | Two clean atomic saves of unchanged sparse content are byte-identical |
| Older reader lacks a replay algorithm | `project-container`, `operation-record`, `project-autosave`, `c-abi-project-autosave` | Unknown versioned content stays opaque; recovered operation records report unavailable algorithms without substitution and retain usable raster checkpoints |

## Texture export

| OpenSpec scenario | Executable evidence | Covered behavior |
| --- | --- | --- |
| A new engine convention | `export-presets` | A caller-created data-only preset validates without engine registration or target-specific code |
| Packed ORM | `texture-export-execution` | One encoded texture places occlusion, roughness and metallic in RGB |
| Specular-glossiness export | `texture-export-execution`, `export-presets` | Encoded diffuse/specular outputs use the documented dielectric derivation and smoothness inversion |
| Default export | `export-presets`, `texture-export-execution` | The individual PBR preset is the default and its identifier appears in the report |
| 32-bit request to PNG | `texture-export-formats` | The impossible format/depth pair is rejected before bytes are returned |
| Per-tile export | `texture-export-plan` | Every occupied UDIM becomes one planned output carrying its tile number in the path |
| Exporting one group | `texture-export-plan` | Selecting a group includes its complete descendant closure in the flattened unit |
| Predictable names | `texture-export-plan` | Default paths differ by texture-set identity and expand every documented token deterministically |
| Pattern would collide | `texture-export-plan` | Case-insensitive path collisions name both outputs and abort the complete plan |
| Downscaling on export | `texture-export-execution` | Independent output dimensions use pixel-centred bilinear resampling |
| Padding prevents seams | `texture-export-execution` | Export calls the shared paint gradient extrapolator and zero disables padding |
| Checking before writing | `texture-export-execution` | Dry run returns every path, dimension, format and estimate without requesting pixels or returning buffers |
| Pipeline consumption | `texture-export-report-json` | Valid deterministic JSON contains path, texture set, preset entry, resolution, format, colour space and depth for every output |
| Cancelled export | `texture-export-execution` | Cancelling a twenty-output export after eight retains exactly eight complete buffers and marks the remaining twelve unstarted |
| Host wants pixels | `texture-export-execution` | Encoded caller-owned buffers carry path, dimensions, format, colour space and bit depth without filesystem I/O |
| Exporting coat weight | `texture-export-execution`, `export-presets` | A registered scalar component exports unchanged beside an sRGB-transferred colour component |
