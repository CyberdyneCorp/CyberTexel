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
| `text.tracking_em` | 0 | -10 | 10 | em |
| `text.size` | 1 | 0.000001 | 1,000,000 | surface units per em |
| `particle.count` | 1 | 1 | 100,000 | particles |
| `particle.lifetime_seconds` | 1 | 0.000001 | 60 | seconds |
| `particle.initial_speed` | 1 | 0 | 1,000,000 | surface units per second |
| `particle.mass` | 1 | 0.000001 | 1,000,000 | relative mass |
| `particle.gravity.x` | 0 | -1,000,000 | 1,000,000 | surface units per second² |
| `particle.gravity.y` | -9.81 | -1,000,000 | 1,000,000 | surface units per second² |
| `particle.gravity.z` | 0 | -1,000,000 | 1,000,000 | surface units per second² |
| `particle.friction` | 0.5 | 0 | 1 | normalized |
| `particle.restitution` | 0 | 0 | 1 | normalized |
| `particle.randomness` | 0 | 0 | 1 | normalized |
| `decal.rotation_radians` | 0 | -2π | 2π | radians |
| `decal.uniform_scale` | 1 | 0.000001 | 1,000,000 | surface units |
| `decal.axis_scale.x` | 1 | 0.000001 | 1,000,000 | multiplier |
| `decal.axis_scale.y` | 1 | 0.000001 | 1,000,000 | multiplier |
| `stencil.position.x` | 0 | -1,000,000 | 1,000,000 | screen units |
| `stencil.position.y` | 0 | -1,000,000 | 1,000,000 | screen units |
| `stencil.rotation_radians` | 0 | -2π | 2π | radians |
| `stencil.scale.x` | 1 | 0.000001 | 1,000,000 | screen units |
| `stencil.scale.y` | 1 | 0.000001 | 1,000,000 | screen units |
| `projection.planar.extent.x` | 1 | 0.000001 | 1,000,000 | surface units |
| `projection.planar.extent.y` | 1 | 0.000001 | 1,000,000 | surface units |
| `projection.triplanar.scale` | 1 | 0.000001 | 1,000,000 | repetitions per surface unit |
| `projection.triplanar.offset.x` | 0 | -1 | 1 | periodic image coordinate |
| `projection.triplanar.offset.y` | 0 | -1 | 1 | periodic image coordinate |
| `rejection.depth_bias` | 0.0001 | 0 | 1,000,000 | projected-depth units |
| `rejection.minimum_normal_dot` | 0.5 | -1 | 1 | dot product |
| `alpha_discard.threshold` | 0.1 (8-bit); 0.004 (16-bit/float) | 0 | 1 | normalized |

`StrokeResolver::settings()` exposes the resolved settings and
`parameter_report()` exposes their clamps. Brush and Eraser consume the
resolved stroke, so they cannot bypass or reinterpret the parameter decision.
Stamp-count taper extents must remain integral after range resolution. Disabled
tapers resolve their otherwise inert extent to zero. The remaining tool
descriptors and entry points are tracked by roadmap task 10.12; that task
remains incomplete until the behavioral no-inert audit also covers every
documented parameter.
