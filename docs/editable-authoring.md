# Editable authoring

`ctex::doc::EditableAuthoringStore` retains decals, text and surface paths as
source entries instead of flattening them at gesture commit. Each entry owns a
stable identity, revision, placement frame, material identity and parameters,
and the exact channel tiles affected by its current evaluation. Text also keeps
its UTF-8 source and supplied-font content identity. Surface paths keep the mesh
revision plus position, normal, triangle, barycentric attachment and width for
every control point.

Adding, editing or removing an entry creates exactly one store undo step.
Editing requires the current entry revision, so a stale host cannot overwrite a
newer edit. The mutation report returns the union of the old and new dependent
tiles; hosts invalidate and re-evaluate those tiles, preventing an edited path
from leaving its old raster trace. Undo and redo restore the retained source
entry, not a flattened image.

Rasterization is deliberately separate. `rasterization_plan` returns a stable
copy of the requested source entry and its dependent tiles without changing the
store. Decal and text hosts feed that plan into the existing paint tools.
`evaluate_editable_surface_path` converts attached control points to canonical
stroke samples, derives tangent frames from the surface normals, maps point
widths to stroke pressure and resolves them with the normal stroke model.

The canonical version-1 `editable-authoring` asset format is bounded on read and
stores all placement, text, font, path and material data. It is carried as a
normal project-container asset. Save/reopen does not persist transient undo
history, but reopened entries remain editable and the first new edit creates an
undo step restoring the saved value.

The C ABI exposes atomic add, edit, inspection, rasterization planning,
undo/redo and surface-path resolution on a texture set. JSON reports use
caller-owned sizing and include invalidated tiles. A too-small report buffer
refuses a mutation before publication. Project upsert and restore operations
round-trip the complete store through canonical container bytes.

Mesh-replacement reprojection preflights every surface attachment against the
destination mesh. Mapped points receive the destination position, normal,
triangle, barycentric coordinates and mesh revision as a revision-checked edit.
Ambiguous attachments require the explicit ambiguity policy; an unmapped
attachment refuses commit rather than silently retaining an invalid reference.
See [mesh-reprojection.md](mesh-reprojection.md).
