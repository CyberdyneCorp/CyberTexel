# Image input and output

The slice-A image path decodes and encodes PNG entirely from caller-owned byte
buffers. Detection uses the file signature; a misleading extension is reported
but does not select the decoder. Signatures for JPEG, BMP, TIFF, OpenEXR,
Radiance HDR and PSD are named as unsupported until their decoder tasks land.

PNG grayscale, grayscale-alpha, RGB and RGBA data retain 8- or 16-bit channel
precision. Sub-byte grayscale expands to 8-bit and palette input expands to
8-bit RGBA. A transparency key expands grayscale or RGB to an alpha-bearing
layout. Sixteen-bit samples are stored in native byte order inside CyberTexel
and converted to PNG network order only at the codec boundary.

Decode limits are checked from the PNG header before pixel allocation. The
default ceiling is 16384×16384 and 1 GiB of decoded pixels; hosts can lower each
limit. Truncated or malformed data produces a named error and no partial image.

An explicit caller colour-space declaration overrides metadata. Otherwise an
embedded PNG sRGB declaration is used, followed by the automatic semantic rule.
Unsupported ICC profiles are reported before the automatic rule is applied.

LodePNG is pinned for this path because it supports memory-based 8/16-bit PNG
encoding and decoding without another runtime dependency. Its revision and
licence are recorded in the dependency manifest and third-party notices.

The texture-export path additionally encodes PNG, JPEG, TGA, TIFF and OpenEXR
to caller-owned buffers. Its exact format/depth compatibility table and output
semantics are documented in [Export presets and channel tokens](export-presets.md).
JPEG and TGA use the pinned stb image writer; the baseline TIFF and uncompressed
scanline OpenEXR writers are implemented locally.
