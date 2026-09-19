# Export presets and channel tokens

An `ExportPreset` is data: it contains an identifier, display name, and output
texture records. Each output names its filename suffix, four ordered RGBA
channel tokens, output colour space, file format, and 8-, 16-, or 32-bit
channel depth. A host can create a new engine convention by constructing this
document; no engine-specific branch or registration step exists in the
library.

## Canonical token vocabulary

| Token | Value |
| --- | --- |
| `base_color.r`, `.g`, `.b` | Corresponding linear working-space base-colour component |
| `opacity` | Authored opacity |
| `roughness` | Authored perceptual roughness |
| `smoothness` | `1 - roughness` |
| `metallic` | Authored metallic value |
| `normal.x`, `.y`, `.z` | Corresponding encoded tangent-space normal component |
| `normal.directx_y` | `1 - normal.y`, converting encoded OpenGL green to DirectX green |
| `height`, `occlusion`, `subsurface` | Corresponding authored scalar channel |
| `emission.r`, `.g`, `.b` | Corresponding linear working-space emission component |
| `emission` | Linear Rec. 709 luminance: `0.2126 R + 0.7152 G + 0.0722 B` |
| `diffuse.r`, `.g`, `.b` | `base_color.component * (1 - metallic)` |
| `specular.r`, `.g`, `.b` | `0.04 * (1 - metallic) + base_color.component * metallic` |
| `0.0`, `1.0` | Exact constant zero or one |
| `mesh_map:<name>` | Component zero of the named mesh map |
| `mesh_map:<name>:<component>` | Component 0–3 of the named mesh map |
| `channel:<semantic-id>:<component>` | Component 0–3 of a registered document channel |

The dielectric specular reflectance constant is `0.04`, matching the preview
shading contract. Token parsing and formatting are strict and canonical;
missing mesh maps or registered channels produce a typed `missing_source`
diagnostic rather than a substituted neutral value.

## Built-in presets

`built_in_export_presets()` supplies the required data documents:

- `pbr-individual` — separate base colour, opacity, roughness, metallic,
  normal, height, occlusion, emission, and subsurface textures;
- `occlusion-roughness-metallic` — packed ORM;
- `metallic-occlusion-smoothness` — packed MOS;
- `metallic-emission-roughness` — packed MER, using emission luminance;
- `base-color` — base colour and opacity in one texture; and
- `specular-glossiness` — derived diffuse plus specular/smoothness textures.

`default_export_preset()` returns `pbr-individual`.

## Formats and channel depth

Each texture record selects an `ExportImageFormat`. The supported combinations
are explicit:

| Format | 8-bit unsigned normalized | 16-bit unsigned normalized | 32-bit float |
| --- | --- | --- | --- |
| PNG | yes | yes | no |
| JPEG | yes | no | no |
| TGA | yes | no | no |
| TIFF | yes | yes | yes |
| OpenEXR | no | 16-bit half float | yes |

`encode_texture_memory()` encodes one- through four-channel tiled images into
caller-owned bytes. PNG output records sRGB metadata when requested. JPEG has a
quality option from 1 through 100; when given RGBA input, JPEG necessarily
discards alpha. TGA is lossless RLE, TIFF is uncompressed baseline
little-endian, and OpenEXR uses uncompressed scanline storage.

Both `validate_export_preset()` and the encoder reject an impossible
format/depth pair before producing bytes. The typed diagnostic names both the
format and requested depth. [Texture export planning](export-planning.md)
defines texture-set, UDIM, atlas, and layer scopes plus collision-safe filename
patterns. Padding, dry runs, and the multi-output export coordinator remain
subsequent texture-export roadmap stages.
