# Image input and output

The image path decodes PNG, flat OpenEXR and Radiance HDR entirely from
caller-owned byte buffers and encodes PNG the same way. Detection uses the file
signature; a misleading extension is reported but does not select the decoder.
Signatures for JPEG, BMP, TIFF and PSD are named as unsupported until their
decoder tasks land.

PNG grayscale, grayscale-alpha, RGB and RGBA data retain 8- or 16-bit channel
precision. Sub-byte grayscale expands to 8-bit and palette input expands to
8-bit RGBA. A transparency key expands grayscale or RGB to an alpha-bearing
layout. Sixteen-bit samples are stored in native byte order inside CyberTexel
and converted to PNG network order only at the codec boundary.

Destination expansion is explicit and never changes component precision.
Single-channel grayscale replicates into RGB; when RGBA is requested it also
gains an opaque alpha. Two-channel input is interpreted as grayscale-alpha and
expands to replicated RGB with the original alpha. RGB expands to RGBA with an
opaque alpha. Identity mappings copy unchanged. Shrinking and ambiguous
two-channel-to-RGB conversion are refused rather than dropping information.
These rules apply equally to 8-bit and 16-bit unsigned-normalized components and
32-bit floating-point components.

Import resizing offers nearest-neighbour and pixel-centred bilinear filters.
Pixel-centred bilinear is the default: destination texel centres map into source
texel space, edge coordinates clamp to the nearest source edge, integer results
round to the nearest representable component, and floating-point values remain
unclamped. Nearest-neighbour is available for identifiers, masks or deliberately
hard-edged inputs. Every result records the resolved filter, including when the
caller requests the default. Resampling accepts row-strided input, writes tightly
packed output and refuses the operation before allocation when its declared or
default 1 GiB output ceiling would be exceeded.

Decode limits are checked from the PNG header before pixel allocation. The
default ceiling is 16384×16384 and 1 GiB of decoded pixels; hosts can lower each
limit. Truncated or malformed data produces a named error and no partial image.

Flat OpenEXR and Radiance HDR headers are inspected against the same limits
before pixel allocation. They decode to native float32 storage without clamping:
OpenEXR expands named colour channels to RGBA and Radiance HDR retains RGB.
Automatic colour interpretation is linear Rec. 709; an explicit caller
declaration remains authoritative. Multipart and deep OpenEXR inputs are refused
until the layered-source API in task 2.6 lands.

An explicit caller colour-space declaration overrides metadata. Otherwise an
embedded PNG sRGB declaration is used, followed by the automatic semantic rule.
Unsupported ICC profiles are reported before the automatic rule is applied.

LodePNG is pinned for this path because it supports memory-based 8/16-bit PNG
encoding and decoding without another runtime dependency. Its revision and
licence are recorded in the dependency manifest and third-party notices.
Radiance HDR uses the pinned stb decoder. Flat OpenEXR uses pinned TinyEXR and
its bundled miniz implementation; both licence texts are recorded with the
dependency.

The texture-export path additionally encodes PNG, JPEG, TGA, TIFF and OpenEXR
to caller-owned buffers. Its exact format/depth compatibility table and output
semantics are documented in [Export presets and channel tokens](export-presets.md).
JPEG and TGA use the pinned stb image writer; the baseline TIFF and uncompressed
scanline OpenEXR writers are implemented locally.
