# Editable authoring scenario coverage

`just test-editable-authoring-scenarios` runs every CTest carrying the
`editable-authoring-scenario` label. This checked matrix maps every OpenSpec
editable-authoring scenario to native and strict-C evidence.

| OpenSpec scenario | CTest | Covered behavior |
|---|---|---|
| Brush asset changes after painting | `operation-record`, `c-abi-operation-record` | Canonical records own pinned resource bytes and content identities, survive a project-container round trip, and do not consult mutable external assets |
| Snapshot-dependent clone | `operation-record`, `c-abi-operation-record` | Clone, blur and smear replay is refused without a pinned source snapshot while checkpoint-only recovery remains explicit |
| Higher-resolution stroke | `resolution-change`, `c-abi-resolution-change` | Resolution-independent records replay at the target size while mixed checkpoint segments require an explicit resampling policy |
| Editing saved text | `editable-authoring`, `c-abi-editable-authoring` | Editable text, decals and surface paths retain source identities and parameters through save/reopen, editing, undo and redo |
| Ambiguous thin surfaces | `mesh-reprojection` | Multiple source candidates are reported and commit is refused until the host supplies an ambiguity policy |
| Cancelled reprojection | `mesh-reprojection` | Cancellation and budget failure leave the old mesh binding, pixels and editable attachments unchanged |
