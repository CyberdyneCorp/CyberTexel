# Smart-material scenario coverage

`just test-smart-material-scenarios` runs every CTest carrying the
`smart-materials-scenario` label. The `smart-material-scenario-matrix` test
compares this table with the OpenSpec capability, so a scenario cannot be added,
removed, or renamed without updating executable evidence.

| OpenSpec scenario | Executable evidence | Covered behavior |
| --- | --- | --- |
| Applying to a new model | `texture-document`, `mesh-map-generators` | One definition instantiates as independent derived fragments on texture sets, while generator evaluation consumes the destination mesh-map values rather than cached preset pixels |
| Mixed content | `smart-material-serialization`, `texture-document` | Derived entries retain definitions, model-specific entries retain exact pixels, and application reports both kinds and stored bytes |
| One slider drives several layers | `smart-material-serialization`, `texture-document` | One typed ranged update changes every bound graph input/property atomically in presets and applied instances |
| Reusing a mask | `smart-mask-instances`, `texture-document` | Separate applications deep-copy mask graphs and parameter state and attach to an eligible layer or group |
| Driving a mask from a paint layer | `smart-material-serialization` | Marked layer/mask outputs bind concrete image inputs and a changed anchor produces its dependent evaluation plan |
| Reference below the anchor is refused | `smart-material-serialization` | A downward reference is refused atomically with the ordering rule named |
| Mutual reference | `smart-material-serialization` | A cycle-closing reference is refused atomically with the complete path |
| Bounded update | `smart-material-serialization` | A change in one of twenty layers returns only its three transitive consumers in dependency order |
| Shelf moved between machines | `smart-material-resources` | Relative identities resolve through ordered search roots after the shelf directory moves |
| Resource genuinely missing | `smart-material-resources` | The missing identifier is reported per input and retained without neutral substitution |
| Sharing a material | `smart-material-resources` | A self-contained package embeds all dependencies and imports with an empty search path |
| Listing available presets | `preset-shelf-library` | All seven kinds enumerate stable identity, kind, display name, sorted tags, version, and embedded thumbnail |
| Forward compatibility | `smart-material-serialization`, `preset-shelf-library` | Future smart-material and all seven shelf-kind versions are named and refused before destination replacement |
| Undoing an application | `texture-document` | One undo removes all twelve entries of a mixed-content material; mask undo leaves its target intact |
| Editing an applied material | `texture-document` | Transactional entry and parameter edits remain permitted while stable identity and preset origin survive |
