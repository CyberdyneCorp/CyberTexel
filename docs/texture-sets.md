# Texture sets

A texture set binds one mesh partition source to one named UV set. It owns its
resolution, default bit depth and semantic channel storage independently from
every other set in the document.

Identity is derived from the partition source kind, the host-supplied stable
partition key and the UV-set name. Display names and insertion order do not
participate, so reordered or renamed submeshes still match when a mesh is
replaced. Length-prefixed identity components prevent delimiter collisions.

The document rejects duplicate identities. Resolution must be non-zero and the
default channel precision must be 8, 16 or 32 bits. A validated
`ctex::mesh::MeshView` can derive one set for every total, non-overlapping face
partition; see [Mesh ingest](mesh-ingest.md).
