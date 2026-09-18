# image-io — Pixels In And Out

## ADDED Requirements

### Requirement: Decoded formats
The system SHALL decode PNG, JPEG, TGA, BMP, TIFF, OpenEXR, Radiance HDR and PSD. A format the build does not support SHALL be refused by name rather than attempted.

#### Scenario: Importing a brush alpha
- **WHEN** a grayscale PNG is imported as a brush alpha
- **THEN** it SHALL decode to a single-channel buffer at its native bit depth

#### Scenario: Unsupported format
- **WHEN** a file whose format this build cannot decode is imported
- **THEN** the import SHALL fail naming the detected format and the supported set

### Requirement: Format detection is by content
Format SHALL be detected from the file's content, not from its extension. A file whose content and extension disagree SHALL decode by content and report the disagreement.

#### Scenario: Mislabelled file
- **WHEN** a PNG named with a `.jpg` extension is imported
- **THEN** it SHALL decode as PNG and the mismatch SHALL be reported

### Requirement: Bit depth and channel preservation
Decoding SHALL preserve the source bit depth up to 32 bits per channel and SHALL NOT silently reduce it. A source with fewer channels than the destination requires SHALL be expanded by a documented rule rather than by guesswork.

#### Scenario: 16-bit source
- **WHEN** a 16-bit PNG is imported
- **THEN** it SHALL be held at 16 bits per channel, not reduced to 8

#### Scenario: Grayscale into a colour slot
- **WHEN** a single-channel image is used where three are required
- **THEN** it SHALL be replicated across the three channels and the expansion SHALL be documented

### Requirement: Colour space on read
Every decoded image SHALL carry a colour space, taken from an embedded profile where the format carries one, from an explicit caller declaration, or from the automatic rule in `color-management`. An embedded profile the system cannot interpret SHALL be reported rather than ignored.

#### Scenario: Embedded profile
- **WHEN** an image carrying an embedded colour profile is imported
- **THEN** that profile SHALL be used, and a caller declaration SHALL override it only when explicitly requested

#### Scenario: Uninterpretable profile
- **WHEN** an embedded profile cannot be interpreted
- **THEN** it SHALL be reported and the automatic rule SHALL apply

### Requirement: High dynamic range
OpenEXR and Radiance HDR SHALL decode to floating-point buffers without clamping, and their values SHALL survive into the working space unclamped where the destination channel is floating point.

#### Scenario: HDR environment image
- **WHEN** a Radiance HDR file with values above 1.0 is imported
- **THEN** those values SHALL be preserved rather than clamped

### Requirement: Layered sources
A PSD or a multi-part OpenEXR SHALL be importable either as its composited result or as its individual layers or parts, selectable by the caller, with each layer named from the source.

#### Scenario: Importing a layered PSD
- **WHEN** a PSD is imported in per-layer mode
- **THEN** each layer SHALL become a separately addressable image named from the source layer

#### Scenario: Composited import
- **WHEN** the same PSD is imported in composited mode
- **THEN** one image matching the file's flattened appearance SHALL be produced

### Requirement: Encoded formats
The system SHALL encode PNG, JPEG, TGA, TIFF and OpenEXR, with per-format options for compression and quality, and SHALL refuse a bit depth a format cannot carry, naming both.

#### Scenario: Quality option
- **WHEN** JPEG output is requested with a quality value
- **THEN** it SHALL be honoured and recorded in the export report

### Requirement: Decoding from memory
Decoding SHALL accept a byte buffer as well as a path, so that a host that has already read or received an image need not write it to disk.

#### Scenario: Image from a network host
- **WHEN** a host supplies encoded bytes
- **THEN** they SHALL decode without touching the filesystem

### Requirement: Every image source is untrusted
The decoders SHALL treat every input as hostile. Declared dimensions SHALL be validated against the available data before allocation, allocations SHALL be bounded by a configurable ceiling, and a malformed or inconsistent file SHALL be refused by name.

#### Scenario: Declared dimensions are implausible
- **WHEN** an image header declares dimensions whose product exceeds the configured ceiling
- **THEN** the decode SHALL be refused before allocation, naming the declared size and the ceiling

#### Scenario: Truncated file
- **WHEN** a file ends mid-scanline
- **THEN** the decode SHALL fail by name, and SHALL NOT return a partially filled buffer as if it were complete

### Requirement: Decoder fuzzing gate
Every decoder SHALL be fuzzed in CI. No input SHALL produce a crash, an out-of-bounds access, an uninitialized read or an unbounded allocation.

#### Scenario: Fuzzing run
- **WHEN** the decoder fuzzing job runs
- **THEN** every decoder SHALL be exercised and any finding SHALL fail the build

### Requirement: Decoding is cancellable and bounded
Decoding a large image SHALL be cancellable and SHALL report progress, and its peak working memory SHALL be bounded by the configured ceiling.

#### Scenario: Cancelling a large decode
- **WHEN** a decode of a very large EXR is cancelled
- **THEN** it SHALL stop promptly and release its working memory

### Requirement: Resampling is documented
Where an imported image is resampled to fit a destination, the filter SHALL be selectable from a documented set, with a stated default, and the choice SHALL be recorded.

#### Scenario: Downscaling on import
- **WHEN** a 8192 image is imported into a 2048 slot
- **THEN** it SHALL be resampled with the documented default filter and the filter SHALL be reported

### Requirement: Round-trip fidelity
An image decoded and re-encoded in the same format, bit depth and colour space without editing SHALL round-trip within the tolerance stated in `color-management`, and losslessly for lossless formats.

#### Scenario: Lossless round trip
- **WHEN** a 16-bit PNG is decoded and re-encoded unchanged
- **THEN** the result SHALL be bit-identical

### Requirement: Third-party decoders are audited
Any vendored decoder SHALL appear in the attribution file with its licence text and pinned revision, and SHALL be covered by the dependency audit and the fuzzing gate.

#### Scenario: Audit covers decoders
- **WHEN** the licence audit runs
- **THEN** every vendored image decoder SHALL appear with its own licence text and revision
