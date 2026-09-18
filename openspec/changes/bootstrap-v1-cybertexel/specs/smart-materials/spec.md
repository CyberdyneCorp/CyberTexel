# smart-materials — Reusable Material And Mask Presets

## ADDED Requirements

### Requirement: A smart material is a parameterised stack
A smart material SHALL be a serialized fragment of the layer stack — layers, groups, masks, filters, generators and their graphs — together with a set of exposed parameters. Applying it to a texture set SHALL instantiate that fragment.

#### Scenario: Applying to a new model
- **WHEN** a smart material is applied to a texture set on a different mesh with its mesh maps bound
- **THEN** its generators SHALL re-derive from that mesh's maps and the result SHALL adapt to the new geometry

### Requirement: Smart materials carry no baked pixels for derived content
A smart material SHALL store generator and graph definitions rather than their rasterized output, so that the result is re-derived per model. Hand-painted content inside a smart material SHALL be stored as pixels and SHALL be marked as model-specific.

#### Scenario: Mixed content
- **WHEN** a smart material containing both a generator mask and a hand-painted mask is applied to another model
- **THEN** the generator mask SHALL re-derive and the hand-painted mask SHALL be applied as stored, with its model-specific nature reported

### Requirement: Exposed parameters
A smart material SHALL expose named parameters with types, defaults, ranges and display groupings, and changing a parameter SHALL update every layer bound to it.

#### Scenario: One slider drives several layers
- **WHEN** an exposed "wear amount" parameter is changed
- **THEN** every generator and graph value bound to it SHALL update together

### Requirement: Smart masks
A smart mask SHALL be a reusable mask definition — generators, filters and a graph — applicable to any layer or group, exposing parameters in the same way as a smart material.

#### Scenario: Reusing a mask
- **WHEN** a smart mask is applied to two different layers
- **THEN** each SHALL receive an independent instance with its own parameter values

### Requirement: Anchor points
A layer or mask SHALL be markable as an anchor point, and another layer's graph SHALL be able to read that anchor's composited output as an input. An anchor SHALL be referable only from layers above it in the stack.

#### Scenario: Driving a mask from a paint layer
- **WHEN** a paint layer is marked as an anchor and a layer above reads it
- **THEN** painting on the anchor SHALL update the dependent layer

#### Scenario: Reference below the anchor is refused
- **WHEN** a layer below the anchor attempts to read it
- **THEN** the reference SHALL be refused with a diagnostic naming the ordering rule

### Requirement: Anchor cycles are refused
A reference graph among anchors SHALL be acyclic. A reference that would close a cycle SHALL be refused, naming the path.

#### Scenario: Mutual reference
- **WHEN** two layers attempt to read each other as anchors
- **THEN** the second reference SHALL be refused with the cycle path named

### Requirement: Evaluation order
The system SHALL evaluate anchors in dependency order and SHALL re-evaluate only the layers whose inputs changed.

#### Scenario: Bounded update
- **WHEN** an anchor changes and three of twenty layers depend on it
- **THEN** only those three SHALL be re-evaluated

### Requirement: Resource resolution is portable
A smart material SHALL reference images, fonts and other resources by a stable identifier resolved through a search path, not by absolute file path. A missing resource SHALL be reported by identifier.

#### Scenario: Shelf moved between machines
- **WHEN** a smart material is applied on a machine whose shelf lies at a different path
- **THEN** its resources SHALL resolve through the search path

#### Scenario: Resource genuinely missing
- **WHEN** a referenced resource cannot be resolved anywhere on the search path
- **THEN** it SHALL be reported by identifier and the material SHALL be applied with that input marked missing rather than substituted

### Requirement: Self-contained packaging
A smart material SHALL be exportable in a self-contained form that embeds its resources, and importing one SHALL not require the exporter's shelf.

#### Scenario: Sharing a material
- **WHEN** a self-contained smart material is imported on a machine with an empty shelf
- **THEN** it SHALL apply with every resource present

### Requirement: Shelf and library
The system SHALL support a library of presets — materials, smart materials, smart masks, brushes, stroke presets, generators and export presets — organised into named shelves with metadata and thumbnails, resolvable by identifier.

#### Scenario: Listing available presets
- **WHEN** a host enumerates the shelf
- **THEN** each preset SHALL report its identifier, kind, display name, tags and thumbnail

### Requirement: Versioned presets
Every preset SHALL carry a schema version. An older preset SHALL load with unknown fields taking documented defaults; a newer one SHALL be refused by name rather than partially applied.

#### Scenario: Forward compatibility
- **WHEN** a preset declaring a newer schema version is loaded
- **THEN** it SHALL be refused naming the version, and nothing SHALL be partially applied

### Requirement: Applying is one undo step
Applying a smart material or smart mask SHALL be a single undo step regardless of how many entries it instantiates.

#### Scenario: Undoing an application
- **WHEN** a smart material instantiating twelve layers is applied and undone
- **THEN** one undo SHALL remove all twelve

### Requirement: Instantiation is inspectable
After application the instantiated entries SHALL be ordinary document entries, editable individually, and SHALL record which preset and version they came from.

#### Scenario: Editing an applied material
- **WHEN** a layer from an applied smart material is edited
- **THEN** the edit SHALL be permitted and the entry SHALL still report its origin preset
