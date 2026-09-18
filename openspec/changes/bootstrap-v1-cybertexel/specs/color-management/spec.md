# color-management — Colour In, Through And Out

## ADDED Requirements

### Requirement: Declared working space
The system SHALL define a single working colour space for colour-valued channels and SHALL state it. Every colour entering the system SHALL be converted into it and every colour leaving SHALL be converted out of it.

#### Scenario: Working space is stated
- **WHEN** the working space is queried
- **THEN** it SHALL be reported by name rather than left implicit

### Requirement: Per-channel colour semantics
Each document channel SHALL declare whether it is colour-valued or data-valued. Colour-valued channels SHALL be colour-managed; data-valued channels — roughness, metallic, height, occlusion, normal, opacity, subsurface — SHALL be treated as linear data and SHALL NOT receive a transfer function.

#### Scenario: Roughness is not gamma-corrected
- **WHEN** a roughness value of 0.5 is authored and exported
- **THEN** the exported value SHALL be 0.5 in the output encoding, with no transfer function applied

### Requirement: Input colour space declaration
Every imported image SHALL carry a declared colour space, selectable per image, with an automatic mode that infers sRGB for colour-valued use and linear for data-valued use. An image whose colour space cannot be inferred SHALL be reported rather than guessed silently.

#### Scenario: Importing a photograph
- **WHEN** a photograph is imported for base colour in automatic mode
- **THEN** it SHALL be interpreted as sRGB and converted to the working space

#### Scenario: Importing a roughness map
- **WHEN** a grayscale map is imported for roughness in automatic mode
- **THEN** it SHALL be interpreted as linear data

### Requirement: Supported transfer functions and primaries
The system SHALL support at minimum the sRGB transfer function, a pure linear transfer function, and Rec. 709 primaries, and SHALL state which additional spaces it supports.

#### Scenario: Space is refused rather than approximated
- **WHEN** an unsupported colour space is declared
- **THEN** the import SHALL be refused naming the space, rather than being approximated

### Requirement: Colour lookup tables
The system SHALL support loading a 3D lookup table in the `.cube` format for display transforms, SHALL apply it only to preview output, and SHALL NOT apply it to exported textures.

#### Scenario: Grading affects preview only
- **WHEN** a LUT is loaded and textures are exported
- **THEN** the exported textures SHALL be unaffected by the LUT

### Requirement: Bit depth policy
The system SHALL state, per channel, the minimum bit depth required to avoid visible artefacts, and SHALL warn when a channel's selected storage bit depth falls below it for an enabled channel. Normal and height SHALL require at least 16 bits.

#### Scenario: 8-bit normal map warning
- **WHEN** a texture set with an enabled normal channel is set to 8 bits per channel
- **THEN** the system SHALL warn naming the channel and the recommended depth, and SHALL still permit the setting

### Requirement: Precision promotion for derived operations
Operations whose intermediate precision exceeds the document's bit depth — normal blending, height accumulation and derivative computation — SHALL be performed at a higher working precision and stored back at the document depth.

#### Scenario: Height accumulation at 8 bits
- **WHEN** many height contributions accumulate on an 8-bit texture set
- **THEN** accumulation SHALL occur at higher precision before being quantized once

### Requirement: Dithering on quantization
Quantization to 8 bits SHALL apply an ordered dither by default, disableable by the host, and the dither pattern SHALL be deterministic.

#### Scenario: Gradient banding
- **WHEN** a smooth gradient is quantized to 8 bits with dithering enabled
- **THEN** banding SHALL be reduced and the result SHALL be reproducible

### Requirement: Colour values are addressable in the working space
A colour supplied through any binding SHALL be accompanied by its space, and the system SHALL NOT assume the caller's convention.

#### Scenario: A colour from a host picker
- **WHEN** a host supplies an sRGB colour without declaring the space
- **THEN** the call SHALL be refused, or the documented default SHALL be applied and reported, rather than being assumed silently

### Requirement: Round-trip fidelity
An image imported and exported without editing, in the same space and bit depth, SHALL round-trip within a stated tolerance.

#### Scenario: Import then export
- **WHEN** a 16-bit linear image is imported and exported unchanged at 16-bit linear
- **THEN** the result SHALL match the input within the stated tolerance

### Requirement: Colour management is testable without a display
Every colour transform SHALL be exercisable headlessly against known reference values.

#### Scenario: Transform test
- **WHEN** the colour transform suite runs in CI with no display
- **THEN** each transform SHALL be verified against its reference values
