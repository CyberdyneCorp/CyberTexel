# texture-export — Getting Texture Sets Out

## ADDED Requirements

### Requirement: Export presets are data
An export preset SHALL be a document listing output textures, each naming a filename suffix, four channel tokens for its RGBA slots, a colour space and a bit depth. Adding support for a target engine's convention SHALL require no engine code.

#### Scenario: A new engine convention
- **WHEN** a preset for an unsupported target engine is authored
- **THEN** it SHALL be usable without modifying the library

### Requirement: Channel token vocabulary
The system SHALL accept the channel tokens: the three base colour components; opacity; roughness; smoothness; metallic; the three normal components and a DirectX-convention green; height; occlusion; emission; subsurface; the three diffuse and three specular components of the specular-glossiness split; the constants `0.0` and `1.0`; and a named mesh map. Every token SHALL be documented with its derivation.

#### Scenario: Packed ORM
- **WHEN** a preset slot lists occlusion, roughness and metallic in RGB
- **THEN** a single packed texture SHALL be written with those channels in those slots

### Requirement: Derived tokens
Smoothness SHALL be one minus roughness. The DirectX normal green SHALL be one minus the normal green. The diffuse components SHALL be the base colour scaled by one minus metallic, and the specular components SHALL be a dielectric constant scaled by one minus metallic plus the base colour scaled by metallic, with the constant stated.

#### Scenario: Specular-glossiness export
- **WHEN** a specular workflow preset is exported
- **THEN** diffuse and specular SHALL be derived by the stated formulas rather than requiring the artist to author them

### Requirement: Built-in presets
The system SHALL ship presets covering, at minimum: individual PBR maps; a packed occlusion-roughness-metallic convention; a packed metallic-occlusion-smoothness convention; a packed metallic-emission-roughness convention; base colour only; and a specular-glossiness workflow.

#### Scenario: Default export
- **WHEN** no preset is chosen
- **THEN** the individual PBR map preset SHALL be used and SHALL be named in the report

### Requirement: Output formats and bit depths
The system SHALL write PNG, JPEG, TGA, TIFF and OpenEXR. Presets SHALL be able to request 8, 16 or 32 bits per channel, and the system SHALL refuse a format and bit-depth combination the format cannot carry, naming both.

#### Scenario: 32-bit request to PNG
- **WHEN** a preset requests 32 bits per channel in PNG
- **THEN** the export SHALL be refused naming the format and the depth, and no file SHALL be written

### Requirement: Export scopes
Export SHALL support the scopes: all texture sets, selected texture sets, per UDIM tile, and per atlas. Scopes SHALL be composable with a selection of layers.

#### Scenario: Per-tile export
- **WHEN** per-UDIM-tile scope is used
- **THEN** one texture set SHALL be written per occupied tile with the tile number in the filename

### Requirement: Layer scope
Export SHALL be able to flatten all visible layers, only selected layers, or each layer separately, and a group selected for export SHALL include its children.

#### Scenario: Exporting one group
- **WHEN** a group is selected and group scope is used
- **THEN** the flattened result of that group's children SHALL be exported

### Requirement: Filename pattern
Output paths SHALL be generated from a documented pattern with named tokens for the project name, texture set name, UDIM tile, preset texture suffix, resolution and bit depth. The pattern SHALL be overridable.

#### Scenario: Predictable names
- **WHEN** a document with two texture sets is exported with the default pattern
- **THEN** the filenames SHALL differ by the texture set token and SHALL be derivable from the pattern without inspecting the output

#### Scenario: Pattern would collide
- **WHEN** a pattern would produce the same path for two outputs
- **THEN** the export SHALL be refused naming the collision, before any file is written

### Requirement: Export resolution
Export resolution SHALL be selectable independently of the texture set's working resolution, and the resampling filter SHALL be documented.

#### Scenario: Downscaling on export
- **WHEN** a 4096 texture set is exported at 1024
- **THEN** 1024 textures SHALL be written using the documented filter

### Requirement: Padding
Export SHALL apply configurable UV border padding, defaulting to a documented value, using the same extrapolating dilation the paint engine uses. A padding of zero SHALL disable it.

#### Scenario: Padding prevents seams
- **WHEN** padding is applied at export
- **THEN** texels beyond island borders SHALL carry extrapolated colour

### Requirement: Dry run
Export SHALL support a dry run that reports every file it would write, with its dimensions, format and estimated size, without writing anything.

#### Scenario: Checking before writing
- **WHEN** a dry run is requested
- **THEN** the full output manifest SHALL be reported and no file SHALL be created

### Requirement: Machine-readable report
Export SHALL emit a machine-readable report naming every file written, its texture set, preset entry, resolution, format, colour space and bit depth.

#### Scenario: Pipeline consumption
- **WHEN** an automated pipeline exports a document
- **THEN** it SHALL be able to locate every output from the report without scanning the directory

### Requirement: Export is cancellable and reports progress
Export SHALL report progress per output and SHALL be cancellable. A cancelled export SHALL leave no partially written file.

#### Scenario: Cancelled export
- **WHEN** an export of twenty textures is cancelled after eight
- **THEN** no partial file SHALL remain on disk

### Requirement: In-memory export
Export SHALL be able to deliver results as buffers to the caller instead of writing files.

#### Scenario: Host wants pixels
- **WHEN** a host requests in-memory export
- **THEN** it SHALL receive the pixel buffers with their declared formats and no file SHALL be written
