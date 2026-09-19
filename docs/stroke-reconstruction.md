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
unit. Pressure is either absent or normalized to `[0, 1]`. Tilt is a normalized
2D vector of magnitude at most one: its magnitude is the tilt amount and its
angle is the pen azimuth. Every numeric input must be finite, and each frame
must have a usable normal and tangent. Invalid batches are rejected before any
of their samples are appended.

The version-1 positional tolerance is `1e-6` units and the fixed stabilization
step is `1,000,000` nanoseconds (1 ms). Reconstruction proceeds as follows:

1. An interior sample is removed when its position, all three frame vectors,
   effective pressure and tilt lie within the positional tolerance of the
   timestamp-linear interpolation between its neighbors. Absent pressure has
   an effective value of one, so a mouse and a full-pressure pen are equivalent.
2. The remaining piecewise-linear path is sampled at every retained knot and
   every 1 ms grid point anchored to the first timestamp. Retained knots keep
   supplied corners; the fixed grid makes callback batching irrelevant.
3. Position, pressure, tilt and frame components interpolate linearly. Frames
   are then orthonormalized while preserving their supplied handedness. The
   deterministic nearer endpoint resolves an exactly opposite interpolated axis.
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
rotation, elongation, flow, tip resource identity and ordinal. Pressure and
tilt response is applied while creating this one canonical sequence. Taper and
jitter then alter its resolved properties without creating a second sequence.

## Pressure and tilt response

A `ResponseCurve` is piecewise-linear over normalized input. It must contain at
least two finite points, begin at input zero, end at input one, have strictly
increasing inputs, and keep every input and output in `[0, 1]`. A
`ResponseMapping` maps the curve result linearly into its declared minimum and
maximum output. Each target has its own enable flag, curve and output range.

Pressure can independently multiply radius, opacity, hardness and flow. It can
also add radians to rotation. Radius response is enabled by default with a
linear curve and a factor range of `0.01` through `1.0`; the other pressure
targets are disabled by default. Thus the default pen changes radius only. An
absent pressure value always evaluates as `1.0`, including when optional
pressure mappings are enabled. Radius factors must remain positive; opacity,
hardness and flow factors remain in `[0, 1]`.

Tilt mappings are disabled by default. When tilt rotation is enabled, the
tilt-vector azimuth is multiplied by its response value and added to the base
rotation. When tilt elongation is enabled, the base elongation is multiplied by
its response to tilt magnitude; its default output range is `1.0` through
`2.0`. Zero tilt adds no rotation. Tilt-rotation response is constrained to
`[0, 1]`, and elongation factors must remain positive.

The radius resolved at one stamp determines the arc-length spacing to the next
stamp. Consequently pressure affects both visible radius and subsequent event
placement, while batching and redundant-sample invariance remain intact.

## Constraints and stabilization

One positional constraint can be active. `straight_line` replaces every
reconstructed position with timestamp-linear motion from the first position to
the final position, so intermediate positional bends have no effect while their
pressure, tilt and frame knots remain. `dominant_axis` selects the largest
absolute final displacement, with X then Y then Z as the tie order, and locks
the other two coordinates to the stroke origin. These directional constraints
run before the timestamp stabilizer.

`grid` instead rounds each stabilized reconstructed position independently to
the nearest multiple of its positive step, using the C++ `round` half-away-from-
zero rule. Spacing then follows the piecewise-linear path through those snapped
points; regular stamps between two grid points need not themselves fall on grid
intersections. The default constraint is `none`.

## Taper and deterministic jitter

Entry and exit taper are independent. Each can be disabled, measured in path
distance, or measured in stamp count. An active stamp-count span is an integer
of at least two: for a ten-stamp entry span, ordinal zero is at the taper floor
and ordinal nine is at full strength. A distance span reaches full strength at
its named distance. Exit progress is measured from the final stamp. When both
are active, the smaller progress wins, and the normalized factor is
`floor + (1 - floor) * progress`. Radius, opacity or both can be selected.

Taper operates on the already placed, pressure-resolved stamps and therefore
does not recursively alter their placement. Distances are measured before
position jitter. The taper floor is normalized to `[0, 1]` and defaults to
zero; taper is disabled by default.

Jitter is stateless and reproducible. Each random value is keyed by the 64-bit
stroke seed, stamp ordinal and a fixed target channel, mixed with the SplitMix64
finalizer, then converted from its high 53 bits to `[-1, 1)`. Position uses
separate tangent and bitangent channels, each scaled by the configured fraction
of the tapered radius. Radius uses a relative fraction smaller than one;
rotation adds radians; opacity and flow add absolute normalized amounts and
clamp to `[0, 1]`. Jitter is applied after taper and does not change stamp
ordinals, sweep links or placement count. Every jitter amount defaults to zero.

In `continuous_sweep` mode every pair of consecutive stamps has a
`SweptSegment`. A final endpoint stamp closes any residual shorter-than-spacing
path before post-placement modifiers, and sweep links then connect the final
jittered stamp positions. In `discrete_alpha` mode only regular spacing events
are emitted and the segment list is empty; there is no forced endpoint tip and
no implicit coverage between transformed alpha tips. Spacing greater than two
radii can therefore leave the explicitly requested visible separation.

An unsupported reconstruction version, invalid setting, non-monotonic sample,
empty resolution, append after resolution, or second resolution is refused.
The resolver stores no render-frame or GPU-batch state, so submitting the same
timestamped samples individually or in coalesced batches yields the same
stamps and sweep links.
