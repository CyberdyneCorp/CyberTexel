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

`ctex_mesh_replace` validates and copies a complete replacement before publishing
it. Success advances the globally unique revision; failure preserves both the
prior mesh and revision. The maximum supported mesh has 100,000,000 vertices and
100,000,000 triangles, exposed as `CTEX_MAX_MESH_VERTEX_COUNT` and
`CTEX_MAX_MESH_TRIANGLE_COUNT`. Both boundaries reject larger declared counts
before reading array contents and report `CTEX_DIAGNOSTIC_MESH_LIMIT_EXCEEDED`
with the count, supplied value and maximum.

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
snapshots. Host-device pending/completion remains outside this slice.

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
format from content. The current decoder slice accepts PNG, flat OpenEXR and
Radiance HDR, and refuses detected JPEG, BMP, TIFF and PSD by name until their
scheduled decoders land. A mismatched filename extension is reported without
changing the content-selected format.

The output uses the standard two-call caller-buffer contract and is tightly
packed, row-major and interleaved; 16-bit components use native byte order.
`ctex_decoded_image_info` reports dimensions, channel count, scalar type, native
bit depth, colour space and its source, detected format, extension mismatch and
an uninterpretable-profile flag. Explicit caller colour declarations and the
automatic per-channel rule use the same enums as the colour-management API.
OpenEXR expands to RGBA float32 and Radiance HDR retains RGB float32; both use
linear Rec. 709 in automatic mode and preserve finite values outside `[0, 1]`.

Optional `ctex_image_decode_limits_descriptor` values cap width, height and
decoded bytes before pixel allocation. Malformed or truncated data is refused
without returning partial pixels; limit, unsupported-format and invalid-data
failures have distinct stable diagnostic codes.

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

`ctex_project_container_save_atomic` applies the same validation and limits,
then publishes the normalized container through the core sibling-temporary,
synchronize and atomic-replace path. It does not write a destination until the
whole input has parsed and re-encoded successfully. Repeated normalization or
save of unchanged input is byte-identical.

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
| `ctex_image_decode_memory`, `ctex_image_expand_channels`, `ctex_image_resample`, `ctex_image_encode_memory`, `ctex_stroke_settings_init`, `ctex_stroke_resolve`, `ctex_stroke_preset_serialize`, `ctex_stroke_preset_deserialize`, `ctex_paint_evaluate_tile_coverage`, `ctex_paint_evaluate_material_coordinates`, `ctex_paint_rejection_init`, `ctex_paint_evaluate_rejected_coverage`, `ctex_paint_work_init`, `ctex_paint_plan_work`, `ctex_paint_seam_dilation_init`, `ctex_paint_dilate_uv_seams`, `ctex_paint_filter_surface_scalar`, `ctex_paint_filter_surface_tangent_vector`, `ctex_paint_plan_island_padding`, `ctex_paint_apply_island_padding`, `ctex_paint_combine_masks`, `ctex_paint_evaluate_tile_deposition`, `ctex_paint_blend_snapshot`, `ctex_paint_apply_brush`, `ctex_paint_apply_eraser`, `ctex_paint_apply_fill`, `ctex_paint_apply_clone`, `ctex_paint_apply_blur`, `ctex_paint_apply_smear`, `ctex_paint_resolve_stencil_mask`, `ctex_paint_rasterize_decal`, `ctex_paint_apply_projection`, `ctex_paint_apply_text` | Stateless and safe to call concurrently; inputs are borrowed only for the call and outputs are caller-owned |
| `ctex_texture_export_get_built_in_preset_ids`, `ctex_texture_export_run` | Stateless and safe to call concurrently; callback state belongs to the host and must support the host's chosen concurrency |
| `ctex_project_container_create_empty`, `ctex_project_container_probe_version`, `ctex_project_container_normalize`, `ctex_project_container_save_atomic`, `ctex_project_asset_export`, `ctex_project_asset_install` | Stateless and safe to call concurrently. Saves to distinct paths are independent; callers serialize saves to the same destination when publication order matters |
| `ctex_smart_material_inspect`, `ctex_smart_material_set_parameter`, `ctex_smart_material_set_anchor`, `ctex_smart_material_add_anchor_reference`, `ctex_smart_material_plan_anchor_evaluation`, `ctex_smart_material_package`, `ctex_smart_material_import` | Stateless and safe to call concurrently; all returned storage is caller-owned |
| `ctex_preset_library_enumerate`, `ctex_preset_library_resolve` | Stateless and safe to call concurrently; descriptors and encoded shelf contents are borrowed only for the call and outputs are caller-owned |
| `ctex_texture_set_apply_smart_material`, `ctex_texture_set_apply_smart_mask`, `ctex_texture_set_get_preset_applications`, `ctex_texture_set_set_applied_entry_state`, `ctex_texture_set_undo_last_preset_application` | Distinct documents are independent; callers serialize these operations with every other operation on the same document |
| `ctex_texture_set_query_channel_delta`, `ctex_texture_set_reset_channel_revision_history` | Distinct documents are independent; callers serialize these operations with every other operation on the same document. Query output contains metadata only and performs no pixel readback |
| `ctex_transport_snapshot_pool_create`, `ctex_transport_snapshot_pool_destroy`, `ctex_transport_snapshot_pool_get_memory_report`, `ctex_texture_set_query_channel_snapshot`, `ctex_paint_preview_session_query_snapshot`, `ctex_transport_snapshot_destroy`, `ctex_transport_snapshot_get_tile_versions`, `ctex_transport_snapshot_negotiate_format`, `ctex_transport_snapshot_get_tile_memory_layout`, `ctex_transport_snapshot_read_tiles` | Distinct pools and snapshots are independent. Callers serialize query/report operations on one pool and all operations or destruction on one snapshot. A document or preview session is required only during its snapshot query and must be externally serialized for that call; an admitted snapshot owns its pinned versions and may outlive the source handle and pool |
| `ctex_executor_registry_create`, `ctex_executor_registry_destroy`, `ctex_executor_registry_get_count`, `ctex_executor_registry_get_info`, `ctex_executor_registry_select`, `ctex_executor_registry_pin_default`, `ctex_executor_registry_clear_default` | Distinct registries are independent. Callers serialize selection/default changes and destruction of one registry; read-only count and descriptor queries may run concurrently when no operation mutates that registry |
| `ctex_executor_make_fallback_report` | Stateless and safe to call concurrently; input strings are borrowed only for the call and the report buffer is caller-owned |
| `ctex_cpu_execute_bounded`, `ctex_cpu_execution_result_destroy`, `ctex_cpu_execution_result_get_info` | Distinct executions and immutable result handles are independent. Work callbacks may run concurrently up to the declared worker bound; cancellation and progress callbacks are serialized; commit runs once on the calling thread. A result may be queried concurrently, but destruction requires that no query is active |
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
| `ctex_document_create_texture_set`, `ctex_document_create_texture_sets_from_mesh`, `ctex_document_get_texture_set_ids`, `ctex_texture_set_*` | Calls on distinct document handles are safe concurrently; every call on the same document handle must be externally synchronized, including read-only calls. Mesh-derived creation also requires no concurrent use of that mesh handle |
| `ctex_mesh_create` | Process-safe; each successful call creates independent owned state and captures the active allocator |
| `ctex_mesh_destroy`, `ctex_mesh_replace`, `ctex_mesh_get_info`, `ctex_mesh_get_uv_set_names` | Calls on distinct mesh handles are safe concurrently; every call on the same mesh handle must be externally synchronized, including read-only calls |
| `ctex_pick_ray_from_screen` | Stateless, process-safe and callable concurrently from any thread |
| `ctex_pick_index_create`, `ctex_uv_pick_index_create` | Process-safe when no concurrent call mutates or destroys the supplied mesh; each successful call creates independent index state and captures the active allocator |
| `ctex_pick_index_destroy`, `ctex_uv_pick_index_destroy`, `ctex_pick_index_get_info`, `ctex_uv_pick_index_get_info`, `ctex_pick_ray_query`, `ctex_pick_uv_query`, `ctex_pick_snap_to_surface`, `ctex_pick_query_*`, `ctex_pick_nearest_batch` | Calls on distinct indexes over idle or distinct meshes are safe concurrently. Every call on the same index or its mesh must be externally serialized; the mesh must outlive the index and callbacks must remain valid for the batch call |
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
