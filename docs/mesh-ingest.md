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

`analyze_uv_overlaps` examines one named UV set within one partition, matching
the boundary of a texture set. It reports unique affected face indices in
ascending order and counts both broad-phase candidate pairs and confirmed
positive-area overlaps. Shared edges or vertices and degenerate UV triangles do
not count as overlaps. The sweep broad phase is deterministic and avoids
narrow-phase intersection work for faces whose UV bounds cannot intersect;
faces assigned to other partitions are never compared.

## Tangent frames

Every validated mesh exposes one tangent frame per triangle corner through
`tangent_frames()`. A host may supply `Vec4f` tangents, with XYZ as a unit
direction and W as the `+1`/`-1` bitangent sign. Supplied values require a
`TangentFrameDescriptor` naming the algorithm and version, normal orientation,
coordinate-system handedness, UV V-axis convention, signed-W encoding and UV
set. Missing declarations, non-finite or non-orthogonal values, invalid signs,
and buffers that are not exactly one value per corner are refused.

When tangents are absent, CyberTexel generates them with
`ctex_uv_derivative_version == 1`: per triangle it differentiates position by
the selected UV set, Gram-Schmidt orthonormalizes the result against each
corner's vertex normal, and records the bitangent sign in W. Degenerate UV
triangles use a deterministic perpendicular axis. The generated descriptor is
right-handed, uses the supplied vertex-normal orientation, an upward UV V axis and the mesh's
default UV set. The algorithm name and version are part of compatibility, so a
future implementation change cannot silently reinterpret an existing normal
map.

`tangent_space_to_object` derives the bitangent as
`cross(normal, tangent.xyz) * tangent.w`. Consequently a shared tangent-space
normal texture follows both ordinary and mirrored UV islands without losing
their per-corner handedness.

`TextureDocument::create_texture_sets_from_mesh` derives one texture set per
validated partition and binds each to a requested named UV set. The caller
chooses the non-square-capable resolution and 8-, 16-, or 32-bit default channel
precision for those sets.

`MeshBinding` associates a validated view with an opaque revision. Call
`replace()` after changing any caller-owned geometry, UV, or partition buffer;
successful replacement advances the revision so derived structures can rebuild.
