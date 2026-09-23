# Image input and output

The image path decodes PNG, JPEG, TGA, BMP, baseline TIFF, OpenEXR,
Radiance HDR and PSD entirely from caller-owned byte buffers. It
encodes PNG, JPEG, TGA, baseline TIFF and flat OpenEXR the same way. Detection
uses signatures and validated structural markers rather than the filename; a
misleading extension is reported but does not select the decoder. Unknown
content is refused with the complete supported-format list.

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

Decode limits are checked from each format header before decoded-pixel
allocation. The default ceiling is 16384×16384 and 1 GiB of decoded pixels;
hosts can lower each limit. Truncated or malformed data produces a named error
and no partial image.

Large decode control is separate from the retained-pixel limit. Before codec
allocation, CyberTexel computes a conservative working-set bound of two decoded
buffers, exact padded 64×64 tile storage, 256 bytes of metadata per tile and
1 MiB of codec scratch, then refuses work above the caller's ceiling. The
default ceiling is 4 GiB on 64-bit hosts and 2 GiB on 32-bit hosts. Synchronous progress names inspection, codec,
row unpack and completion phases. Cancellation is checked before and after
codec work and on every unpacked row; a host can cancel from the inspected-header
progress callback to stop before the codec allocates. Cancelled work destroys
all staged storage and never publishes a partial decoded image.

JPEG, TGA and BMP decode to their native one-to-four-channel 8-bit layouts.
Flattened PSD composites retain one-to-four 8-bit or 16-bit channels. The
layered path imports 8/16-bit RGB PSD raster layers with raw or PackBits channel
storage as named RGBA images at their signed source origins; composited mode
uses the file's authored flattened appearance rather than reconstructing blend
effects. Unsupported adjustment/empty layers and ZIP-compressed layer channels
are refused explicitly. The baseline TIFF path accepts
little- or big-endian, uncompressed, contiguous, top-left grayscale,
grayscale-alpha, RGB and RGBA strips. It retains unsigned 8/16-bit and float32
samples and rejects unsupported compression, planar layouts, orientations or
photometric interpretations by name. TIFF metadata fields unrelated to pixel
layout are ignored safely.

OpenEXR and Radiance HDR headers are inspected against the same limits before
pixel allocation. They decode to native float32 storage without clamping:
OpenEXR expands named colour channels to RGBA and Radiance HDR retains RGB.
Rec. 709 OpenEXR `chromaticities` attributes and Radiance `PRIMARIES`
declarations resolve to linear Rec. 709. Other primaries and OpenEXR
`colorInteropID` values that this two-space implementation cannot interpret are
reported before the automatic linear rule is applied. An explicit caller
declaration remains authoritative. Multipart OpenEXR resolves metadata per part
and only reports an embedded source for a composite when every part agrees.
Multipart imports place each named part at its data-window origin; composited
mode alpha-overs later file parts over earlier parts across the union of their
data windows. Deep parts are refused because they are not two-dimensional image
layers.

Layered decoding enforces the image-count, aggregate decoded-byte, dimension,
and working-memory ceilings before codec pixel allocation. Its conservative
working bound includes codec planar storage, interleaved staging, padded tiles,
tile metadata, and—in multipart composited mode—the union output. Progress and
cancellation use the same synchronous phases as flat decoding, and no names,
metadata, or pixels are published until the whole request succeeds.

An explicit caller colour-space declaration overrides metadata. Otherwise an
embedded PNG sRGB declaration is used. PNG iCCP payloads, ordered multipart
JPEG APP2 `ICC_PROFILE` chunks, TIFF tag 34675, Photoshop image resource
`0x040f` (for both flattened and individual-layer decoding), and embedded BMP
V5 profiles are parsed with bounded tag-table reads. BMP V4/V5 `LCS_sRGB`
declarations resolve directly to sRGB. D50-adapted Rec. 709
primaries plus either the standard sRGB parametric transfer curve or an identity
curve resolve to sRGB Rec. 709 or linear Rec. 709 respectively. Missing or
duplicate JPEG chunks, duplicate Photoshop resources, linked BMP profiles,
profiles above the 16 MiB interpretation ceiling, unsupported primaries or
transfer curves, malformed tag ranges, unsupported colour models and unknown
types are reported before the automatic semantic rule is applied. TGA has no
ICC container, so it follows an explicit caller declaration or the same
automatic semantic rule.

LodePNG is pinned for memory-based 8/16-bit PNG encoding and decoding. The
pinned stb implementation decodes JPEG, TGA, BMP, flattened PSD and Radiance
HDR and encodes JPEG and TGA. OpenEXR uses pinned TinyEXR and its bundled
miniz implementation. Revisions and licence texts are recorded in the
dependency manifest and third-party notices; baseline TIFF is implemented
locally.

`just fuzz-image-decoders` dispatches a fixed seed for every supported content
signature and then performs 20,000 deterministic, coverage-guided mutations
under libFuzzer, AddressSanitizer and UndefinedBehaviorSanitizer. CI fails on a
crash, sanitizer finding, timeout or memory-limit breach and uploads the
reproducer. The dependency licence gate independently verifies that LodePNG,
stb, TinyEXR and bundled miniz retain pinned revisions and repository-owned
licence texts.

The texture-export path additionally encodes PNG, JPEG, TGA, TIFF and OpenEXR
to caller-owned buffers. Its exact format/depth compatibility table and output
semantics are documented in [Export presets and channel tokens](export-presets.md).
JPEG and TGA use the pinned stb image writer; the baseline TIFF and uncompressed
scanline OpenEXR writers are implemented locally.
