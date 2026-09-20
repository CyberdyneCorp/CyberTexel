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
normalization does not claim tangent-basis compatibility.

Every tangent-space normal additionally carries the complete
`mesh::TangentFrameDescriptor`. The map set also retains the frame of its bound
mesh (or an explicit host declaration when only a mesh revision is available).
Binding is refused unless algorithm and version, normal orientation,
coordinate-system handedness, signed-W encoding, UV V-axis convention and UV
set match exactly. A missing frame on either side is also refused. This check
happens independently of OpenGL/DirectX green-channel normalization, so a
green flip can never disguise an incompatible basis.

The descriptor is the single frame contract used by tangent normal sampling,
normal composition and height-to-normal derivatives. The mesh's per-corner W
sign remains part of preview reconstruction and exported vertex data; it is not
collapsed into a texture-wide flag. `MeshMapSet` created from a `MeshBinding`
adopts its validated frame automatically. Providers receive that frame in
`BakeRequest` and must return the frame actually used in `BakeProviderOutput`;
external imports declare it in `ExternalMeshMapImport`.

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

## Memory accounting and release

`MeshMapSet::memory_report` lists every bound map's dimensions and resident tile
pixel bytes. Binding and replacement update a texture-set memory account
transactionally, so `TextureSet::memory_report` and
`TextureDocument::memory_report` separate channel pixels from mesh-map pixels
and also provide their combined resident total. Multiple live map sets for one
texture set contribute independently, and destroying a map set removes its
account automatically.

`release_map` is an idempotent selective release; `release_all_maps` returns the
stable list of every binding removed. Both report the resident bytes removed
from document accounting. Release drops only the library's bindings: the
texture document, texture set and channel pixels remain valid. A later consumer
of a released map receives the normal structured missing-map error rather than
a dangling read or neutral value. Pixel storage is treated as immutable while
bound, matching the `shared_ptr<const TiledImage>` binding contract.

The public C boundary exposes the same external-import, sampling, requirement
preflight, revision synchronization, inventory, memory accounting and release
contract through `ctex_mesh_map_set_*`. Imported strided buffers are copied
before return. The document and mesh are non-owning parents of the opaque map
set and therefore outlive it; separate map sets may be used concurrently only
when their parent documents and meshes are also distinct.

The generator catalogue and execution path cross the same boundary through
`ctex_mesh_map_generator_get_info` and `ctex_mesh_map_generator_generate`.
Caller-owned outputs include the float mask, packed parameter names and
meanings, resolved values, clamp reports, stale-map identities and diagnostic
text. A missing required map returns `CTEX_RESULT_MISSING_RESOURCE`; no mask is
published as a neutral substitute.

The C mesh boundary also accepts a declared tangent frame plus one signed
tangent per triangle corner and reports whether a mesh retained supplied data
or generated the pinned default. Normal-map import compares the whole reported
frame, so a green-channel convention cannot conceal an incompatible basis.

## Bake-provider seam

`BakeProvider` is an optional synchronous callback table with an opaque context,
a capability query and a request callback. Requests name the map, texture-set
identity, UV layout, mesh revision, bake-settings revision, request generation
and requested resolution. The provider returns a borrowed strided pixel-buffer
view; CyberTexel validates and copies it
before the callback returns, then binds it transactionally. Provider memory
never becomes document-owned memory by accident.

The C equivalent is `ctex_mesh_map_set_request_bake`. Its callback descriptors
carry the same identities, progress and cancellation contract, and provider
callbacks execute synchronously on the calling thread. The provider is not
retained and no baker is linked into CyberTexel.

`ctex_mesh_map_bake_session_*` exposes the asynchronous state machine without
retaining a provider. Allocator-owned request tokens make the session, mesh,
texture set, UV set, bake-settings revision and request generation inspectable.
Completion binds only a current token and reports `bound`, `stale`, `cancelled`,
`invalid_output` or `unknown_token`. Settings edits and their accepted map
replacements form one undo step, and undo invalidates pending completions before
restoring the exact prior binding snapshot.

`BakeControl` carries C-style progress and cancellation callbacks. CyberTexel
reports initial zero progress, forwards valid monotonic provider progress below
one, and reports one only after validated output is bound. Existing or observed
cancellation publishes no map. Unsupported requests do not invoke the request
callback and return a diagnostic naming both the requested map and the complete
advertised set. Provider failure, invalid progress and malformed output likewise
leave existing bindings unchanged. The callback arguments and returned image
view are valid only for the synchronous call. Normal-map output must include its
OpenGL/DirectX declaration. `BakeRequestVersion` lets a host forward its real
settings revision and request generation through this synchronous adapter.

## Asynchronous bake publication and undo

`AsyncBakeSession` separates request issue from result publication. Each
`BakeRevisionToken` owns the issuing-session identity, requested map and
resolution plus the texture-set, UV, mesh, tangent-frame, settings-revision and
monotonically increasing request identities captured at issue time.
`bake_request_view` exposes those owned fields through the provider callback's
borrowed `BakeRequest` shape while the token remains alive. Starting another
request for the same map supersedes the previous generation. A completion is
copied and bound only when
the complete token still matches the session and `MeshMapSet`; cancelled,
superseded, settings-stale and mesh-stale results return an explicit disposition
without changing any binding. Altered, duplicate and foreign tokens are
reported as unknown.

The referenced `MeshMapSet` must outlive its session. Session methods serialize
the logical request lifecycle; hosts synchronize calls made from worker threads.

`edit_settings` creates one undo step by retaining the settings revision and an
immutable snapshot of every current map binding, then invalidates outstanding
requests from the previous settings. Any map replacements accepted under the
new revision remain part of that same step. `undo_settings_edit` restores the
prior settings revision and complete binding snapshot atomically and invalidates
all requests still pending for the undone state. Their later completions are
therefore stale and cannot republish the undone pixels. Snapshot pixels use
shared immutable ownership; restoring a snapshot revalidates every descriptor
before changing the live set.
