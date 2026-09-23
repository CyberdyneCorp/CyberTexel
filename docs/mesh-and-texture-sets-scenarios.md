# Mesh and texture-set scenario coverage

`just test-mesh-and-texture-set-scenarios` runs the CTests carrying the
`mesh-and-texture-sets-scenario` label. The scenario-matrix test compares this
document with the OpenSpec capability, so a new or renamed scenario must name
registered, labeled executable evidence.

| OpenSpec scenario | Executable evidence | Covered behavior |
| --- | --- | --- |
| Geometry is never written | `mesh-ingest`, `c-abi-mesh-ingest` | In-memory vertex, index and attribute buffers are unchanged after ingest and inspection through both native and C boundaries. |
| Host supplies buffers | `mesh-ingest`, `c-abi-mesh-ingest` | Complete descriptors are accepted from caller-owned memory without a filesystem path or parser. |
| Second UV set for a lightmap channel | `mesh-ingest`, `c-abi-mesh-ingest` | Named UV sets and the selected default survive ingest and are addressable independently. |
| Partition is total | `mesh-ingest` | Material, submesh and explicit partitions cover every face exactly once; gaps and duplicates are refused. |
| Mixed resolutions | `texture-document` | Stable texture sets in one document retain independent 4096 and 1024 resolutions and channel storage. |
| Resolution change preserves content | `resolution-change`, `c-abi-resolution-change` | Replay and resampling policies are explicit, budgeted and atomic, with undo and redo restoring dimensions and content. |
| Painting across a tile border | `texture-set-udim`, `c-abi-texture-set-udim` | One border-crossing write allocates and updates both addressed UDIM tiles. |
| Unused tiles cost nothing | `texture-set-udim`, `c-abi-texture-set-udim` | Logical occupancy is retained while pixel storage exists only for populated tiles. |
| Two props share an atlas | `texture-set-atlas`, `c-abi-texture-set-atlas` | Two stable texture sets plan one region-aware atlas output. |
| Warning on import | `mesh-uv-diagnostics`, `c-abi-mesh-uv-diagnostics` | Positive-area overlaps are reported by affected face index without rejecting the mesh. |
| Faces outside the UV square | `mesh-uv-diagnostics`, `c-abi-mesh-uv-diagnostics` | Non-UDIM coverage reports uncovered area and exact outside-face indices. |
| Topology changed, UVs preserved | `mesh-replacement-policy` | Stable partition and UV identity retain existing channel pixels across reordered topology. |
| UVs changed | `mesh-replacement-policy`, `mesh-reprojection` | Changed layouts are reported before mutation and require keep, clear or reprojection policy. |
| Reordered submeshes | `mesh-replacement-policy` | Partition-derived identifiers match texture sets independently of mesh and face order. |
| Derived state is invalidated once | `picking-spatial-index`, `paint-surface-cache`, `mesh-maps` | Picking, paint surface maps and bound mesh maps all observe the single published mesh revision. |
| Oversized mesh | `mesh-ingest`, `c-abi-mesh-ingest` | Vertex and triangle limits are checked before array access and diagnostics name supplied and maximum counts. |
