# Mesh ingest

`ctex::mesh::MeshView` accepts triangulated geometry entirely from caller-owned
memory. CyberTexel does not open geometry files and does not own or modify the
position, normal, optional vertex-colour, index, UV, or partition buffers.
Callers must keep every referenced buffer and string alive for the lifetime of
the view.

Positions and normals are required and have equal vertex counts. Vertex colours
are optional, but when present also have one value per vertex. Indices describe
complete triangles and every index must address a supplied vertex. UV sets have
unique, non-empty names, one value per vertex, and include the named default.
The view exposes the validated vertex, triangle and UV-set counts and whether
vertex colours are present through `attributes()`.

Each face has one partition index. That fixed-width assignment makes
partitioning total and non-overlapping: a missing assignment or an index that
does not identify a supplied partition is rejected. Partitions identify their
source as material, object, submesh, or an explicit host-supplied face set, and
carry a stable key independent of their order and display name. Each face also
carries one numeric material identifier for picking and material-ID workflows.
All supplied floating-point attributes must be finite.

`TextureDocument::create_texture_sets_from_mesh` derives one texture set per
validated partition and binds each to a requested named UV set. The caller
chooses the non-square-capable resolution and 8-, 16-, or 32-bit default channel
precision for those sets.

`MeshBinding` associates a validated view with an opaque revision. Call
`replace()` after changing any caller-owned geometry, UV, or partition buffer;
successful replacement advances the revision so derived structures can rebuild.
