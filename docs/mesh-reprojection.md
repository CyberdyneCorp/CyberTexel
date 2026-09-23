# Mesh reprojection

Mesh replacement keeps the current mesh, channel pixels, history and editable
attachments authoritative while reprojection is pending. A preflight maps each
covered destination texel back to a source-mesh point and reports the mapping
before any document or mesh mutation.

`MeshReprojectionLimits` makes the acceptance envelope explicit:

- `maximum_distance` rejects source surfaces outside the host's cage distance.
- `maximum_normal_angle_radians` rejects incompatible surface orientation.
- `require_visibility` rejects a mapping when another source triangle occludes
  the segment to the selected surface; `visibility_epsilon` controls endpoint
  tolerance.
- `ambiguity_distance_epsilon` classifies equivalently close candidates.
- `maximum_work_items` bounds triangle and visibility tests. Cancellation and
  progress are checked at the same work-item boundary.

The preflight records mapped, unmapped and ambiguous destination texels,
texture-set identities, source/destination triangles, barycentric coordinates,
source UVs, distances and angles. It also maps every surface-path attachment
that names the source mesh revision. Channel revision cursors and editable-entry
revisions are captured so a commit refuses stale source content rather than
applying a mapping different from the one the host inspected.

Commit requires two independent policies. Holes either retain the current
target texel or receive the channel default. Ambiguities either refuse the
operation or resolve by nearest distance followed by the lowest source triangle
identifier. An unmapped editable attachment always refuses publication because
there is no valid destination surface on which to retain it.

Every enabled channel and affected editable store is staged before publication.
Normal-vector channels are decoded from their source tangent basis, transformed
through object space, and encoded in the interpolated destination tangent basis.
Supplied geometry and UV buffers remain read-only. Failure, cancellation,
budget exhaustion, stale state, and unresolved policy leave the current mesh,
pixels and history unchanged.

At the C boundary, call
`ctex_mesh_replacement_plan_preflight_reprojection` after replacement-policy
apply reports a pending reprojection. Its caller-owned JSON output exposes the
full mapping. `ctex_mesh_replacement_plan_commit_reprojection` stages the
document changes and then publishes the replacement mesh in the same successful
operation. The replacement plan continues to own both mesh states until that
commit completes.
