# Mesh maps

`ctex::maps::MeshMapSet` owns the map bindings for one texture set and its
named UV layout. The stable inventory is tangent-space normal, object-space
normal, world-space direction, ambient occlusion, curvature, thickness,
position, height, bent normal, material ID, object ID, UV density and vertex
colour.

Every binding names its texture-set identity and UV set. A mismatch is refused
before the set changes. Normal, direction and position maps require three
channels; scalar and identifier maps require one; vertex colour accepts three
or four. Storage can use 8-bit normalized, 16-bit normalized or 32-bit float
channels through `ctex::image::TiledImage`.

Map resolution is independent of the texture-set resolution. `bind` accepts a
different resolution and returns one `MapResolutionMismatch` in that bind's
result; later sampling does not repeat the report. Rebinding a kind replaces its
previous pixels and reports that disposition.

`sample` accepts finite normalized UV coordinates and returns the sampled value
plus any `MeshMapStaleness`. Continuous maps use bilinear filtering, with UV
`(0, 0)` at the bottom-left and edge clamping implicit at the inclusive
normalized boundary. Material-ID and object-ID maps use nearest sampling so
filtering cannot create identities that were never stored. Missing maps and
malformed bindings are refused by name instead of returning neutral values.

Consumers that know their complete inputs call `check_required_maps` before
execution. Its structured `MeshMapRequirementReport` carries the consumer and
texture-set identities, a deduplicated stable list of every absent map and an
English diagnostic naming them. `require_maps` turns the same report into a
`MissingMeshMapsError`; direct `map` and `sample` reads use that typed error too.
A satisfied report has no diagnostic. A failed check never binds placeholder
pixels, changes existing bindings or returns a neutral sample, so generators,
smart masks and materials can expose the failure without silently flattening
their result.

Every binding records the `mesh::MeshRevision` from which its pixels were
produced. The map set records the current revision and
`synchronize_mesh_revision` compares those identities after mesh replacement,
accepting either the binding itself or its revision and returning all stale
bindings without erasing them. Binding an already stale map reports that
condition immediately. `stale_maps`, requirement preflight and every successful
sample all carry the producing and current revisions, so a host can warn while
still displaying or inspecting the retained pixels. A map becomes current only
when it is explicitly replaced by pixels produced from the current mesh
revision.

## External map import

`import_external_mesh_map` accepts caller-owned strided pixel buffers without a
bake provider. Both `channel_meaning` and `color_space` are required concrete
declarations; omission, unknown values or a meaning incompatible with the map
kind are refused before publication. Meanings distinguish scalar data, XYZ
normal/direction/position vectors, identifiers and RGB/RGBA colour. The import
copies the buffer and then uses the same `MeshMapSet::bind` path as provider
output, so texture-set/UV validation, mesh revision staleness and resolution
mismatch reporting are identical and failures leave existing bindings intact.

RGB and RGBA vertex-colour imports declared as sRGB are converted to the linear
Rec. 709 working space before binding, with alpha preserved. Numeric data,
vectors and identifiers are never transfer-function transformed; their colour
space is still declared and returned in `ExternalMeshMapImportResult`, alongside
the canonical storage space and whether conversion occurred. Caller memory can
be released or changed as soon as the synchronous import returns.

## Normal-map convention

Every tangent-space, object-space and bent-normal binding must declare
`NormalMapConvention::open_gl` or `NormalMapConvention::direct_x`; omitting it,
using an unknown value or attaching one to a non-normal map is refused before
the binding changes. Provider output and external import carry the same
declaration, and the source convention remains queryable on
`MeshMapDescriptor`.

CyberTexel exposes OpenGL-style encoded XYZ as its single downstream
convention. Sampling a DirectX normal changes only the encoded green component
to `1 - green`; red, blue, alpha, source pixels and the recorded declaration are
unchanged. Conversion occurs on read after filtering, which is algebraically
equivalent to converting each texel before linear filtering. This convention
normalization does not claim tangent-basis compatibility: tangent algorithm,
orientation and mirrored-handedness validation remain roadmap task 11.12.

## Generators

`mesh_map_generator_info` exposes the stable eight-generator inventory and the
complete mesh-map inputs for each generator. This lets a host preflight or
request baked inputs before evaluation. `generate_mesh_map_mask` then evaluates
the selected generator at pixel centres into a caller-sized, one-channel
32-bit-float `TiledImage`. Missing inputs raise `MissingMeshMapsError` with the
generator name and the complete missing-map list. Stale inputs remain usable,
with their `MeshMapStaleness` entries returned in the result.

The baseline interpretations are:

| Generator | Required maps | Baseline mask |
| --- | --- | --- |
| Ambient occlusion | ambient occlusion | `1 - AO` |
| Curvature | curvature | curvature value |
| Thickness | thickness | `1 - thickness` (thin regions) |
| Position gradient | position | encoded world-space Y |
| World-space direction | world-space direction | encoded +Y alignment |
| Dirt | ambient occlusion, curvature | maximum of occlusion and concavity |
| Edge wear | curvature | positive curvature above encoded neutral `0.5` |
| Scratches | position, world-space direction | narrow world-position stripes modulated by grazing orientation |

Every result is saturated to `[0, 1]`. Every generator exposes these common
parameters:

| Parameter | Default | Range | Meaning |
| --- | ---: | ---: | --- |
| `strength` | 1 | `[0, 2]` | Multiplier applied to the generated mask |
| `contrast` | 1 | `[0.1, 4]` | Positive power applied before strength |

The specialised parameters are:

| Generator | Parameter | Default | Range | Meaning |
| --- | --- | ---: | ---: | --- |
| Dirt | `curvature-weight` | 1 | `[0, 1]` | Contribution of concave curvature |
| Edge wear | `threshold` | 0.5 | `[0, 0.99]` | Encoded curvature where wear begins |
| Scratches | `scale` | 1 | `[1, 128]` | World-position frequency |
| Scratches | `width` | 1/24 | `[0.001, 0.25]` | Scratch half-width in phase space |

Callers may omit any parameter to use its declared default. Supplied values
must be finite; empty, duplicate and unknown names are refused. Values outside
their range are clamped, used in evaluation and returned in
`MeshMapGeneratorParameterReport`, which also carries the complete resolved
parameter set for deterministic replay.

The CPU output defines correctness and is byte-stable for repeated evaluation.
The committed generator fixture evaluates every generator with non-default
parameters through both the CPU route and an independent portable-host formula,
then uses the standard filtered floating-point executor tolerance to detect
drift.

## Bake-provider seam

`BakeProvider` is an optional synchronous callback table with an opaque context,
a capability query and a request callback. Requests name the map, texture-set
identity, UV layout, mesh revision and requested resolution. The provider
returns a borrowed strided pixel-buffer view; CyberTexel validates and copies it
before the callback returns, then binds it transactionally. Provider memory
never becomes document-owned memory by accident.

`BakeControl` carries C-style progress and cancellation callbacks. CyberTexel
reports initial zero progress, forwards valid monotonic provider progress below
one, and reports one only after validated output is bound. Existing or observed
cancellation publishes no map. Unsupported requests do not invoke the request
callback and return a diagnostic naming both the requested map and the complete
advertised set. Provider failure, invalid progress and malformed output likewise
leave existing bindings unchanged. The callback arguments and returned image
view are valid only for the synchronous call. Normal-map output must include its
OpenGL/DirectX declaration. Revisioned asynchronous requests are reserved for
roadmap task 11.13.
