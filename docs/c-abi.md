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
