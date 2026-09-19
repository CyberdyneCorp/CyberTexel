# Material graph scenario coverage

`just test-material-graph-scenarios` runs every CTest carrying the
`material-graph-scenario` label. The suite maps each OpenSpec scenario to
headless executable coverage.

| OpenSpec scenario | CTest | Covered behavior |
|---|---|---|
| Round trip | `material-graph-document` | Complete canonical graph serialization, including positions and stored values |
| Partially authored material | `material-graph-output` | Nine channel-derived inputs retain descriptor defaults when unconnected |
| Reading a mesh map | `material-graph-validation` | Ambient-occlusion availability and named missing-map diagnostic |
| Noise determinism | `material-graph-portable-nodes` | Equal seed, coordinates and parameters produce identical normalized values; changes affect output |
| Blend parity with the layer stack | `material-graph-portable-nodes` | Every Blend mode routes through the one public reference formula surface reserved for node and stack use |
| Normal map blending modes | `material-graph-catalogue` | Partial-derivative, whiteout and reoriented choices match the documented formulas |
| Adding a group socket | `material-graph-groups` | Three material instances gain the socket while compatible values survive |
| Self-reference | `material-graph-groups` | Placement is refused transactionally with a typed recursion path |
| Colour into roughness | `material-graph-socket-links`, `graph-emission` | Edit accepts the coercion and emitted WGSL contains Rec. 709 reduction |
| Image into scalar is refused | `material-graph-socket-links` | Both incompatible types are reported before mutation |
| Replacing a connection | `material-graph-socket-links` | One-link-per-input replacement returns the displaced link |
| Creating a cycle | `material-graph-cycle-detection` | Closing path is named and the graph remains unchanged |
| Validating a downloaded material | `material-graph-validation` | Missing image is named without invoking emission |
| Reusing a material | `material-graph-library` | Canonical transferred library resolves stable identity to the same independent graph |
| Host-provided node | `material-graph-host-nodes`, `graph-emission` | Registered type serializes, validates, groups and emits |
| Unknown node on load | `material-graph-host-nodes` | Opaque content round-trips while emission is refused by type and version |
| GPU-only callback | `material-graph-host-nodes` | Registration fails before use when CPU semantics are absent |

The portable Noise fixture becomes the executor-parity input when CPU and GPU
execution arrive in task 7.5. The Blend fixture establishes one formula surface;
task 3.5 wires the layer-stack data model to it and adds per-mode image fixtures.
Those later integrations extend the same labeled contract rather than replacing
this suite.
