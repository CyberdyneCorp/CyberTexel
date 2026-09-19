# Canonical stroke reconstruction

`ctex/paint/stroke.hpp` defines reconstruction version 1. A `StrokeResolver`
accepts timestamped 3D `StrokeInputSample` values in one or more batches and
produces exactly one `ResolvedStroke` sequence. Paint, preview, history and
later replay code consume its stamps; they do not independently reinterpret
the input callbacks.

## Version 1 input contract

Timestamps are unsigned integer nanoseconds and must be strictly increasing
across every appended batch. Positions and the tangent/bitangent/normal frame
use one caller-consistent 3D unit system; the configured radius uses that same
unit. Every numeric input must be finite, and each frame must have a usable
normal and tangent. Invalid batches are rejected before any of their samples
are appended.

The version-1 positional tolerance is `1e-6` units and the fixed stabilization
step is `1,000,000` nanoseconds (1 ms). Reconstruction proceeds as follows:

1. An interior sample is removed when its position and all three frame vectors
   lie within the positional tolerance of the timestamp-linear interpolation
   between its neighbors.
2. The remaining piecewise-linear path is sampled at every retained knot and
   every 1 ms grid point anchored to the first timestamp. Retained knots keep
   supplied corners; the fixed grid makes callback batching irrelevant.
3. Position and frame components interpolate linearly. Frames are then
   orthonormalized while preserving their supplied handedness. The deterministic
   nearer endpoint resolves an exactly opposite interpolated axis.
4. The stabilizer updates once per reconstructed point using its actual
   timestamp delta.

For raw point `p`, prior stabilized cursor `c`, stabilizer radius `r`, elapsed
seconds `dt`, and time constant `tau`, a point inside `r` leaves `c` unchanged.
Otherwise the boundary target is `b = p - normalize(p - c) * r`, and the new
cursor is `c + (b - c) * (1 - exp(-dt / tau))`. A zero time constant applies
the full correction. A zero radius and zero time constant therefore reproduce
the reconstructed input path exactly; a nonzero radius is a dead zone, not a
promise that lag can never exceed that radius.

## Stamp spacing and tip modes

The first resolved point always emits stamp ordinal zero. Further regular
stamps are placed by arc length at `spacing_fraction * radius`, carrying the
interpolated coordinate frame. The spacing fraction defaults to `0.1` and its
inclusive valid range is `0.01` through `4.0`. It is independent of input frame
rate because distances accumulate across reconstructed path segments.

Each stamp carries position, coordinate frame, radius, opacity, hardness,
rotation, elongation, flow, tip resource identity and ordinal. Task 9.1 takes
those properties directly from `StrokeSettings`; pressure/tilt response,
jitter and taper alter them in later stroke-model tasks without creating a
second stamp sequence.

In `continuous_sweep` mode every pair of consecutive stamps has a
`SweptSegment`. A final endpoint stamp closes any residual shorter-than-spacing
path so continuous coverage reaches the stabilized cursor. In `discrete_alpha`
mode only regular spacing events are emitted and the segment list is empty;
there is no forced endpoint tip and no implicit coverage between transformed
alpha tips. Spacing greater than two radii can therefore leave the explicitly
requested visible separation.

An unsupported reconstruction version, invalid setting, non-monotonic sample,
empty resolution, append after resolution, or second resolution is refused.
The resolver stores no render-frame or GPU-batch state, so submitting the same
timestamped samples individually or in coalesced batches yields the same
stamps and sweep links.
