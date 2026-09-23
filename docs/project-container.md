# Project container

CyberTexel's project container is a little-endian, length-delimited binary
format. Its fixed 40-byte header can be probed without decoding or even loading
the body. The schema version is `CTEX_VERSION_MAJOR.MINOR.PATCH`, using the same
generated version constants as the library and every binding manifest.

| Header field | Type | Meaning |
| --- | --- | --- |
| Magic | 8 bytes | `CTEXPRJ\0` |
| Header bytes | `u32` | Offset of the first section; currently 40 |
| Schema major/minor/patch | three `u32` | Writer schema version |
| Section count | `u32` | Number of framed body sections |
| Reserved | `u32` | Zero in this version |
| Body bytes | `u64` | Exact byte length after the header |

Each body section is framed as `kind: u32`, `version: u32`, `payload-bytes:
u64`, then that many payload bytes. An older reader decodes section kinds and
versions it understands and retains every other section as an
`OpaqueContainerSection`. `ContainerReadReport` reports the source schema,
whether it is newer than the reader, and every opaque section. Re-saving writes
the opaque payload and its numeric identifiers back unchanged. Additional
header bytes are skipped according to `header-bytes`, allowing a future schema
to extend the header without moving the version fields.

## Tiled pixel section

Section kind 1, version 1 stores zero or more named tiled images. Image metadata
contains the resource identity, canvas dimensions, tile size, channel type and
count, and clear pixel. Only allocated tiles are serialized. Each tile records
its coordinate and edge-aware extent, compression identifier, decoded byte
count, encoded byte count and encoded payload.

The version-1 writer uses zlib-wrapped DEFLATE (`TileCompression::zlib_deflate`)
independently for every tile. This keeps random tile access possible and makes
the compression algorithm explicit for future readers. Pixel rows are tightly
packed in each record; unused padding from an edge tile is not stored. A 16384
canvas with one allocated 64x64 corner tile therefore contains one tile record,
not 65,536 records or a full-canvas buffer.

`snapshot_tiled_image` captures allocated tiles from `image::TiledImage`, and
`restore_tiled_image` reconstructs the canvas, clear value and pixel contents.
The reader verifies coordinates, exact edge extents, decoded sizes, duplicate
identities and compression output before publishing a decoded image. Unknown
tile encodings preserve the complete section opaquely instead of guessing.

## Project resource section

Section kind 2, version 1 stores external resources independently of tiled
document pixels. Every record has a stable identifier, a caller-defined kind
such as `image`, `font`, `mesh-map`, or `mesh`, and a relative path. Resources
are referenced by that path by default. Setting `ProjectResource::packed_bytes`
embeds the exact payload while retaining the original path for later unpacking
or source attribution; an empty packed payload remains distinguishable from an
unpacked reference.

`resolve_project_resources` resolves references relative to the directory that
contains the project. Packed resources do not access the filesystem. A missing
or unreadable referenced file does not prevent the container from opening: its
resolved status is `missing`, its identifier is included in
`missing_identifiers`, and other resources remain available. Absolute paths,
paths containing a parent (`..`) component, empty metadata, and duplicate
identifiers are refused so a project cannot accidentally bind outside its
portable directory layout.

Resource count, string length, and individual packed-payload limits are
configurable through `ProjectContainerReadLimits`. An unknown storage encoding
causes the complete resource section to be reported and retained opaquely for a
lossless re-save.

## Standalone asset section

Section kind 3, version 1 stores versioned standalone asset records. Each record
has a stable identifier, an extensible string kind, an asset-format version, an
opaque payload, and explicit lists of project-resource and tiled-image
dependencies. The built-in kind vocabulary covers materials, smart materials,
smart masks, brushes, stroke presets, export presets, and node groups. Payloads
remain owned by those domain serializers; the container preserves kinds and
versions it does not yet interpret.

Asset and dependency counts, payload bytes, and all identifier lengths use the
same configurable read limits and checked framing as the other sections.
Duplicate asset identities or dependencies, empty metadata, and version zero
are refused.

### Texture-document assets

The built-in `texture-document` asset is the durable bridge to
`doc::TextureDocument`. Its versioned canonical payload records stable texture
set descriptors, the complete channel catalogue and enablement, UDIM tile
membership, ordered layer entries and material graphs, applied-preset origins
and parameter state, editable authoring entries, and atlas layouts. Channel
pixels are not duplicated in the payload: each enabled base or UDIM channel
names one sparse image in the tiled pixel section and lists it as an explicit
asset dependency.

`upsert_texture_document` builds the replacement completely before publishing
it to an in-memory `ProjectContainer`, rejects graph resources that lack project
resource metadata, removes only the prior document's exclusively owned pixel
dependencies, and verifies the resulting container. Repeating an unchanged
upsert produces byte-identical project bytes. `unpack_texture_document` checks
the asset schema, stable identities, nested counts, enum values, finite numeric
state, layer and atlas invariants, exact pixel dependencies, and the domain
serializers for graphs, smart-material fragments, and editable entries before
returning a live document.

`list_texture_documents` validates each document asset and reports its texture
sets, tiled images, layer entries, atlases, editable entries, and applied
presets. The headless `info` command includes those live document totals in
both text and JSON output.

### Document mesh-state assets

A versioned `document-mesh-state` companion asset associates one texture
document with its source `mesh` project resource, current mesh revision and
bound mesh maps. Each map records its texture set, UV set, kind, producing mesh
revision, optional normal convention and tangent frame. Map pixels use the same
sparse compressed tiled-image section as authored channels rather than being
duplicated in the asset payload.

`upsert_document_mesh_state` validates the referenced document, mesh resource,
texture-set/UV identities, revisions and unique map kinds before atomically
replacing the companion asset. Replacing it reclaims only its unshared old map
images. `read_document_mesh_state` distinguishes an absent binding from a
malformed one, enforces payload, string, count and total decoded-map-byte
ceilings before allocation, and requires the declared image dependencies to
match exactly. A referenced mesh may be missing on disk: the container still
opens and the existing project-resource resolver reports that external state.

## Untrusted input limits

`ProjectContainerReadLimits` caps the complete encoded input, aggregate
parser-owned decoded allocations, section and record counts, string lengths,
and each decoded tile or embedded payload. Counts are checked both across all
repeated sections and against the bytes remaining in the current section
before any `reserve` or payload allocation. Declared section and payload sizes
must fit the already supplied input before they are copied or decompressed.

Limit violations return `ProjectContainerErrorCode::over_limit`; impossible or
truncated framing returns `malformed_header` or `malformed_section`. The reader
therefore does not allocate from an untrusted declared size that has not first
been bounded and proven to fit its enclosing input.

## Atomic and deterministic save

`save_project_container_atomic` encodes the complete project before touching
the destination, writes it to an exclusively created sibling temporary file,
synchronizes that file, and atomically replaces the destination. It then
synchronizes the containing directory on POSIX systems; Windows publication
uses replace-existing and write-through semantics. A serialization, write, or
publication failure removes its temporary file and leaves an existing project
untouched. If the process is interrupted before publication, only the sibling
temporary file can be incomplete—the prior project remains at its original
path.

Container encoding has no timestamps, random identifiers, filesystem metadata,
or iteration over unordered collections. Saving the same in-memory container
twice therefore publishes byte-identical files; the project-save determinism
gate exercises the atomic filesystem API rather than only the memory encoder.

Periodic background publication and restart discovery build on this format and
are described in [Project snapshots, autosave, and recovery](project-autosave.md).

## C ABI access

The public C boundary exposes header probing, canonical empty-container
creation, bounded open/re-encode with a JSON inventory, and atomic filesystem
publication. `ctex_project_container_normalize` reports exact caller-buffer
sizes before writing either the canonical bytes or NUL-terminated report, and
preserves newer-schema and opaque sections. Passing a null read-limits
descriptor uses `ProjectContainerReadLimits` defaults; a supplied versioned
descriptor controls every input, allocation, record, string, tile, resource and
asset ceiling. See [C ABI](c-abi.md#project-containers) for the complete calling
contract.

Standalone assets are also available without exposing C++ objects.
`ctex_project_asset_export` produces either a referenced or self-contained
package containing one asset and its exact dependencies, while
`ctex_project_asset_install` resolves any referenced resources through caller
search paths and atomically returns an updated encoded library container.

Versioned [operation records](operation-records.md) use the standalone asset
section with kind `operation-record`. Their payload owns pinned replay inputs,
while their tiled-image dependencies name raster checkpoints stored in the same
container. Dedicated C entry points create, inspect, insert, replace, and
retrieve these records without exposing C++ objects.

The in-memory `ProjectContainer` is the single framing for sparse tiled pixels,
portable resources, recovery checkpoints and versioned domain payloads. Domain
serializers own their payload schemas, while the container preserves their
identifiers, kinds, versions, dependencies and bytes losslessly. The native
texture-document round-trip fixture exercises live texture sets, custom channel
descriptors and pixels, UDIM state, layers and graphs, applied presets, editable
entries, and atlas settings in one canonical project. Other portable payloads,
including operation records and standalone presets, continue to use their own
versioned asset kinds in the same container.
