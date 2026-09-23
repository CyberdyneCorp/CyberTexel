# Image-I/O scenario coverage

`just test-image-io-scenarios` runs the device-free CTests carrying the
`image-io-scenario` label. The matrix test compares this document with the
OpenSpec capability. Decoder fuzzing remains its sanitizer-backed CI recipe and
the matrix verifies that the recipe is present in the workflow.

| OpenSpec scenario | Executable evidence | Covered behavior |
| --- | --- | --- |
| Importing a brush alpha | `png-io`, `c-abi-image-io` | A grayscale PNG remains one channel at its native unsigned-normalized depth. |
| Unsupported format | `png-io`, `c-abi-image-io` | Detected unsupported content is refused with the supported format set. |
| Mislabelled file | `png-io`, `c-abi-image-io` | PNG bytes with a JPEG name decode as PNG and report the extension mismatch. |
| 16-bit source | `png-io`, `c-abi-image-io` | Native 16-bit samples survive decode and caller-buffer transfer without reduction. |
| Grayscale into a colour slot | `tiled-image`, `c-abi-image-io` | The documented expansion replicates grayscale into RGB and adds opaque alpha only when requested. |
| Embedded profile | `png-io` | Embedded PNG sRGB metadata and bounded RGB ICC profiles for sRGB/linear Rec. 709 are interpreted; an explicit caller declaration overrides either. |
| Uninterpretable profile | `png-io` | A valid PNG carrying an unsupported ICC profile reports it before applying the automatic semantic rule. |
| HDR environment image | `png-io` | Radiance HDR and OpenEXR decode into float storage while retaining values above one. |
| Importing a layered PSD | `layered-image-io` | Named PSD layers are returned independently in file order. |
| Composited import | `layered-image-io` | The same layered PSD and multipart EXR produce their flattened appearance in composite mode. |
| Quality option | `texture-export-execution` | JPEG quality reaches the encoder and the machine-readable export report. |
| Image from a network host | `png-io`, `c-abi-image-io` | Every flat decoder consumes caller-owned bytes and requires no filesystem path. |
| Declared dimensions are implausible | `png-io`, `c-abi-image-io` | Header dimensions and decoded byte counts are rejected against caller ceilings before allocation. |
| Truncated file | `png-io`, `c-abi-image-io` | Mid-stream truncation is named and never returns partial pixels. |
| Fuzzing run | `just fuzz-image-decoders` | CI runs the deterministic 20,000-input ASan/UBSan campaign over every decoder signature. |
| Cancelling a large decode | `png-io`, `c-abi-image-io` | Cancellation during inspected large input returns the cancellation outcome and publishes no pixels. |
| Downscaling on import | `tiled-image`, `c-abi-image-io` | Nearest and pixel-centred bilinear filters are selectable, bounded and recorded, with bilinear as default. |
| Lossless round trip | `png-io`, `c-abi-image-io` | A 16-bit PNG decodes and re-encodes with bit-identical sample values. |
| Audit covers decoders | `shader-emission-licence-audit` | The dependency audit requires pinned revisions and repository-owned licence texts for every image decoder. |
