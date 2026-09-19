# Paint-tool parameter validation

`<ctex/paint/parameters.hpp>` is the shared numeric-validation seam for paint
tools and, later, every language binding. A `ToolParameterDescriptor` gives a
stable name plus its default and inclusive minimum and maximum. Finite values
outside the range are clamped. Every clamp appends the stable name, supplied
value, and resolved value to a `ToolParameterReport`; in-range values produce
no report entry. Non-finite inputs and internally inconsistent descriptors are
refused without modifying the report.

The currently routed paint-tool parameters are:

| Parameter | Default | Minimum | Maximum | Unit |
|---|---:|---:|---:|---|
| `stroke.spacing_fraction` | 0.1 | 0.01 | 4 | radius fraction |
| `stroke.radius` | 1 | 0.000001 | 1,000,000 | caller-defined surface unit |
| `stroke.opacity` | 1 | 0 | 1 | normalized |
| `stroke.hardness` | 1 | 0 | 1 | normalized |
| `stroke.rotation_radians` | 0 | -2π | 2π | radians |
| `stroke.elongation` | 1 | 0.01 | 100 | ratio |
| `stroke.flow` | 1 | 0 | 1 | normalized |
| `stroke.stabilizer.radius` | 0 | 0 | 1,000,000 | caller-defined surface unit |
| `stroke.stabilizer.time_constant_seconds` | 0 | 0 | 60 | seconds |
| `stroke.jitter.position_fraction` | 0 | 0 | 4 | radius fraction |
| `stroke.jitter.radius_fraction` | 0 | 0 | 0.99 | radius fraction |
| `stroke.jitter.rotation_radians` | 0 | 0 | 2π | radians |
| `stroke.jitter.opacity` | 0 | 0 | 1 | normalized |
| `stroke.jitter.flow` | 0 | 0 | 1 | normalized |
| `stroke.taper.floor` | 0 | 0 | 1 | normalized |
| `stroke.constraint.grid_step` | 1 | 0.000001 | 1,000,000 | caller-defined surface unit |
| `stroke.symmetry.radial_count` | 1 | 1 | 4,096 | instances |
| `stroke.input.pressure_radius.minimum_output` | 0.01 | 0.01 | 100 | radius multiplier |
| `stroke.input.pressure_radius.maximum_output` | 1 | 0.01 | 100 | radius multiplier |
| `stroke.input.pressure_opacity.minimum_output` | 0 | 0 | 1 | normalized multiplier |
| `stroke.input.pressure_opacity.maximum_output` | 1 | 0 | 1 | normalized multiplier |
| `stroke.input.pressure_hardness.minimum_output` | 0 | 0 | 1 | normalized multiplier |
| `stroke.input.pressure_hardness.maximum_output` | 1 | 0 | 1 | normalized multiplier |
| `stroke.input.pressure_flow.minimum_output` | 0 | 0 | 1 | normalized multiplier |
| `stroke.input.pressure_flow.maximum_output` | 1 | 0 | 1 | normalized multiplier |
| `stroke.input.pressure_rotation.minimum_output` | 0 | -2π | 2π | additive radians |
| `stroke.input.pressure_rotation.maximum_output` | 1 | -2π | 2π | additive radians |
| `stroke.input.tilt_rotation.minimum_output` | 0 | 0 | 1 | azimuth multiplier |
| `stroke.input.tilt_rotation.maximum_output` | 1 | 0 | 1 | azimuth multiplier |
| `stroke.input.tilt_elongation.minimum_output` | 1 | 0.01 | 100 | elongation multiplier |
| `stroke.input.tilt_elongation.maximum_output` | 2 | 0.01 | 100 | elongation multiplier |
| `stroke.taper.entry.extent` | 0 (disabled) | 0 when disabled; 2 stamps; 0.000001 distance | 0 when disabled; 1,000,000 stamps or distance | selected taper unit |
| `stroke.taper.exit.extent` | 0 (disabled) | 0 when disabled; 2 stamps; 0.000001 distance | 0 when disabled; 1,000,000 stamps or distance | selected taper unit |
| `blur.radius` | 1 | 1 | 4,096 | texels |
| `smear.strength` | 0.5 | 0 | 1 | normalized |
| `smear.footprint.radius_x` | 1 | 0 | 4,096 | texels |
| `smear.footprint.radius_y` | 1 | 0 | 4,096 | texels |
| `paint.connected.maximum_angle_degrees` | 45 | 0 | 180 | degrees |
| `colour_id.tolerance` | 0 | 0 | √3 | normalized linear-RGB distance |

`StrokeResolver::settings()` exposes the resolved settings and
`parameter_report()` exposes their clamps. Brush and Eraser consume the
resolved stroke, so they cannot bypass or reinterpret the parameter decision.
Stamp-count taper extents must remain integral after range resolution. Disabled
tapers resolve their otherwise inert extent to zero. The remaining tool
descriptors and entry points are tracked by roadmap task 10.12; that task
remains incomplete until the behavioral no-inert audit also covers every
documented parameter.
