# Picking rays

`ctex::pick::ray_from_screen` constructs a CPU world-space ray from a floating
screen position, integer viewport size, view matrix, projection matrix and an
explicit perspective/orthographic projection kind.

Matrices are column-major and multiply column vectors. The combined transform
is `projection * view`. Clip-space depth ranges from -1 on the near plane to +1
on the far plane. Screen coordinates use a top-left origin, so `(0, 0)` maps to
NDC `(-1, +1)` and `(width, height)` maps to `(+1, -1)`.

Perspective rays begin at the camera's world-space origin. Orthographic rays
begin at the unprojected near-plane position and remain parallel as the screen
position changes. Directions are normalized. Empty viewports, non-finite screen
positions, singular matrices and degenerate unprojected directions are rejected.
