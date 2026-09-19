# Texture export execution

`export_textures_to_memory()` turns the complete preflight manifest from
`plan_texture_export()` into encoded caller-owned buffers. It never opens or
writes a path. Each buffer carries its relative path, dimensions, image format,
colour space and bit depth, and also points to the matching report entry. A host
can publish those bytes atomically, send them to another process, or keep them
entirely in memory.

## Resolution and padding

`ExportPlanRequest::output_resolution` overrides the working resolution for
every planned output. Both dimensions must be non-zero. Export uses
pixel-centred bilinear sampling, with edge coordinates clamped to the source
image. Coverage is resized with nearest-neighbour sampling because it is a
discrete UV ownership mask.

Padding runs after resize and before output colour conversion. The default is
two texels, zero disables it, and the maximum accepted radius is 4096 texels.
The implementation is the same gradient-extrapolating UV dilation used by the
paint engine. An omitted `ExportPixelSource::coverage` means every source texel
is covered.

## Dry runs and reports

Setting `TextureExportOptions::dry_run` returns the full manifest without
requesting any source pixels or producing encoded buffers. Every entry includes
the predicted path, contributing texture sets, optional UDIM or atlas identity,
preset entry, dimensions, format, colour space, bit depth and a conservative
encoded-size estimate.

`texture_export_report_json()` serializes the same report deterministically.
Completed entries additionally contain their actual encoded byte count. JPEG
entries record the requested quality from 1 through 100; other formats report
`null` for that field. Entry state is `planned`, `encoded`, or `not_started`,
and the top-level report records whether the operation was a dry run or was
cancelled.

Progress is delivered once per completely encoded output. Cancellation is
checked while sampling, resizing and converting rows and between outputs. The
current output is appended only after encoding completes, so the result never
contains a partial buffer.

## Channel and colour handling

The pixel provider supplies an `ExportChannelSample` for each source texel.
Preset tokens select or derive the four output components. Colour transfer is
applied only to colour-semantic tokens such as base colour, emission, diffuse
and specular. Scalar data, mesh maps and registered channel components remain
data values even when packed beside colour components in an sRGB output.

JPEG quality is validated before any pixel request. Invalid padding, missing
providers, malformed dimensions or coverage, non-finite samples, impossible
format/depth pairs, invalid scopes and filename collisions are likewise refused
before a partial result is published.
