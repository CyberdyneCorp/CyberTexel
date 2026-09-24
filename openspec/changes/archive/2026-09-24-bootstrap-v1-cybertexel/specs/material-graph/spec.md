# material-graph — The Node Document

## Purpose
Model and validate connected material programs before shader emission.

## ADDED Requirements

### Requirement: Graph document
A material SHALL be a directed graph of nodes connected by links, with exactly one output node. The graph SHALL be serializable, clonable and comparable, and SHALL carry no reference to a device, a shading language or a host type.

#### Scenario: Round trip
- **WHEN** a graph is serialized and read back
- **THEN** it SHALL compare equal to the original, including node positions and unconnected socket values

### Requirement: Output node
The output node SHALL expose one input per registered document channel, with defaults taken from its channel descriptor; the built-in preset SHALL expose its nine named channels. An input left unconnected SHALL contribute its stored constant.

#### Scenario: Partially authored material
- **WHEN** only base colour is connected
- **THEN** every other channel SHALL take its stored constant

### Requirement: Node catalogue — input
The catalogue SHALL provide the input nodes Constant Value, Constant Colour, Texture Coordinate, UV Set, Geometry, Object Info, Mesh Map, Layer Reference, Anchor Point and Picker.

#### Scenario: Reading a mesh map
- **WHEN** a Mesh Map node names ambient occlusion
- **THEN** it SHALL output the bound AO map, and SHALL report an error if no AO map is bound

### Requirement: Node catalogue — texture
The catalogue SHALL provide Image, Noise, Voronoi, Gradient, Checker, Brick, Wave, Magic, Gabor and Tile Sheet.

#### Scenario: Noise determinism
- **WHEN** a noise node is evaluated twice with the same seed and coordinates
- **THEN** it SHALL produce identical values on every executor within tolerance

### Requirement: Node catalogue — colour and filter
The catalogue SHALL provide Mix, Blend (the full blend mode set), Levels, Curves, Colour Ramp, Hue/Saturation/Value, Brightness/Contrast, Gamma, Invert, Quantize, Replace Colour, Colour Mask, Separate Colour, Combine Colour, Blur, Sharpen, Warp and Grayscale Conversion.

#### Scenario: Blend parity with the layer stack
- **WHEN** the Blend node uses a named mode
- **THEN** it SHALL implement the same formula the layer stack uses for that mode

### Requirement: Node catalogue — vector and math
The catalogue SHALL provide Math, Vector Math, Map Range, Clamp, Mapping, Normal Map, Mix Normal Map, Bump, Separate XYZ, Combine XYZ, Vector Rotate, Vector Transform, Float Curve and Vector Curves. Math and Vector Math operations SHALL be enumerated in the reference documentation with their formulas.

#### Scenario: Normal map blending modes
- **WHEN** Mix Normal Map is used
- **THEN** partial-derivative, whiteout and reoriented modes SHALL be available and documented

### Requirement: Node groups
Nodes SHALL be groupable into a reusable subgraph with declared input and output sockets. A group SHALL be instantiable many times, and changing a group's socket list SHALL propagate to every instance, preserving stored values whose socket name and type are unchanged.

#### Scenario: Adding a group socket
- **WHEN** a socket is added to a group used by three materials
- **THEN** all three instances SHALL gain it and their existing socket values SHALL be preserved

### Requirement: Group recursion is refused
Placing a group instance whose subgraph transitively contains the group currently being edited SHALL be refused with a diagnostic naming the cycle, before the instance is created.

#### Scenario: Self-reference
- **WHEN** a group is placed inside itself
- **THEN** the placement SHALL be refused and the graph SHALL be unchanged

### Requirement: Socket types and coercion
Socket types SHALL be scalar, vector, colour, string, image and boolean. Any output SHALL be connectable to any input of a coercible type; coercion SHALL be applied at emission, not at edit time. Scalar to vector SHALL broadcast; vector to scalar SHALL reduce by the documented luminance weights; colour to vector SHALL pass the RGB components. A connection between non-coercible types SHALL be refused.

#### Scenario: Colour into roughness
- **WHEN** a colour output is connected to the scalar roughness input
- **THEN** the connection SHALL be accepted and the emitted code SHALL reduce it by the documented luminance weights

#### Scenario: Image into scalar is refused
- **WHEN** an image-typed output is connected to a scalar input
- **THEN** the connection SHALL be refused with a diagnostic naming both types

### Requirement: One link per input
An input socket SHALL hold at most one link. Connecting to an occupied input SHALL replace the existing link and report the replacement.

#### Scenario: Replacing a connection
- **WHEN** a second link is made to an occupied input
- **THEN** the previous link SHALL be removed and the replacement SHALL be reported

### Requirement: Cycle detection is explicit
The graph SHALL detect cycles at edit time and refuse the link that would create one, naming the path. Cycle handling SHALL NOT be left to the emission stage.

#### Scenario: Creating a cycle
- **WHEN** a link would close a cycle
- **THEN** it SHALL be refused with the cycle path in the diagnostic, and the graph SHALL be unchanged

### Requirement: Validation before emission
A graph SHALL be validatable independently of emission, reporting unconnected required inputs, missing mesh maps, missing image resources and unreachable nodes.

#### Scenario: Validating a downloaded material
- **WHEN** a material referencing an unavailable image is validated
- **THEN** validation SHALL report the missing resource by name and SHALL NOT attempt emission

### Requirement: Node presets and material library
A graph SHALL be saveable as a named material preset with a thumbnail, and a set of presets SHALL be loadable as a library. Preset identity SHALL be stable across machines.

#### Scenario: Reusing a material
- **WHEN** a saved material is applied on another machine with the same library
- **THEN** it SHALL resolve to the same graph

### Requirement: Extensibility for host node types
A host SHALL be able to register a node type by supplying its versioned socket declaration, an emission callback and a CPU evaluation callback or portable representation supported by both executors, and such a node SHALL participate in validation, serialization and grouping like a built-in node.

#### Scenario: Host-provided node
- **WHEN** a host registers a node type and uses it in a graph
- **THEN** the graph SHALL serialize, validate and emit with that node present

#### Scenario: Unknown node on load
- **WHEN** a graph referencing an unregistered node type is loaded
- **THEN** the node SHALL be preserved as opaque, the graph SHALL be marked non-emittable, and the missing type SHALL be named

### Requirement: Custom nodes satisfy the reference contract
Node registration SHALL reject an emission-only node without a CPU implementation. The registration SHALL declare determinism, input resource dependencies and supported targets; recoverable replay SHALL require deterministic evaluation and pinned inputs. Custom nodes SHALL be exercised by parity fixtures like built-in nodes.

#### Scenario: GPU-only callback
- **WHEN** a host registers a node with emission but no CPU evaluation implementation
- **THEN** registration SHALL fail naming the missing implementation before any graph can use the node
