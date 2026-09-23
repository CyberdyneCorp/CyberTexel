# C ABI foundations

The stable host boundary is the C header `ctex/capi.h`. Every exported symbol
uses the `ctex_` prefix and the shared `cybertexel_c` library applies a platform
export map that hides every other symbol. The regular `cybertexel` static target
remains available for native static linkage. iOS consumes that static target.

Library objects are represented by incomplete C types. A caller creates a
`ctex_document*` with `ctex_document_create` and releases it exactly once with
`ctex_document_destroy`; the structure behind that pointer is private and may
change without changing the ABI. Destroying a null handle is permitted.

Every fallible entry point returns `ctex_result`. Zero is success. The stable
nonzero categories distinguish invalid arguments, missing resources,
unsupported operations, allocation failure, budget refusal, cancellation and
unexpected internal failures. C++ exceptions are caught by the C boundary and
translated to one of those values.

Bulk results use caller-owned buffers and the same two-call contract. A null
buffer with size zero reports the exact required size without writing data. A
non-null buffer that is too small returns `CTEX_RESULT_BUFFER_TOO_SMALL`, reports
the required size, and leaves the complete buffer untouched. A right-sized
second call fills it completely. `ctex_document_get_texture_set_ids` applies
this contract to the ordered stable texture-set identities: each UTF-8 identity
is NUL-terminated and entries are packed consecutively; `out_count` reports how
many entries are present.

Input descriptors begin with a `uint32_t size`. Callers set it to the descriptor
size they compiled against; the library rejects values below the required prefix
or above its own current structure before reading any later field. Fields are
read only when `size` covers the complete field. The first
`ctex_texture_set_descriptor` prefix ends before `default_bit_depth`, so an older
caller receives the documented 8-bit default even if bytes beyond its declared
prefix contain another value. Current callers use
`CTEX_TEXTURE_SET_DESCRIPTOR_CURRENT_SIZE`; the stable older prefix is exposed
as `CTEX_TEXTURE_SET_DESCRIPTOR_V1_SIZE`. Descriptor strings are borrowed for
the duration of `ctex_document_create_texture_set` and copied into the document.

## Mesh ingest

`ctex_mesh_create` accepts positions, normals, triangle indices, at least one
named UV set, optional vertex colours, total face partitions and material IDs
entirely from memory. It never writes caller buffers. The resulting opaque
handle owns allocator-routed copies, so arrays and strings need only remain valid
for the call. `ctex_mesh_get_uv_set_names` enumerates all names with the packed
two-call buffer contract; at least four UV sets are supported, and
`ctex_mesh_get_info` reports attribute counts, partition count and revision.

`ctex_mesh_analyze_uv_overlaps` queries one named UV set and texture-set
partition without changing the mesh. It reports unique sorted face indices for
positive-area overlaps through the two-call array contract. Boundary-only
contact and degenerate UV triangles are excluded. Candidate and confirmed-pair
counts expose the deterministic broad-phase work rather than hiding a scan
behind the result.

`ctex_mesh_analyze_uv_coverage` samples a non-UDIM partition at the requested
texture-set resolution. It returns covered and uncovered texel counts, the
normalized uncovered area, actual texel tests and sorted faces leaving the unit
square. Outside-face output uses the two-call array contract. The public
`CTEX_MAX_MESH_UV_COVERAGE_SAMPLES` ceiling is checked before allocation.

`ctex_mesh_replace` validates and copies a complete replacement before publishing
it. Success advances the globally unique revision; failure preserves both the
prior mesh and revision. The maximum supported mesh has 100,000,000 vertices and
100,000,000 triangles, exposed as `CTEX_MAX_MESH_VERTEX_COUNT` and
`CTEX_MAX_MESH_TRIANGLE_COUNT`. Both boundaries reject larger declared counts
before reading array contents and report `CTEX_DIAGNOSTIC_MESH_LIMIT_EXCEEDED`
with the count, supplied value and maximum.

For a painted document, use `ctex_mesh_replacement_plan_create` instead of the
mesh-only replacement call. The opaque plan owns a validated copy of the
candidate mesh. Its two-call info query reports each stable texture-set ID,
source and replacement partition, face counts, and whether its named UV layout
is unchanged, changed, or missing. Unchanged sets need no decision; every other
set requires exactly one keep, reprojection, or clear policy at apply time.

Keep and clear policies publish atomically with the candidate mesh, while stale,
missing, duplicate, or unknown decisions publish nothing. If any texture set
requests reprojection, apply succeeds as a preflight with
`replacement_applied == 0`: neither mesh nor pixels change and the plan can be
queried or submitted again. Run
`ctex_mesh_replacement_plan_preflight_reprojection` with explicit distance,
normal-angle, visibility, ambiguity and work limits. Its two-call JSON result
lists every mapped, unmapped or ambiguous destination texel and affected
editable entry. Cancellation and budget exhaustion preserve the original
document. Commit supplies explicit hole and ambiguity policies, transforms
normal-vector channels between tangent bases, rejects stale source revisions,
and publishes pixels, editable attachments and the replacement mesh together.
The document and mesh must outlive the plan. The `_with_tangent_data`
constructor provides the same transaction for a declared per-corner tangent
frame.

`ctex_document_create_texture_sets_from_mesh` selects any named UV set on a mesh
and creates one document texture set for every validated face partition, using
the requested resolution and precision. The operation refuses a missing UV set
before changing the document. Created stable identities can be retrieved with
`ctex_document_get_texture_set_ids` and retain the selected UV name.

## Picking

`ctex_pick_index_create` and `ctex_uv_pick_index_create` build reusable CPU
acceleration structures over an existing mesh. The mesh is non-owning and must
outlive its indexes. A query detects `ctex_mesh_replace`, rebuilds before use and
reports the indexed revision, build count, visited nodes and tested leaf
triangles. Persistent index arrays and UV names use the allocator captured when
the index was created.

`ctex_pick_ray_from_screen` supports perspective and orthographic matrices.
`ctex_pick_ray_query` selects nearest or ordered all-hit occlusion and accepts or
rejects backfaces per call. Counter-clockwise vertices define the front when
viewed from the geometric-normal side; equivalent shared-boundary hits choose
the lowest triangle index. Each hit contains position, both normals, UV,
texture-set identity, UDIM tile, triangle, barycentrics, material and distance.
A zero result count is a successful miss.

`ctex_pick_uv_query` performs the inverse surface lookup,
`ctex_pick_snap_to_surface` finds the nearest surface within a distance, and the
four `ctex_pick_query_*` region calls cover screen rectangles, screen lassos,
world spheres and world boxes. Result arrays use the null-buffer sizing and
atomic short-buffer rules. Texture-set identities occupy a separate packed
NUL-terminated buffer addressed by each hit's offset and size.

`ctex_pick_nearest_batch` returns one `ctex_pick_hit` per ray; `has_hit == 0`
marks a non-error miss. Its optional control descriptor sets a logical memory
ceiling, cancellation callback and progress interval. Cancellation returns no
partial batch as complete, while `ctex_pick_batch_info` retains the status,
processed count, required memory and aggregate traversal cost. Callbacks execute
on the calling thread, carry the descriptor's user-data pointer and must not
throw across the C boundary; a nonzero cancellation result requests a stop.

## Texture-set channels

Every new texture set registers the nine metallic/roughness preset channels but
leaves them disabled. `ctex_texture_set_get_channel_ids` enumerates their stable
semantic identifiers with the same packed two-call buffer contract used for
texture-set identities. `ctex_texture_set_register_channel` adds an extensible
descriptor without imposing a fixed channel-slot limit. Its semantic ID,
component count, scalar representation, preferred precision, default value,
colour/data classification, blending policy, export mapping and evaluable flag
are copied into document-owned storage.

`ctex_texture_set_set_channel_enabled` enables or disables one registered
channel. A zero precision override selects the texture-set default; 8, 16 or 32
selects an explicit per-channel precision. Disabled channels have no image
storage. `ctex_texture_set_get_channel_info` returns the complete numeric
descriptor and current enabled/storage state while copying the export mapping
into a caller-owned buffer. `ctex_texture_set_get_memory_report` reports enabled
channel count and resident channel, mesh-map and total bytes. Enabled constant
channels remain sparse and therefore report zero resident pixel bytes until a
write materializes a tile.

The current `ctex_texture_set_descriptor` appends `udim_tiling`; older prefixes
default it off. UDIM sets expose sorted logical occupancy through
`ctex_texture_set_get_udim_tiles`, and hosts can predeclare mesh occupancy with
`ctex_texture_set_ensure_udim_tiles` without allocating channel pixels.
`ctex_texture_set_write_udim_pixels` accepts one atomic batch of absolute UV
coordinates, applies standard `1001 + u + 10 * v` addressing and reports changed
and newly allocated tile counts. `ctex_texture_set_read_udim_pixel` uses the
normal two-call sizing contract. Invalid coordinates or pixel layouts are
refused before any tile in the batch changes.

`ctex_document_create_atlas` groups existing texture sets into validated,
non-overlapping regions. Atlas identities use the packed two-call enumeration
contract, while `ctex_document_get_atlas` returns dimensions, ordered regions,
the display name and packed texture-set identities through caller-owned buffers.
Missing sets, repeated membership, out-of-bounds regions and overlaps are
refused before the document changes.

## Texture-set layer stacks

`ctex_texture_set_layer_append` accepts an ordered batch of explicit paint,
fill, group, mask, filter, instance, editable-decal, editable-text and
surface-path entries. The complete candidate stack is validated before it is
published, so invalid evaluation order, nesting, attachments, instance sources,
cycles, blend modes or channel modulation leave the stack unchanged.
`ctex_texture_set_layer_inspect` returns a canonical JSON snapshot with the
stack revision, explicit kinds, relationships, state, content revisions and
per-channel modulation.

Hosts can update entry state, layout and channel modulation without replacing
stable identity or content. `ctex_texture_set_layer_record_paint` advances paint
content revision and refuses direct instance painting with a diagnostic naming
the source. Source deletion explicitly selects refusal or conversion of live
instances to independent content. Applicable masks use packed identity
enumeration; a participation query then requires exactly one finite normalized
sample per applicable enabled mask and returns the flattened effective opacity.
Blend evaluation uses the same authoritative linear-working-space formulas as
document compositing.

`ctex_texture_set_layer_composite_cpu` is the device-free reference compositor.
Callers supply explicit normalized content, optional coverage and mask rasters;
the document supplies bottom-to-top order, instances, filters, channel policy
and group scope. The two-call output contract returns ordered channel metadata,
packed semantic identities and flattened caller-owned pixels. Ordinary groups
isolate and blend once, while Pass Through groups evaluate their children into
the enclosing accumulator. Missing, duplicate, unknown, mismatched, non-finite
or unevaluable inputs are refused before any output buffer changes, and repeated
unchanged calls are bit-identical.

`ctex_layer_snapshot_create` copies those resolved content, coverage and mask
rasters into an allocator-routed opaque handle. `ctex_layer_snapshot_read`
uses the same two-call contract to return dimensions, offsets and flattened
caller-owned arrays without exposing internal storage. Snapshot creation checks
descriptor structure and ownership requirements; semantic validation against a
particular layer stack occurs when the snapshot is composited or used by an
operation. `ctex_texture_set_layer_composite_snapshot_cpu` evaluates the owned
snapshot with the same deterministic compositor and output guarantees. Snapshot
handles remain valid independently of the borrowed creation arrays and must be
destroyed with `ctex_layer_snapshot_destroy`.

`ctex_texture_set_apply_layer_operation` exposes create, duplicate, delete,
reorder, reparent, clear, invert, merge down, merge group, flatten,
paint/fill conversion and apply-mask through one versioned descriptor. The
descriptor supplies the owned snapshot, any baked replacement rasters, the
output-byte ceiling and appearance tolerance required by the core operation.
Calling with a null affected-ID buffer validates the complete candidate and
returns its packed-ID size without committing. A second call with sufficient
storage atomically publishes both the layer stack and the updated snapshot;
invalid operations, appearance mismatches, budget failures and short buffers
leave both unchanged. Create and paint-to-fill conversion accept a canonical
serialized graph. Delete always carries an explicit live-instance policy.

`ctex_texture_set_begin_transaction` copies an owned snapshot and opens an
isolated texture-set candidate over a declared tile write set. Pixel writes,
layer operations, state/layout/channel edits and fill-graph replacement affect
only that candidate. Commit checks the live tile, layer and history revisions,
then publishes every changed tile, the layer stack and the updated snapshot as
one undo step. A stale or failed commit consumes the transaction and publishes
nothing. Cancel, destruction without commit and every failed staged operation
leave the document and caller snapshot unchanged. The document, texture set and
snapshot must outlive the transaction handle.

Layer-only commits retain command records and report zero tile count and zero
pixel-history bytes. This applies to renaming, reorder/reparent, opacity, blend
mode, channel modulation and graph edits. Transaction handles use the document's
allocator and must be destroyed even after commit or cancel.

Tile history has an explicit byte ceiling and reports retained and available
bytes, proposed-step capacity, and undo/redo counts. A capture declares unique
channel/tile targets before an operation; it can wrap an ordinary paint-preview
commit, and its final commit retains only targets whose generation changed.
Capture handles use the configured host allocator and must be destroyed; a
commit consumes its capture even when it reports an error.

Undo and redo exchange exact tile storage owners and report exchanged storage
and copied pixel bytes. Empty directions return `CTEX_RESULT_NO_UNDO` or
`CTEX_RESULT_NO_REDO`, stale live state returns `CTEX_RESULT_STALE_STATE`, and
oversized capture admission returns `CTEX_RESULT_OVER_BUDGET`. Committing a new
step after undo releases the redo side immediately without exceeding the
declared ceiling.

## Mesh maps

`ctex_mesh_map_set_create` binds an explicit map set to one document texture set
and the current revision and tangent frame of one mesh. The document and mesh
must outlive the map-set handle. Stable map names, texture-set identity, UV set,
dimensions, revision, bound entries, conventions and resident pixel bytes are
available through caller-owned two-call queries.

Hosts may use `ctex_mesh_create_with_tangent_data` and
`ctex_mesh_replace_with_tangent_data` to supply one signed tangent per triangle
corner with the full basis version and coordinate conventions. Ordinary mesh
creation uses the documented generated frame. `ctex_mesh_get_tangent_frame`
reports which route produced the frame and its complete descriptor, including
the UV set; mirrored handedness remains carried by each supplied tangent's
signed `w` component.

`ctex_mesh_map_set_import_external` validates declared channel meaning, colour
space, pixel layout, normal convention and tangent frame before copying the
source pixels. Resolution differences are accepted but reported. Sampling uses
filtered normalized UVs, converts DirectX tangent normals to the canonical
OpenGL convention, and returns the producing and current mesh revisions. A
missing map returns `CTEX_RESULT_MISSING_RESOURCE`; requirement preflight names
every missing or stale input rather than substituting neutral values.

After `ctex_mesh_replace`, `ctex_mesh_map_set_synchronize_mesh` first supports a
non-mutating sizing call and then advances the set revision while returning the
bindings made stale by that change. Selective and bulk release report the actual
resident tile bytes removed. The library has no implicit baking behavior.

`ctex_mesh_map_generator_get_info` exposes the stable eight-generator catalogue,
each required map and every parameter's default, range and meaning through
caller-owned arrays and packed strings. `ctex_mesh_map_generator_generate`
returns a tightly packed float mask together with the complete resolved
parameter set, all clamps, stale inputs and the named requirement diagnostic.
Sizing calls evaluate but do not publish caller buffers; repeated calls over the
same map set and parameters are byte-deterministic.

`ctex_mesh_map_set_request_bake` is the synchronous host-provider seam. The
provider advertises supported map kinds, receives the texture-set, UV, mesh,
settings and generation identities, and reports progress and cancellation on
the calling thread. Returned pixels are borrowed only for the callback and are
validated and copied before return. Unsupported, cancelled and provider-failed
requests return their matching `ctex_result`, leave bindings unchanged and put
the detailed named reason in the thread-local diagnostic. Callbacks must not
throw or unwind across the C boundary.

A provider shared library can attach the same descriptor to the headless CLI by
exporting the symbol named by
`CTEX_MESH_MAP_BAKE_PROVIDER_ENTRY_POINT_V1`. The entry point receives an
output descriptor whose `size` is initialized by the caller and returns a
`ctex_result`. Provider code and its `user_data` remain owned by the library;
the CLI keeps that library loaded until all synchronous requests finish. The
provider is responsible for its own external mesh and baker configuration, as
it is for an in-process host attachment.

For host-scheduled work, `ctex_mesh_map_bake_session_*` creates versioned request
tokens and accepts explicit completion data. Only the latest token whose
session, texture-set, UV, mesh, settings and generation identities remain
current can bind. Superseded, cancelled, malformed, foreign and duplicate
completions return a structured disposition without publishing pixels. Settings
edits snapshot bindings as one undo step; undo restores that snapshot and
invalidates pending work. Tokens are independent allocator-owned handles and
must be destroyed; sessions must be destroyed before their non-owning map set.

## Host transport

`ctex_texture_set_query_channel_delta` reports epoch-qualified channel cursors
and coalesced, row-major tile versions without reading pixels. A stale epoch
returns `CTEX_TRANSPORT_FULL_RESYNCHRONIZATION_REQUIRED`; explicit history reset
advances the epoch without changing channel contents.

`ctex_transport_snapshot_pool_create` establishes the host's pinned-byte budget.
`ctex_texture_set_query_channel_snapshot` atomically captures the delta and
returns an independently releasable snapshot whose tile versions remain stable
across later document edits. Admission returns `CTEX_RESULT_OVER_BUDGET` with
the additional required byte count when it would cross the pool ceiling.
Destroying a snapshot releases its pinned allocations; the pool memory report
exposes budget, pinned bytes, active tokens and unique pinned allocations.

The host enumerates versions from the snapshot, negotiates an exact or explicitly
converted component format, then requests each tile's stable layout. Layouts are
row-major, tightly packed, interleaved, native-endian and use a separate caller
buffer per tile. `ctex_transport_snapshot_read_tiles` stages every requested CPU
tile before publishing any output, so a bad version, format, layout or buffer
leaves all destinations unchanged. Snapshot query itself performs no pixel
transfer. `ctex_paint_preview_session_query_snapshot` applies the identical
version, format, layout and readback contract to provisional or finalized paint
previews; an admitted token remains valid after its preview session is destroyed.
Committed and cancelled sessions are no longer in flight and refuse new preview
snapshots.

`ctex_transport_snapshot_begin_readback` exposes CPU transfer through the same
opaque four-state operation used by the host route; CPU snapshots may already
be complete when the call returns. For a host-resident copy of a pinned logical
version, `ctex_transport_snapshot_begin_host_readback` returns pending and
accepts one exact version/layout/payload record per tile through
`ctex_transport_readback_complete_host`. Completion validates the whole batch
before publishing any caller-owned output. Cancellation, explicit failure,
mismatched completion and late completion publish nothing. Each readback retains
its snapshot pin until destruction, even if the public snapshot handle and its
source preview or document are destroyed first. Revision and delta queries never
start either transfer path.

## Execution routes

`ctex_executor_registry_create` always registers the CPU reference and
host-executed routes. Its required host descriptor reports capabilities and
attachment without passing any device handle into the library; a compiled
owned-GPU backend is also registered and
reports `CTEX_EXECUTOR_DEVICE_UNAVAILABLE` when its device cannot be opened.
Every route has a stable identifier plus its binding budget, maximum texture
dimension, supported texture formats, filtering support and compute support.
`ctex_executor_registry_get_info` returns those variable-size fields through
atomic caller-owned buffers.

Selection may be explicit or may apply, in order, a pinned registry default,
the `CTEX_EXECUTOR` environment variable and automatic route ranking. An unknown
environment value is reported while automatic selection remains in effect; an
unknown explicit or pinned identity is refused. `ctex_executor_make_fallback_report`
validates fallback decisions and refuses a CPU fallback unless recovery was
restored and the fallback executor is named.

`ctex_cpu_execute_bounded` exposes staged CPU work without publishing partial
results. The descriptor declares the item count, shared staging bytes,
per-worker scratch bytes, maximum workers, total memory ceiling and progress
interval before execution. A request whose complete working set overflows or
exceeds the ceiling is refused before allocating work storage or invoking a
work callback. A worker bound of one provides the same result as a larger bound
when the operation obeys the callback contract.

Work-item callbacks may run concurrently, receive one shared staging buffer and
one worker-private scratch buffer, and must synchronize any overlapping shared
writes. Cancellation and progress callbacks are serialized by the executor.
Cancellation is polled between bounded items; it discards all staging and never
calls commit. Commit runs once on the calling thread only after every item
succeeds. A non-success work or commit callback result aborts the C operation,
and callbacks must not throw across the boundary. The immutable execution-result
handle preserves status, processed count, exact memory request, actual worker
count and diagnostic message for two-call readback without rerunning the work.

`ctex_cpu_reference_rasterize_viewport` exposes the reference executor's own
camera rasterization. It clips triangles in homogeneous space, depth-tests
pixel centres and returns depth, perspective-correct UV, byte coverage and
source-triangle identity without consuming host-rendered buffers.
`ctex_cpu_reference_rasterize_uv` independently rasterizes one UV tile and
returns camera depth plus top-left-origin screen coordinates for every covered
texel. Both routes require an explicit maximum output-pixel count, reject the
request before raster allocation when it is exceeded, and publish their four
caller-owned arrays atomically after a sizing call. These functions expose the
reference inputs used by paint rejection and executor parity rather than a
fallback copy of GPU output.

`ctex_executor_parity_get_tolerance` publishes the numeric agreement contract:
one normalized code value for direct 8-bit and 16-bit values, two code values
after filtering, and explicit absolute-plus-relative bounds for floating-point
values. `ctex_executor_compare_parity` applies that same contract to complete
caller arrays, refuses empty inputs and treats every non-finite value as a
mismatch. Its report includes the maximum absolute deviation and the first
failure's index, values, measured deviation, allowed deviation and message.

`ctex_executor_run_parity_gate` applies that comparison contract to a corpus of
documents, strokes, cameras, materials and channel declarations. Executor
bindings refer to registry indices and supply host render callbacks. Exactly one
available CPU-reference binding is required. Every other available executor is
compared per channel against it; a compiled route whose device is unavailable
is reported as `unmeasured`, while an available route without a renderer fails
the report. Callback-produced arrays are borrowed and must remain valid until
the synchronous gate call returns; the returned result does not retain them.

The returned immutable handle reports reference, passed, failed and unmeasured
counts through `ctex_parity_gate_result_get_info`. Its caller-owned report buffer
contains JSON naming each executor and device, measured fixture count, status,
message, and any first-value failures with fixture, channel, measured deviation
and allowed deviation. A parity mismatch is a successful gate execution whose
`passed` field is false; invalid fixtures or CPU-reference output fail the call
without publishing a partial handle.

`ctex_host_execution_session` exposes the host-executed submission state
machine without moving device handles or pixels across the boundary. Each
resource handoff names a stable logical identity and generation, its owner,
required state, format, extent, mip count and tile shape. Submissions also name
their base document revision and replay semantics, and receive a unique
completion token. The library validates returned output identities, formats and
extents before any revision is published.

Successful results publish only against the expected base revision and only
after the host supplies either a complete result checkpoint or a deterministic
operation record with its base checkpoint and pinned inputs. Checkpoint-only
work remains pending until `ctex_host_execution_session_establish_recovery`
supplies a complete result checkpoint. Duplicate, stale, failed and cancelled
completions never advance the revision. Cancellation retains handed-off
resources until a late completion arrives, at which point the result is
discarded and the released logical generations are returned.

Completion and device-loss reports are independently owned opaque handles.
Their resource arrays use offsets into a packed NUL-terminated identity buffer,
with the standard two-call sizing contract. Committed logical generations can
be queried without exposing pointer identities. On device loss, all uncommitted
tokens are cancelled and reported, their resources are released, and the report
identifies the last recoverable committed revision and retained recovery bytes.

## Colour management

`ctex_get_working_color_space` and `ctex_color_space_get_name` identify the
working space as Linear Rec. 709. Every `ctex_rgb_color` carries its source
space; `ctex_color_convert` therefore never guesses a caller convention and
refuses unsupported spaces. `ctex_color_input_to_working` additionally applies
the channel policy, converting colour semantics while leaving linear-data
semantics unchanged. The supported space set is Linear Rec. 709 and sRGB with
Rec. 709 primaries.

`ctex_channel_get_color_policy` reports whether each built-in semantic is colour
or linear data and its recommended minimum precision. Normal and height report
16 bits; other built-ins report 8. `ctex_channel_get_bit_depth_warning` compares
a selected precision with that policy and names both selected and recommended
depths. `ctex_resolve_input_color_space` reports both the resolved space and
whether automatic mode inferred it: sRGB for colour semantics and linear for
data semantics. `ctex_accumulate_height` performs the sum before a single
quantization at the requested storage precision, while `ctex_quantize_unorm8`
exposes the deterministic ordered dither and its host disable switch.

`ctex_cube_lut_create` parses an in-memory `.cube` 3D LUT into an immutable
opaque handle. `ctex_cube_lut_apply_preview` first converts its explicitly typed
input to the working space and applies that LUT only through a preview-named
operation; authored and exported texture data are not mutated.

## In-memory image decode

`ctex_image_decode_memory` accepts encoded bytes directly and detects their
format from content. It accepts PNG, JPEG, TGA, BMP, baseline TIFF, flat
OpenEXR, Radiance HDR and flattened PSD. A mismatched filename extension is
reported without changing the content-selected format; unknown content is
refused with the supported set.

The output uses the standard two-call caller-buffer contract and is tightly
packed, row-major and interleaved; 16-bit components use native byte order.
`ctex_decoded_image_info` reports dimensions, channel count, scalar type, native
bit depth, colour space and its source, detected format, extension mismatch and
an uninterpretable-profile flag. Explicit caller colour declarations and the
automatic per-channel rule use the same enums as the colour-management API.
The source distinguishes an embedded PNG/BMP sRGB declaration from an
interpreted ICC profile embedded in PNG, multipart JPEG, TIFF, flattened PSD or
BMP, and from OpenEXR chromaticities or Radiance primaries. Supported ICC
profiles resolve D50-adapted Rec. 709 primaries with either sRGB or linear
transfer curves. Individual PSD layers use the same source profile in the C++
layered-image API; OpenEXR resolves colour metadata per part. Unsupported
embedded colour declarations set `uninterpretable_profile` before the automatic
rule is applied.
OpenEXR expands to RGBA float32 and Radiance HDR retains RGB float32; both use
linear Rec. 709 in automatic mode and preserve finite values outside `[0, 1]`.

Optional `ctex_image_decode_limits_descriptor` values cap width, height and
decoded bytes before pixel allocation. Malformed or truncated data is refused
without returning partial pixels; limit, unsupported-format and invalid-data
failures have distinct stable diagnostic codes.

`ctex_image_decode_memory_bounded` adds a required working-memory ceiling,
progress interval and optional synchronous progress/cancellation callbacks.
Progress reports inspection, codec, row-unpack and completion phases plus the
conservative peak working-set bound. Cancellation from the inspected-header
checkpoint stops before codec allocation; cancellation is also checked after
codec work and on every output row. `ctex_image_decode_execution_info` reports
the bound, checkpoint count and cancellation state even when the operation is
cancelled or refused for budget. Pixel output remains untouched unless the
whole decode succeeds. Callback state is borrowed for the call and callbacks
must not throw or re-enter the same decode operation.

`ctex_image_decode_layered_memory` selects composited or individual import for
PSD and multipart OpenEXR. Individual output preserves every source layer/part
name and signed origin. PSD composited output uses the authored flattened image;
multipart OpenEXR compositing applies later parts over earlier parts with
straight-alpha source-over across the union of their data windows. The sizing
call reports image-record, packed UTF-8 name, and packed pixel capacities.
`ctex_layered_decoded_image_info` gives each slice's format, origin, name range,
and pixel range. All three output regions are validated before any are written.
The same limits, working bound, progress, and cancellation descriptors apply.

`ctex_image_expand_channels` applies the documented destination expansion rule
without changing component representation or bit depth. It accepts tightly
packed or row-strided input and produces tightly packed caller-owned output.
Grayscale replicates into RGB, grayscale-alpha expands to RGBA while preserving
alpha, and RGB gains an opaque alpha for RGBA. The result reports the exact rule
used; shrinking and ambiguous grayscale-alpha-to-RGB conversion are refused.

`ctex_image_resample` exposes nearest-neighbour and pixel-centred bilinear
filtering over the same 8-bit, 16-bit and float32 layouts. `DEFAULT` resolves to
bilinear and the resolved filter is returned in `ctex_image_resample_info`.
Integer interpolation rounds to the nearest component value; floating-point
values remain unclamped. Input may be row-strided, output is tightly packed, and
the operation refuses allocation beyond the caller's ceiling or the 1 GiB
default ceiling.

## In-memory image encode

`ctex_image_encode_memory` accepts borrowed, row-major interleaved pixels and
encodes PNG, JPEG, TGA, TIFF or OpenEXR directly into a caller-owned buffer.
Input can be tightly packed or use an explicit row stride. The descriptor names
the input scalar representation and bit depth separately from the requested
output bit depth, colour space and format; JPEG output also accepts quality from
1 through 100.

The operation uses the standard null-buffer sizing call and does not modify a
too-small output buffer. It accepts 8- or 16-bit unsigned-normalized and 32-bit
floating-point input. PNG supports 8/16-bit output, JPEG and TGA support 8-bit,
TIFF supports 8/16/32-bit, and OpenEXR supports 16/32-bit. Unsupported pairs are
refused with a stable diagnostic that names both the format and bit depth.

## Texture export

`ctex_texture_export_run` exposes the complete texture-export pipeline without
giving the library ownership of host files. Borrowed catalogue descriptors name
texture sets, occupied UDIMs, atlases and layer hierarchies. The optional plan
selects all or named texture sets, texture-set/UDIM/atlas spatial scope,
visible/selected/per-selected-layer output, an optional resolution and a
filename pattern. A null plan uses the documented core defaults.

A null preset selects the default built-in. A preset descriptor with an
identifier and zero textures selects another built-in; their stable identifiers
are available through the packed caller-owned buffer returned by
`ctex_texture_export_get_built_in_preset_ids`. A descriptor with textures is a
fully data-driven custom preset. Its four channel-token strings can name built-in
values, derived values, mesh maps, or arbitrary registered semantic components,
so adding a channel does not add a fixed ABI field. Formats use the existing PNG,
JPEG, TGA, TIFF and OpenEXR enumeration and validate the same 8/16/32-bit matrix
as `ctex_image_encode_memory`.

The call is synchronous. For each planned output the source callback receives a
borrowed, self-describing view of its path, source sets, UDIM/atlas identity,
selected layers, resolution and encoding. It returns dimensions, optional binary
coverage and a sampler. Samples carry the standard PBR values plus open-ended
named mesh-map and registered-channel arrays. Completed encoded files are passed
to the output callback as self-describing borrowed byte spans; the host copies
any bytes it wants to retain. The final report callback receives the canonical
JSON manifest. Dry runs require only the report callback and invoke neither the
source nor output callback.

Progress is reported after each fully encoded output. Cancellation is checked
throughout sampling, resampling, padding and encoding. Already completed buffers
and a report marked `cancelled` are delivered, no partial buffer is exposed, and
the call returns `CTEX_RESULT_CANCELLED`. Callback views and byte spans remain
valid only for the callback. All callbacks run on the calling thread, carry the
configured user-data pointer and must not throw across the C boundary.

## Project containers

`ctex_document_get_memory_report` returns aggregate and per-texture-set channel,
tile-history, bound-map and total resident bytes through caller-owned arrays and
packed stable identities. It also reports a pre-save size estimate without
encoding the document. The estimate uses current sparse resident channel and
map payloads plus deterministic container/texture-set/atlas metadata overhead;
undo history is reported as resident memory but is not counted as saved project
payload. Short detail or identity buffers publish neither aggregate nor detail
output.

The project-container boundary accepts and returns complete encoded container
bytes rather than exposing C++ container objects. `ctex_project_container_create_empty`
uses the normal null-buffer sizing call to create a canonical empty container,
and `ctex_project_container_probe_version` reads only its fixed header.

`ctex_project_container_normalize` opens caller-supplied bytes under either the
default limits or a `ctex_project_container_read_limits_descriptor`. It returns
the exact canonical byte and NUL-terminated JSON-report sizes in
`ctex_project_container_info`. The report inventories tiled images, referenced
or packed resources, standalone assets and unknown sections. Newer schema
versions and unsupported sections are reported and retained in the canonical
output. Both output buffers are validated before either is written, so a short
buffer never exposes a partial pair.

Document-domain serializers store texture sets, layer structures, graph and
preset state, bindings, editable entries, replay records and document settings
as versioned asset payloads inside this same framing. Normalization preserves
their identifiers, kinds, versions, dependencies and payload bytes exactly;
resources, sparse tiled pixels and recovery checkpoints remain first-class
sections in the same canonical output.

`ctex_project_container_get_texture_document_ids` inventories the live-document
assets without exposing container internals. A host restores a selected asset
into an existing `ctex_document` with
`ctex_project_container_restore_texture_document`, edits it through the normal
document API, and writes it back with
`ctex_project_container_upsert_texture_document`. Upsert follows the atomic
two-call project-output contract and retains unrelated, opaque and newer-schema
content.

The document's mesh resource reference and per-texture-set mesh-map bindings
use a versioned companion asset. A host writes all bindings from live
`ctex_mesh_map_set` handles with
`ctex_project_container_upsert_document_mesh_state`, inventories the saved mesh
revision, resource identifier and sorted texture-set identifiers with
`ctex_project_container_get_document_mesh_state_info`, and atomically restores
them with `ctex_project_container_restore_document_mesh_state`. Restore requires
exact texture-set coverage and map-set handles created for the saved current
mesh revision; a mismatched revision returns `CTEX_RESULT_STALE_STATE` without
changing any binding. Sparse stored tiles are charged at their full restored
allocation size under the project read budget.

`ctex_project_container_save_atomic` applies the same validation and limits,
then publishes the normalized container through the core sibling-temporary,
synchronize and atomic-replace path. It does not write a destination until the
whole input has parsed and re-encoded successfully. Repeated normalization or
save of unchanged input is byte-identical.

`ctex_project_autosave_session_*` exposes the periodic worker through an
allocator-owned handle. Submission validates the complete caller-owned
container, pins an independent snapshot of its canonical content before
returning, and queues only a nonzero revision newer than every pending, writing
or published revision. Parsing and snapshot capture happen on the submitting
thread; compression and filesystem publication happen on the worker. Status
reports pending, writing and published revisions, successful writes, the stable
recovery path and the last error. Timed wait observes normal completion, while
flush requests immediate durable publication and waits.

`ctex_project_recovery_enumerate` probes only published `.ctex-recovery` files
and returns valid candidates newest-first beside malformed candidates and their
diagnostics. Paths and keys use one packed caller-owned string buffer.
`ctex_project_recovery_read` opens one selected path under the same untrusted
input limits and atomic two-call byte/report contract as container
normalization. Merely pending autosaves and sibling temporary files are never
enumerated.

`ctex_project_asset_export` extracts one named standalone asset together with
exactly its declared resource and tiled-image dependencies. By default external
resources remain referenced; setting `self_contained` packs them from the
supplied source directory. `ctex_project_asset_install` validates a standalone
package, resolves referenced resources through ordered search paths, packs
resolved bytes into the destination library, rejects identity conflicts, and
returns the updated encoded library. Both operations use the same atomic
two-call byte-and-report output contract as normalization. A null search-path
descriptor can install a package only when all required resources are already
packed.

`ctex_operation_record_create` validates algorithm, preset, replay, channel,
mesh-frame, pinned-input, checkpoint, and payload metadata and returns a
canonical versioned record plus JSON inventory. `ctex_operation_record_inspect`
applies the same validation to existing bytes. The inventory exposes content
identities and byte counts but never duplicates pinned payload bytes.

`ctex_project_container_upsert_operation_record` installs the canonical record
as an `operation-record` standalone asset and refuses to replace an asset of a
different kind. `ctex_project_container_get_operation_record` retrieves the
named record only after validating agreement between the asset metadata,
checkpoint dependencies, and payload. These functions follow the usual
caller-owned sizing contract and are described with the binary format in
[Versioned operation records](operation-records.md).

`ctex_operation_record_assess_replay` compares a record with host-declared
algorithm version ranges and reports same-resolution, resolution-independent,
checkpoint-only, resample-required, or unsupported-algorithm disposition.
Unknown versions retain named raster checkpoints and return an explicit
diagnostic. Replayable clone, blur, and smear records are valid only with an
owned `source-snapshot` input.

`ctex_resource_ledger_admit_operation_recovery` subjects the canonical record
and its checkpoint byte count to the existing CPU and backing-store budgets.
Success returns a normal reservation that must remain alive through commit;
budget refusal and quiescence publish no reservation.

After normal open or `ctex_project_recovery_resume`,
`ctex_project_container_assess_operation_replay` inventories every operation
record against the host-supported version ranges. Counts distinguish replayable,
checkpoint-fallback and unsupported records; the JSON report names each record,
its disposition and diagnostic. Report-buffer refusal is atomic.

`ctex_texture_set_change_resolution` requires replay-eligible, resample-all or
cancel policy. Replay mode assesses the supplied canonical operation records,
requires an explicit nearest or bilinear policy for every checkpoint fallback,
and validates one complete host-evaluated raster for each enabled base or UDIM
channel. Resample-all evaluates the same filter in the core. Both paths enforce
working and retained-history byte ceilings before atomically swapping any
state. `ctex_texture_set_undo_resolution_change` and
`ctex_texture_set_redo_resolution_change` restore dimensions, pixels, UDIM
storage and tile-history context as one step. See
[`resolution-changes.md`](resolution-changes.md).

## Smart materials

The smart-material boundary accepts the canonical versioned serialization used
by the C++ document model and returns migrated canonical bytes plus a
NUL-terminated JSON inventory. `ctex_smart_material_inspect` validates the full
stack, reports derived versus model-specific content and inventories exposed
parameters, anchors and resource declarations.

`ctex_smart_material_set_parameter` applies one typed scalar, vector, colour,
string, image or boolean value to every declared graph binding atomically.
`ctex_smart_material_set_anchor` and
`ctex_smart_material_add_anchor_reference` edit anchor metadata while enforcing
stack ordering, active graph inputs and acyclic dependencies.
`ctex_smart_material_plan_anchor_evaluation` returns the affected entries in
deterministic stack order. Every byte/report pair uses the same caller-owned
two-call contract as project normalization; validation failure leaves all
outputs untouched.

`ctex_smart_material_package` combines a canonical material with an explicit
portable resource manifest. Referenced resources may remain external or be
packed from a source directory for self-contained sharing.
`ctex_smart_material_import` opens the package under project-container limits,
resolves resources through ordered search paths, and reports every material
image input as packed, referenced or missing. Missing inputs retain their stable
identifiers and are never silently substituted.

`ctex_texture_set_apply_smart_material` and
`ctex_texture_set_apply_smart_mask` instantiate canonical fragments into a
texture set. Each application contributes one undo step regardless of its entry
count. `ctex_texture_set_get_preset_applications` returns the stable entry
identities, mask targets, editable state and preset origin metadata as JSON;
`ctex_texture_set_set_applied_entry_state` edits an individual instantiated
entry without losing that origin. One
`ctex_texture_set_undo_last_preset_application` call removes the complete most
recent material or mask application.

`ctex_preset_library_enumerate` validates caller-described shelves and returns
stable JSON listings for all seven preset kinds, including display names,
sorted tags, schema versions, and thumbnail metadata. Shelf contents are
bounded project-container bytes. `ctex_preset_library_resolve` selects a preset
by its globally stable identifier and returns a standalone project-container
package containing the asset, its exact dependencies, and its shelf thumbnail.
Future preset versions are refused by identity before enumeration or
resolution.

`ctex_texture_set_query_channel_delta` exposes epoch-qualified channel cursors
and row-major changed-tile records containing revision, generation and
residency. The report states how many revision-index entries were visited, so
hosts can verify that unchanged and recent queries scale with the change rather
than total document size. A cursor from another epoch requests full
resynchronization without returning a misleading partial delta.
`ctex_texture_set_reset_channel_revision_history` starts a new epoch without
changing channel pixels and is the explicit overflow/recovery mechanism.

## Stroke reconstruction

`ctex_stroke_settings_init` produces the complete canonical default settings,
including the default linear response curves. A caller can then configure tip
mode, spacing, base stamp properties, stabilizer, independent pressure and tilt
response mappings, deterministic jitter, entry and exit taper, path constraints,
and mirror or radial symmetry. A mapping with a null points pointer and zero
count uses the canonical linear curve; custom curve points are borrowed for the
duration of resolution.

`ctex_stroke_resolve` accepts timestamped object-space samples and emits the
canonical ordered stamp sequence plus swept segments for continuous tips. It is
stateless and deterministic: callers use null arrays and zero capacities to
query both output counts, then supply caller-owned arrays. No array is modified
when either capacity is too small. Each returned stamp refers to the settings'
borrowed tip-resource string, so that string must remain alive while the result
is consumed.

Sample and settings structures are versioned. Pressure presence is explicit, so
mouse input is distinct from zero pen pressure. Frames, resolved stamp
properties, source and symmetry ordinals, reconstruction version, tip mode and
symmetry-instance count are all preserved at the boundary. Invalid curves,
frames, enum values, timestamps and reconstruction versions return
`CTEX_DIAGNOSTIC_INVALID_STROKE` without partial array output. The interpolation,
timestamp and fixed-grid rules remain defined by
[`stroke-reconstruction.md`](stroke-reconstruction.md).

`ctex_stroke_preset_serialize` writes a named current-schema preset as canonical
bytes with the standard two-call buffer contract. `ctex_stroke_preset_deserialize`
validates and migrates those bytes, first reporting the exact storage needed for
the name, tip identity and all response-curve points. A filling call writes that
data into caller-owned buffers and returns a settings descriptor whose pointers
refer only to those buffers. Older schemas receive documented defaults for
absent fields; newer schemas are refused with
`CTEX_DIAGNOSTIC_INVALID_STROKE_PRESET` rather than partially applied.

## Tile paint coverage

`ctex_paint_evaluate_tile_coverage` rasterizes a named mesh UV set into one
bounded tile and evaluates canonical geometric stroke coverage at every covered
surface texel. The caller supplies a `ctex_resolved_stroke_descriptor`; its
stamps and swept segments are validated and consumed unchanged, so hosts with
their own stroke engine bypass spacing, jitter and taper reconstruction. A
resolved result from `ctex_stroke_resolve` can be passed directly by combining
its returned info and arrays into this descriptor.

The output is a row-major array of double-precision coverage values using the
standard two-call contract. Continuous tips evaluate their swept segments and
therefore cover gaps between distant stamps; discrete tips evaluate only their
rotation- and elongation-transformed stamp shapes. Values contain geometric
falloff only—opacity, flow, masks, rejection and deposition remain later paint
stages.

`ctex_paint_evaluate_material_coordinates` rasterizes that same tile and returns
an explicit per-texel material-sampling plan. Covered texels return either one
UV projection, three squared-normal-weighted world-axis projections for
triplanar sampling, or one projection through a caller-supplied orthonormal
planar frame. Uncovered texels are reported with zero projections. The complete
result is staged before caller output changes.

`ctex_paint_rejection_init` initializes depth and angle rejection to their
canonical enabled defaults and leaves backface rejection disabled. A caller can
then attach per-symmetry depth projection contexts and per-texel view directions
before calling `ctex_paint_evaluate_rejected_coverage`. The result reports the
explicit depth disposition, counts rejected by each rule, resolved thresholds,
and whether either threshold was clamped. Depth projection positions use
top-left-origin viewport coordinates. Backface rejection uses counter-clockwise
triangle winding and directions from the surface toward the camera. All inputs
are borrowed only for the call, and output remains unchanged on failure.

`ctex_paint_work_init` supplies the canonical 64-texel storage-tile size and
two-texel seam-dilation radius. `ctex_paint_plan_work` accepts exact half-open
stamp footprints in canvas coordinates, expands and clips them by the resolved
dilation radius, and returns the deduplicated storage tiles in deterministic
row-major order. Its report includes total canvas tiles, supplied footprints,
candidate visits before deduplication, the exact processed count, and any
dilation-radius clamp. Planning scales with reachable footprint tiles rather
than scanning the canvas; it does not mutate a document or invoke processing.

`ctex_paint_seam_dilation_init` supplies the canonical two-texel radius.
`ctex_paint_dilate_uv_seams` accepts a bounded one-to-four-component floating-point
raster and one binary coverage value per texel. It extrapolates each component
independently from immutable source pixels, preserving signed and HDR gradients,
and never overwrites covered texels. Radius zero returns the source unchanged;
larger radii are clamped to 4,096. The report distinguishes all dilated texels
from thin-island cases that required constant, zero-gradient extrapolation.

Deferred stroke dilation uses an opaque `ctex_paint_dilation_session`. Staging a
UV coordinate replaces its earlier raster, so hosts can submit the latest tile
from each interactive frame without triggering a pass. Preview queries return
sorted, undilated tiles marked `CTEX_PAINT_DILATION_PROVISIONAL`. Finish dilates
every staged tile once, returns `CTEX_PAINT_DILATION_FINAL`, and is idempotent;
staging afterward is refused. Tile metadata gives offsets into one flattened
caller-owned pixel buffer. Both preview and finish support count-only queries,
and insufficient output buffers do not finalize or modify caller output. The
session and all long-lived staged arrays use the allocator captured at creation.

Surface-filter descriptors provide explicit surface-adjacent taps, logical tap
offsets, sampling radii and tangent frames. Scalar filtering preserves signed
derivative weights; tangent-vector filtering transforms every value through its
source frame into the output frame and renormalizes it, including across
mirrored UV islands. Island-padding descriptors use `CTEX_NO_UV_ISLAND` for
unoccupied texels. Planning returns ownership plus flattened unsupported-mip
records whose offsets address the affected-island array. Applying the same
request copies only from the owning island, leaves contested gutters and valid
island texels unchanged, and returns caller-owned pixels. Planning and applying
both use count-only queries and leave result structures and arrays unchanged
when an output buffer is too small.

`ctex_paint_surface_map_cache` retains immutable, allocator-routed UV-space map
bundles keyed by mesh revision, partition identity, UV set, dimensions and tile
origin. A lookup exposes interpolated surface texels, coverage, exact
source-triangle identities and UV-island identities through optional
caller-owned buffers. Passing no buffers performs a sizing lookup; a following
copy is a cache hit. The info structure is always updated with required sizes
before buffer validation, while undersized buffers themselves remain unchanged.
Changing a mesh revision invalidates every old entry, and selecting another UV
set invalidates every tile for that partition. Statistics report entries, hits,
misses and invalidations; clearing also resets those counters.

Paint previews use an opaque `ctex_paint_preview_session` bound to one enabled
document channel. Pixel writes affect only its copy-on-write image. Metadata,
row-major authored-format bytes and changed tile coordinates are available
through caller-owned buffers. `CTEX_DEFAULT_TILE_SIZE` declares the 64-texel
storage tile used to interpret those coordinates. Finalization applies seam dilation, commit refuses
a stale source revision, and cancellation never publishes pixels. The document
must outlive the session.

`ctex_paint_evaluate_tile_deposition` evaluates the same bounded tile through
per-stamp deposition and alpha discard. Non-building mode reports maximum
coverage and maximum `opacity * flow * coverage`; explicit build-up mode applies
flow once per canonical stamp and caps the accumulated result with opacity. The
output retains both the accumulated strength and the write-filtered strength,
plus a per-texel write mask. Alpha discard defaults to 0.1 for 8-bit output and
0.004 for 16-bit or floating-point output; a custom threshold is clamped to the
normalized range and that clamp is reported in the result metadata.

Per-tile raster, coordinate, rejection, dilation, deposition and blending operations are
capped at `CTEX_MAX_PAINT_TILE_TEXEL_COUNT` (1,048,576) texels so these
boundaries cannot silently allocate a canvas-sized intermediate.

`ctex_paint_combine_masks` intersects any number of active-layer masks with
optional colour-ID, geometry, screen and UV-island selections. Null mask input
produces the identity mask, while supplied inputs must contain one normalized
weight per tile texel. The operation reports how many active inputs contributed.
The current deposition descriptor accepts that same mask-input descriptor and
applies its intersection to every canonical stamp event before accumulation.
The original V1 deposition descriptor remains accepted and means no masks.

`ctex_paint_blend_snapshot` turns deposition samples into row-major RGBA pixels.
It evaluates the named blend mode against the caller's immutable stroke-start
tile, never against a prior call's output. All twenty blend modes use the same
formula surface as material graphs and layer compositing. A deposition sample
whose alpha-discard write flag is zero preserves its snapshot pixel. Input
arrays are validated in full before any caller-owned output is changed.

`ctex_paint_apply_brush` composes that deposition with an active material across
all enabled layer channels. Layer and material rasters name their semantic and
one-to-four component count; missing or mismatched material channels are
refused, while extra disabled material channels are ignored. Output channels
follow enabled-layer order. A null output array queries both the channel count
and pixels per channel, and a filling call validates every channel buffer before
writing any of them.

`ctex_paint_apply_eraser` applies the same write-filtered deposition to either
layer-opacity or mask values using `start * (1 - strength)`. Both calls consume
the canonical upstream stroke result, so radius, opacity, flow, hardness,
rotation, coordinate mode, rejection, masks and symmetry are resolved once by
the existing stroke/deposition pipeline rather than reinterpreted at the tool
boundary.

`ctex_paint_apply_fill` consumes the public cached surface-map arrays and
resolves Whole set, Triangle, Connected-by-angle, UV island, UV tile or
Selection scope. Connected fill traverses explicit triangle adjacency and
unit geometric normals; face and island fills use the exact cached identities.
The resolved scope is intersected with ordinary paint masks and optional
rejection acceptance before every enabled channel is shaded. Query calls report
all required sizes, and filling calls validate the scope, selected-triangle and
channel buffers before publishing any output.

`ctex_paint_apply_clone` maps a separately supplied source anchor onto cached
destination UVs in aligned or fixed mode. It samples only the immutable source
snapshot, applies canonical write-filtered deposition and returns the resolved
source index for every destination texel. Source and destination texture-set
identities must match; a refusal names both identities. Sample-index and channel
outputs are validated together before either is written.

`ctex_paint_apply_blur` runs separable horizontal and vertical filtering from
immutable stroke-start channels through explicit surface-aware neighborhoods.
`ctex_paint_apply_smear` samples one upstream surface mapping per texel and
multiplies canonical deposition by its resolved strength. Both preserve the
seam-aware tangent-frame rules, report clamped radius/strength values and
validate every output channel before publication.

`ctex_paint_resolve_stencil_mask` samples a bounded opacity image at explicit
screen positions through position, rotation and per-axis scale, with optional
inversion. It reports the resolved transform and clamp flags. The caller-owned
normalized result is accepted directly by the canonical deposition mask input,
so Stencil composes with per-stamp build-up, rejection and Brush shading without
duplicating those rules.

`ctex_paint_rasterize_decal` projects a caller-retained surface placement and
pinned material through a normal-derived frame. Hosts may edit its transform
without repicking; explicit rasterization reports the resolved frame and clamp
flags, then atomically publishes source indices, composed strength and all
enabled channels.

`ctex_paint_apply_projection` applies a pinned material through an explicit
camera matrix and visible-surface mask, a finite planar frame, or a repeating
triplanar mapping. Each destination texel reports up to three source indices and
weights beside the composed strength. Parameter clamps and every enabled output
channel are reported through caller-owned storage without partial publication.

`ctex_paint_apply_text` consumes length-delimited UTF-8 and caller-supplied font
metrics plus normalized glyph coverage. It exposes the decoded codepoints and
aligned raster alongside the projected decal result, with explicit per-string
size, tracking and left, centre or right alignment. Font inputs are borrowed for
the call; raster opacity, source samples, strength and enabled channels publish
atomically into caller-owned storage.

`ctex_paint_apply_particles` reuses a caller-owned pick index to simulate
fixed-step mesh collisions from an explicit emitter and deterministic seed. It
reports all resolved count, lifetime, speed, mass, gravity, friction,
restitution and randomness controls, then atomically publishes ordered contacts,
final states, texture-set identities, contact-to-texel mappings, composed
strength and enabled channels. The surface-map revision must match the indexed
mesh revision.

`ctex_paint_pick_enabled_channels` consumes an existing surface hit and
validated texture-set/UDIM views. It preserves enabled-channel order, reports
the sampled semantic, component count and value, and returns material provenance
only when the matching view explicitly supplies it. Missing and overlapping
views are refused. Result identities use one packed caller-owned string buffer;
channel values and strings publish atomically after a sizing call.

`ctex_paint_select_colour_id` compares a picked linear-RGB colour with a
validated colour-ID map using an explicitly bounded Euclidean tolerance. It
reports the resolved tolerance, clamp status, selected texel count and a
distinct matched-or-empty result. The caller-owned scalar output is directly
usable as a paint restriction, mask source or visibility filter; a zero
tolerance is never widened.

`ctex_paint_select_screen` projects revision-matched cached surface texels after
an indexed rectangle or lasso broad phase, so only texel centers inside the
clipped screen region remain selected. `ctex_paint_select_polygon` expands from
one picked texel by exact triangle, UV island or bounded connected angle. Both
report ordered represented triangles and publish a caller-owned binary mask;
that owned output is directly storable and reusable as a paint restriction.

`ctex_paint_get_parameter_catalogue` exposes the stable name, default, inclusive
minimum and maximum for every audited numeric paint control. General controls
have one entry; taper extents and alpha discard use explicit context values for
their conditional ranges and defaults. `ctex_paint_validate_parameter` resolves
a finite supplied value through the same `validate_tool_parameter` seam used by
tool execution and reports whether it clamped. Each descriptor identifies
whether the value is continuous or integral. The catalogue contains 74 contextual
entries representing all 69 documented controls; the repository's parameter-audit
gate proves that every listed control changes observable output.

`ctex_material_graph_get_builtin_catalogue` returns a deterministic JSON
description of all 52 built-in material nodes. Each record includes stable type
identity and version, category, input and output socket declarations, property
defaults and allowed choices. Separate scalar and vector-math tables include all
operation names and formulas. The info structure reports exact family and
operation counts before the caller allocates the JSON buffer.

`ctex_material_graph_create_default` creates a canonical device-independent
document with exactly one output node and the nine registered metallic/roughness
channels. `ctex_material_graph_inspect` validates serialized input and returns
canonical bytes plus readable node, socket, value, position and link JSON;
canonical byte equality is also available through `ctex_material_graph_compare`.
Built-in nodes and typed socket/property values are edited transactionally.
`ctex_material_graph_add_link` reports the resolved coercion and any replaced
source link, while incompatible types and cycles return a graph diagnostic
without output bytes. `ctex_material_graph_validate` reports required inputs,
missing images or mesh maps, and unreachable nodes without attempting emission.
Serialized graph input is bounded to 64 MiB.

Reusable node groups live in an opaque `ctex_material_graph_workspace`. A
workspace owns named material graphs and group definitions; group subgraphs can
be edited through the same built-in-node, typed-value and link operations as a
material. Each group declares ordered input and output sockets. Interface edits
propagate atomically to every material and nested-group instance, preserve
stored values whose socket identity and type remain compatible, and report the
number of updated instances and removed links. Group placement checks the full
dependency path before mutation. Direct and transitive recursion return
`CTEX_DIAGNOSTIC_INVALID_MATERIAL_GRAPH` with the complete cycle in the thread-
local diagnostic, leaving the containing graph unchanged. Material and group
graphs remain inspectable through the caller-owned graph two-call contract.

Host node types live in an isolated `ctex_material_graph_node_registry`. A
versioned registration copies ordered socket and property declarations, CPU and
emission callbacks, callback user data, determinism, resource dependencies,
supported WGSL/MSL/SPIR-V/HLSL targets and typed parity fixtures. Registration
is atomic and refuses a missing CPU or emission implementation before the type
becomes visible. Registered nodes can be inserted into caller-owned documents
or workspace material/group graphs. Registry-aware validation checks the saved
interface and selected target; an unregistered serialized node remains opaque
and canonical but makes the graph non-emittable with its type and version named.
`ctex_material_graph_node_registry_verify_contract` executes every fixture
through both callbacks and separately reports whether deterministic replay is
eligible with the caller's pinned dependencies. Callback request storage is
borrowed only for the call. CPU output descriptors are pre-sized to the declared
output count. Text/image values, emission expressions and resource strings
returned by a callback remain host-owned, must stay valid until the enclosing
API call returns, and are copied immediately after the callback returns. The
host keeps callback user data alive until no operation can use the registration
and the registry is destroyed.

Material presets use canonical caller-owned library bytes rather than a process
handle. `ctex_material_graph_library_add_preset` saves a graph with its stable
identity, display name and thumbnail resource; duplicate identities are refused.
Inspection validates and reorders presets by stable identity, returning exact
canonical bytes plus deterministic JSON metadata. Resolving a preset returns an
independent canonical graph document. Consequently the same library bytes on a
second machine resolve an identity to the same graph without depending on
insertion order or local paths. Library input is bounded to 256 MiB.

## Shader emission

`ctex_shader_emit_material` compiles a canonical material graph without
creating or receiving a graphics-device object. Its versioned request declares
the target language, device binding and texture capabilities, logical input
resources, output resource generation and extent, sampling policy, and draw
vertex count. A node registry is optional for built-in-only graphs and required
when the document contains registered host nodes.

WGSL and HLSL return separate NUL-terminated vertex and fragment sources. MSL
returns one NUL-terminated module containing both entry points in the vertex
artifact and reports a zero fragment size. SPIR-V returns separate raw vertex
and fragment byte modules. The accompanying NUL-terminated JSON pass plan names
the stable plan identity, logical textures and generations, extents and tile
shapes, subresource accesses, bindings and entry points, vertex layout, target
state, draw command, dependencies, and derived submission lifetimes. A second
JSON array reports capability workarounds such as nearest sampling when float
linear filtering is unavailable.

All four outputs use one atomic two-call sizing operation. The sizing call
returns exact artifact and JSON sizes in `ctex_shader_material_info`; a later
call validates every destination before copying any output. Text sizes include
their terminating NUL, while SPIR-V sizes are exact binary byte counts.
Repeated requests are byte-identical, and changing only graph constants changes
the shader while preserving the pass plan and binding layout.

`ctex_shader_emit_material_inspectable` accepts either serialized graph bytes or
a named material in a graph workspace. Text artifacts retain deterministic
producer comments and stable variable names. Its companion JSON maps every
emitted variable to the qualified group path, node type and ID, and output
socket. Group paths include every enclosing group identifier and instance ID,
so identical internal node IDs in different instances remain distinct. The
SPIR-V artifacts remain raw binary; `binary_companion` identifies the JSON as
their inspection metadata. The artifacts, plans, workaround report and debug
metadata share one atomic two-call sizing operation.

`ctex_shader_get_backend_attribution` returns deterministic JSON naming the
vendored Kongruent backend, Zlib licence, source URL, pinned ArmorPaint and
upstream revisions, licence file and local-changes record. This runtime report
matches the dependency information enforced by the repository licence audit.

`ctex_shader_emit_layer_stack` compiles a bottom-to-top premultiplied stack for
the same four targets. A stack that fits the declared binding budget produces
one compositing pass; a larger stack is split deterministically, with each later
pass depending on and sampling the preceding carried intermediate. The artifact
blob packs every pass's vertex and fragment program. Its JSON inventory names
the pass and target, distinguishes NUL-terminated text from raw SPIR-V, and
publishes the offset and byte size of every stage. The pass-plan and workaround
outputs use the same atomic sizing contract as material emission.

`ctex_shader_emission_cache_create` creates one cache shared by material,
layer-stack and preview emission. `ctex_shader_emit_material_cached` caches by
canonical graph content, host-node registry semantics, target and normalized
feature set; the layer-stack entry point uses the supplied cache when non-null
and otherwise emits directly. Cache hits return byte-identical artifacts and
plans. Aggregate entry, hit and miss counts are observable, and clearing removes
entries and resets all counters.

`ctex_shader_emit_lit_preview` compiles the document-channel preview for WGSL,
MSL, SPIR-V or HLSL. Its optional environment descriptor declares prefiltered
cube radiance, cube diffuse irradiance and the 2D split-sum BRDF lookup as
logical textures. The JSON pass plan preserves each view dimension, encoding
and mip convention. It also publishes the exact uniform block layout for camera
position, environment rotation and intensity, and up to four analytic lights.
Without the environment descriptor, the shader uses the documented deterministic
sky/ground fallback and requires no environment texture binding.

`ctex_shader_emit_channel_inspection` emits an unlit display shader for one
semantic channel. It binds only that channel and its sampler and declares no
lighting uniform. Both preview routes use the standard atomic artifact and JSON
sizing contract, report capability workarounds, and use the supplied emission
cache when non-null. The complete BRDF, environment and fallback conventions
remain documented in [Preview shading](preview-shading.md).

## ABI version and compatibility

`ctex_get_abi_version` is safe before any handle exists and returns the major,
minor and patch values derived from the repository's single `VERSION` source.
Bindings compare its major value before calling any other operation.
`ctex_get_version` remains the general library-version query and currently
returns the same value.

Within one ABI major version:

- an exported symbol is never removed and its declaration never changes;
- public structure fields are never removed, reordered, renamed, or repurposed;
- new descriptor fields are appended and remain guarded by the size prefix;
- enumeration entries keep their names, values, and order, with additions
  appended; and
- every public C declaration has one matching `ctex_*` dynamic export.

[`abi/cybertexel-abi-v0.json`](../abi/cybertexel-abi-v0.json) records the first
v0 release surface. `just gate-abi-diff` extracts the current header surface,
compares it with that baseline, checks the Windows export definition, and
inspects the built shared object. A same-major removal, signature change,
non-append descriptor edit, enum renumbering, or header/export mismatch fails
with the affected name. A deliberate incompatible change therefore requires a
major version increment.

The [C ABI capability coverage inventory](c-abi-coverage.md) maps OpenSpec
requirements to their public entry points. Its gate stays red, with every gap
named, until the complete runtime surface is reachable without C++ access.

## Threading contract

The contract is stated per entry-point family:

| Calls | Contract |
| --- | --- |
| `ctex_get_version`, `ctex_get_abi_version` | Process-safe and callable concurrently from any thread |
| `ctex_get_working_color_space`, `ctex_color_space_get_name`, `ctex_channel_get_color_policy`, `ctex_channel_get_bit_depth_warning`, `ctex_resolve_input_color_space`, `ctex_color_convert`, `ctex_color_input_to_working`, `ctex_accumulate_height`, `ctex_quantize_unorm8` | Stateless, process-safe and callable concurrently from any thread |
| `ctex_image_decode_memory`, `ctex_image_decode_memory_bounded`, `ctex_image_decode_layered_memory`, `ctex_image_expand_channels`, `ctex_image_resample`, `ctex_image_encode_memory`, `ctex_material_graph_get_builtin_catalogue`, `ctex_material_graph_create_default`, `ctex_material_graph_inspect`, `ctex_material_graph_compare`, `ctex_material_graph_add_builtin_node`, `ctex_material_graph_set_input_value`, `ctex_material_graph_set_property_value`, `ctex_material_graph_add_link`, `ctex_material_graph_validate`, `ctex_stroke_settings_init`, `ctex_stroke_resolve`, `ctex_stroke_preset_serialize`, `ctex_stroke_preset_deserialize`, `ctex_paint_evaluate_tile_coverage`, `ctex_paint_evaluate_material_coordinates`, `ctex_paint_rejection_init`, `ctex_paint_evaluate_rejected_coverage`, `ctex_paint_work_init`, `ctex_paint_plan_work`, `ctex_paint_seam_dilation_init`, `ctex_paint_dilate_uv_seams`, `ctex_paint_filter_surface_scalar`, `ctex_paint_filter_surface_tangent_vector`, `ctex_paint_plan_island_padding`, `ctex_paint_apply_island_padding`, `ctex_paint_combine_masks`, `ctex_paint_evaluate_tile_deposition`, `ctex_paint_blend_snapshot`, `ctex_paint_apply_brush`, `ctex_paint_apply_eraser`, `ctex_paint_apply_fill`, `ctex_paint_apply_clone`, `ctex_paint_apply_blur`, `ctex_paint_apply_smear`, `ctex_paint_resolve_stencil_mask`, `ctex_paint_rasterize_decal`, `ctex_paint_apply_projection`, `ctex_paint_apply_text`, `ctex_paint_pick_enabled_channels`, `ctex_paint_select_colour_id`, `ctex_paint_select_polygon`, `ctex_paint_get_parameter_catalogue`, `ctex_paint_validate_parameter` | Stateless and safe to call concurrently; inputs are borrowed only for the call and outputs are caller-owned. Bounded-decode callbacks run synchronously and their state remains borrowed until return |
| `ctex_texture_export_get_built_in_preset_ids`, `ctex_texture_export_run` | Stateless and safe to call concurrently; callback state belongs to the host and must support the host's chosen concurrency |
| `ctex_project_container_create_empty`, `ctex_project_container_probe_version`, `ctex_project_container_normalize`, `ctex_project_container_save_atomic`, `ctex_project_asset_export`, `ctex_project_asset_install` | Stateless and safe to call concurrently. Saves to distinct paths are independent; callers serialize saves to the same destination when publication order matters |
| `ctex_project_autosave_session_*` | Distinct sessions are independent. Submission, wait, flush, status queries and destruction are serialized per session; destruction waits for an active write but discards merely pending work. The worker owns submitted snapshots until publication or destruction |
| `ctex_project_recovery_enumerate`, `ctex_project_recovery_read` | Stateless and safe to call concurrently. Hosts serialize these calls with writers targeting the same recovery path when they require a particular publication order |
| `ctex_material_graph_library_create_empty`, `ctex_material_graph_library_inspect`, `ctex_material_graph_library_add_preset`, `ctex_material_graph_library_resolve_preset` | Stateless and safe to call concurrently; serialized inputs are borrowed only for the call and all canonical, graph and report outputs are caller-owned |
| `ctex_smart_material_inspect`, `ctex_smart_material_set_parameter`, `ctex_smart_material_set_anchor`, `ctex_smart_material_add_anchor_reference`, `ctex_smart_material_plan_anchor_evaluation`, `ctex_smart_material_package`, `ctex_smart_material_import` | Stateless and safe to call concurrently; all returned storage is caller-owned |
| `ctex_preset_library_enumerate`, `ctex_preset_library_resolve` | Stateless and safe to call concurrently; descriptors and encoded shelf contents are borrowed only for the call and outputs are caller-owned |
| `ctex_texture_set_apply_smart_material`, `ctex_texture_set_apply_smart_mask`, `ctex_texture_set_get_preset_applications`, `ctex_texture_set_set_applied_entry_state`, `ctex_texture_set_undo_last_preset_application` | Distinct documents are independent; callers serialize these operations with every other operation on the same document |
| `ctex_mesh_create_with_tangent_data`, `ctex_mesh_replace_with_tangent_data`, `ctex_mesh_get_tangent_frame`, `ctex_mesh_map_set_*`, `ctex_mesh_map_bake_session_*`, `ctex_mesh_map_bake_request_token_*` | Distinct meshes and map sets on distinct documents are independent. Callers serialize every operation and destruction on one mesh, map set or bake session with mutations of its document, keep both source handles alive until the map set is destroyed, and destroy bake sessions before their map set. Request tokens are immutable independent handles |
| `ctex_mesh_map_kind_get_name` | Stateless, process-safe and callable concurrently from any thread; the name buffer is caller-owned |
| `ctex_mesh_map_generator_get_info`, `ctex_mesh_map_generator_generate` | Catalogue queries are stateless. Generator evaluation is read-only and may run concurrently only when the source map set and its parent document are not being mutated |
| `ctex_texture_set_query_channel_delta`, `ctex_texture_set_reset_channel_revision_history` | Distinct documents are independent; callers serialize these operations with every other operation on the same document. Query output contains metadata only and performs no pixel readback |
| `ctex_transport_snapshot_pool_create`, `ctex_transport_snapshot_pool_destroy`, `ctex_transport_snapshot_pool_get_memory_report`, `ctex_texture_set_query_channel_snapshot`, `ctex_paint_preview_session_query_snapshot`, `ctex_transport_snapshot_destroy`, `ctex_transport_snapshot_get_tile_versions`, `ctex_transport_snapshot_negotiate_format`, `ctex_transport_snapshot_get_tile_memory_layout`, `ctex_transport_snapshot_read_tiles`, `ctex_transport_snapshot_begin_readback`, `ctex_transport_snapshot_begin_host_readback`, `ctex_transport_readback_destroy`, `ctex_transport_readback_get_info`, `ctex_transport_readback_complete_host`, `ctex_transport_readback_cancel`, `ctex_transport_readback_fail` | Distinct pools, snapshots and readbacks are independent. Callers serialize query/report operations on one pool, calls that use one public snapshot handle, and all operations or destruction on one readback. A document or preview session is required only during its snapshot query. An admitted snapshot owns its pinned versions and may outlive the source handle and pool; a readback retains that pin and may outlive destruction of the public snapshot handle |
| `ctex_executor_registry_create`, `ctex_executor_registry_destroy`, `ctex_executor_registry_get_count`, `ctex_executor_registry_get_info`, `ctex_executor_registry_select`, `ctex_executor_registry_pin_default`, `ctex_executor_registry_clear_default` | Distinct registries are independent. Callers serialize selection/default changes and destruction of one registry; read-only count and descriptor queries may run concurrently when no operation mutates that registry |
| `ctex_executor_make_fallback_report` | Stateless and safe to call concurrently; input strings are borrowed only for the call and the report buffer is caller-owned |
| `ctex_cpu_execute_bounded`, `ctex_cpu_execution_result_destroy`, `ctex_cpu_execution_result_get_info` | Distinct executions and immutable result handles are independent. Work callbacks may run concurrently up to the declared worker bound; cancellation and progress callbacks are serialized; commit runs once on the calling thread. A result may be queried concurrently, but destruction requires that no query is active |
| `ctex_cpu_reference_rasterize_viewport`, `ctex_cpu_reference_rasterize_uv` | Stateless and safe to call concurrently; mesh and camera inputs are borrowed only for the call and all raster arrays are caller-owned |
| `ctex_executor_parity_get_tolerance`, `ctex_executor_compare_parity` | Stateless and safe to call concurrently; comparison inputs are borrowed only for the call and output storage is caller-owned |
| `ctex_executor_run_parity_gate`, `ctex_parity_gate_result_destroy`, `ctex_parity_gate_result_get_info` | Distinct runs and immutable result handles are independent. The registry, fixture descriptors and callback state must remain valid for the run; callbacks execute serially on the calling thread. A result may be queried concurrently, but destruction requires that no query is active |
| `ctex_host_execution_session_*`, `ctex_host_completion_result_*`, `ctex_host_recovery_report_*` | Distinct sessions are independent. Callers serialize submission, completion, cancellation, recovery, queries and destruction on one session. Completion-result and recovery-report handles are immutable after creation; each may be queried concurrently, but destruction requires that no query is active |
| `ctex_paint_dilation_session_create`, `ctex_paint_dilation_session_destroy`, `ctex_paint_dilation_session_stage_tile`, `ctex_paint_dilation_session_get_preview`, `ctex_paint_dilation_session_finish` | Distinct sessions are independent and may be used concurrently; callers serialize staging, preview, finish and destruction of the same session |
| `ctex_paint_surface_map_cache_create`, `ctex_paint_surface_map_cache_destroy`, `ctex_paint_surface_map_cache_clear`, `ctex_paint_surface_map_cache_get_statistics`, `ctex_paint_surface_map_cache_lookup` | Distinct caches are independent and may be used concurrently; callers serialize lookup, statistics, clearing and destruction of the same cache, and keep each mesh alive for its lookup call |
| `ctex_paint_preview_session_create`, `ctex_paint_preview_session_destroy`, `ctex_paint_preview_session_write_pixel`, `ctex_paint_preview_session_get_info`, `ctex_paint_preview_session_get_pixels`, `ctex_paint_preview_session_get_changed_tiles`, `ctex_paint_preview_session_finalize`, `ctex_paint_preview_session_commit`, `ctex_paint_preview_session_cancel` | Distinct sessions on distinct documents are independent; callers serialize every operation on a session and every operation on its document, and keep the document alive through session destruction |
| `ctex_cube_lut_create` | Process-safe; each successful call creates independent immutable state and captures the active allocator |
| `ctex_cube_lut_apply_preview` | Safe to call concurrently, including against the same immutable LUT handle |
| `ctex_cube_lut_destroy` | The caller ensures no application call is using that handle; distinct handles may be destroyed concurrently |
| `ctex_set_log_sink` | Process-safe process-wide setting; replacement is atomic, but the host keeps callback user data alive until calls that could have observed the prior sink finish |
| `ctex_set_allocator` | Process-safe process-wide default for subsequently created objects; each object retains its creating configuration, and the host keeps its user data alive through destruction |
| `ctex_document_create` | Process-safe; each successful call creates independent state |
| `ctex_document_destroy` | The caller ensures no other call is using that handle; distinct handles may be destroyed concurrently |
| `ctex_material_graph_workspace_create` | Process-safe; each successful call creates an independent workspace and captures the active allocator |
| `ctex_material_graph_workspace_destroy`, `ctex_material_graph_workspace_get_info`, `ctex_material_graph_workspace_add_material`, `ctex_material_graph_workspace_create_group`, `ctex_material_graph_workspace_instantiate_group`, `ctex_material_graph_workspace_update_group_interface`, `ctex_material_graph_workspace_get_graph`, `ctex_material_graph_workspace_add_builtin_node`, `ctex_material_graph_workspace_set_input_value`, `ctex_material_graph_workspace_set_property_value`, `ctex_material_graph_workspace_add_link` | Calls on distinct workspaces are independent. Every operation on one workspace, including reads and destruction, requires external serialization |
| `ctex_material_graph_node_registry_create` | Process-safe; each successful call creates an independent registry and captures the active allocator |
| `ctex_material_graph_node_registry_destroy`, `ctex_material_graph_node_registry_get_info`, `ctex_material_graph_node_registry_register`, `ctex_material_graph_add_registered_node`, `ctex_material_graph_workspace_add_registered_node`, `ctex_material_graph_validate_registered`, `ctex_material_graph_node_registry_verify_contract` | Calls on distinct registries are independent. Registration and destruction require exclusive access; read-only operations may run concurrently when no registration is active. A workspace insertion also requires exclusive access to that workspace. Host callbacks obey the concurrency chosen by the caller and their borrowed request/result storage is valid only for the callback |
| `ctex_shader_emit_material`, `ctex_shader_emit_material_inspectable`, `ctex_shader_emit_layer_stack`, `ctex_shader_emit_lit_preview`, `ctex_shader_emit_channel_inspection` with a null cache | Built-in-only serialized-graph calls are stateless and safe to invoke concurrently. Registry-backed calls may run concurrently while that registry is not being registered into or destroyed; callback state must support the caller's chosen concurrency. An inspectable workspace source requires external serialization against every operation and destruction of that workspace. Graph, request, workspace, and registry storage is borrowed only for the call, and every output is caller-owned |
| `ctex_shader_get_backend_attribution` | Stateless, process-safe and callable concurrently from any thread; the report buffer is caller-owned |
| `ctex_shader_emission_cache_create` | Process-safe; each successful call creates an independent cache and captures the active allocator |
| `ctex_shader_emission_cache_destroy`, `ctex_shader_emission_cache_get_info`, `ctex_shader_emission_cache_clear`, `ctex_shader_emit_material_cached`, `ctex_shader_emit_material_inspectable`, `ctex_shader_emit_layer_stack`, `ctex_shader_emit_lit_preview`, `ctex_shader_emit_channel_inspection` with a cache | Emission and statistics queries may run concurrently on one cache. Clearing or destruction requires exclusive access. Registries, callback state, and inspectable workspace sources follow their contracts above; all request storage is borrowed only for the call and outputs are caller-owned |
| `ctex_document_create_texture_set`, `ctex_document_create_texture_sets_from_mesh`, `ctex_document_get_texture_set_ids`, `ctex_texture_set_*` | Calls on distinct document handles are safe concurrently; every call on the same document handle must be externally synchronized, including read-only calls. Mesh-derived creation also requires no concurrent use of that mesh handle |
| `ctex_texture_set_editable_entry_*`, `ctex_texture_set_editable_surface_path_resolve`, `ctex_project_container_*_editable_authoring` | Calls touching distinct documents are independent. Every edit, history operation, inspection, path resolution, save or restore on one document requires external serialization. Descriptors and output buffers are borrowed only for the call; retained source data is copied into the document |
| `ctex_mesh_create` | Process-safe; each successful call creates independent owned state and captures the active allocator |
| `ctex_mesh_destroy`, `ctex_mesh_replace`, `ctex_mesh_get_info`, `ctex_mesh_get_uv_set_names`, `ctex_mesh_analyze_uv_overlaps`, `ctex_mesh_analyze_uv_coverage` | Calls on distinct mesh handles are safe concurrently; every call on the same mesh handle must be externally synchronized, including read-only calls |
| `ctex_mesh_replacement_plan_*` | Plans over distinct document/mesh pairs are independent. The source document and mesh must outlive the plan; callers serialize plan queries, apply and destruction with every operation on either source handle. Apply consumes the candidate mesh only when `replacement_applied` is nonzero |
| `ctex_pick_ray_from_screen` | Stateless, process-safe and callable concurrently from any thread |
| `ctex_pick_index_create`, `ctex_uv_pick_index_create` | Process-safe when no concurrent call mutates or destroys the supplied mesh; each successful call creates independent index state and captures the active allocator |
| `ctex_pick_index_destroy`, `ctex_uv_pick_index_destroy`, `ctex_pick_index_get_info`, `ctex_uv_pick_index_get_info`, `ctex_pick_ray_query`, `ctex_pick_uv_query`, `ctex_pick_snap_to_surface`, `ctex_pick_query_*`, `ctex_pick_nearest_batch`, `ctex_paint_apply_particles`, `ctex_paint_select_screen` | Calls on distinct indexes over idle or distinct meshes are safe concurrently. Every call on the same index or its mesh must be externally serialized; the mesh must outlive the index and callbacks must remain valid for the batch call |
| `ctex_get_last_result`, `ctex_get_last_diagnostic_code`, `ctex_get_last_diagnostic` | Thread-local; concurrent threads never observe or replace one another's diagnostic state |

A handle may move between threads while idle. CyberTexel does not attach thread
affinity to a document, but it does not lock operations on the same document;
the caller owns that serialization. The `c-abi-two-document-concurrency` test
starts two workers together, creates and enumerates 256 texture sets on each
independent document, and verifies that a failure diagnostic in one worker does
not alter the successful worker's state.

After a failed call, `ctex_get_last_result` returns the same result and
`ctex_get_last_diagnostic` returns an English message naming the operation and
the offending value. `ctex_get_last_diagnostic_code` returns a stable
`ctex_diagnostic_code` for localization and program logic; hosts must not parse
the English prose. Existing code names and numeric values remain fixed within an
ABI major, and new codes are appended. Diagnostic state belongs to the calling
thread. The message is owned by CyberTexel, requires no caller allocation, and
remains valid until the next fallible C entry point on that thread. A successful
fallible call clears the result, code, and message.

## Host logging

`ctex_set_log_sink` installs one process-wide callback using a versioned
`ctex_log_sink_descriptor`. Passing a null descriptor uninstalls it. The
descriptor atomically pairs the callback, opaque user-data pointer, and minimum
`ctex_log_severity`; messages below that threshold are discarded. C API
failures are sent at error severity in the `capi.diagnostic` category after the
thread-local result, stable code, and English message have been populated.

The sink may be called concurrently from the thread entering the library or
from a library worker thread, so it must be thread-safe. It may inspect the
thread-local diagnostic getters and may replace the sink reentrantly. The host
must keep a replaced sink's user data valid until calls that could already have
observed it finish. Exceptions thrown by a C++ callback are contained at the
boundary and never change the operation result. Without an installed sink,
CyberTexel emits no log output to standard output, standard error, or another
implicit destination.

## Host allocator

`ctex_set_allocator` installs a versioned process-wide allocator descriptor for
objects created afterward; a null descriptor restores the library default. The
descriptor pairs allocation and deallocation callbacks with opaque user data.
Both callbacks receive the exact requested byte count and alignment. A null
allocation result becomes `CTEX_RESULT_OUT_OF_MEMORY`; an incorrectly aligned
result is returned to the host deallocator and refused as
`CTEX_DIAGNOSTIC_ALLOCATOR_CONTRACT_VIOLATION`.

Every opaque document captures the allocator active when it is created and uses
that same callback, size, alignment, and user-data tuple at destruction. The
process default may therefore change while older documents remain alive. The
callbacks may run on the calling thread or a library worker thread, must be
thread-safe, and must not throw. User data remains host-owned and must outlive
all objects and work that captured it.

Opaque document storage and every persistent allocation reachable through the
current C surface use the captured allocator through a core
`std::pmr::memory_resource`. This includes the ordered texture-set index and
keys, texture-set identity and descriptor strings, shared accounting state,
preset-vector capacity, channel descriptor/map storage, tiled channel-image
metadata and pixels, opaque `.cube` LUT handles and sample tables, and opaque
mesh handles with their copied geometry, UV, partition and material buffers.
Opaque picking indexes route their BVH nodes, triangle order and retained UV
name through the allocator captured by the index handle.
Copy-on-write image versions retain that resource across copy and move
publication. Native C++ callers use the standard default resource unless they
provide another one. Temporary conversion and scratch allocations are
intentionally outside the long-lived allocation contract. Any future C entry
point that creates persistent state must propagate its captured resource;
allocating such state from the process default is a contract violation.

The Linux export surface is constrained by `cmake/exports/cybertexel.map`, macOS
uses `cybertexel.exports`, and Windows uses `cybertexel.def`. The
`c-abi-export-surface` test inspects the produced Linux shared object and refuses
any visible symbol outside `ctex_*`.
