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

The current in-memory `ProjectContainer` is the extensible framing and tiled
pixel foundation. Resource packing, atomic filesystem publication, autosave,
standalone assets and the complete document object schema are added by the
subsequent project-I/O roadmap tasks.
