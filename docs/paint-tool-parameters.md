# Paint-tool parameter validation

`<ctex/paint/parameters.hpp>` is the shared numeric-validation seam for paint
tools and, later, every language binding. A `ToolParameterDescriptor` gives a
stable name plus its default and inclusive minimum and maximum. Finite values
outside the range are clamped. Every clamp appends the stable name, supplied
value, and resolved value to a `ToolParameterReport`; in-range values produce
no report entry. Non-finite inputs and internally inconsistent descriptors are
refused without modifying the report.

The first routed parameter is `stroke.radius`:

| Parameter | Default | Minimum | Maximum | Unit |
|---|---:|---:|---:|---|
| `stroke.radius` | 1 | 0.000001 | 1,000,000 | caller-defined surface unit |

`StrokeResolver::settings()` exposes the resolved radius and
`parameter_report()` exposes its clamp. Brush and Eraser consume the resolved
stroke, so they cannot bypass or reinterpret the parameter decision. The
remaining tool descriptors and entry points are tracked by roadmap task 10.12;
that task remains incomplete until the behavioral no-inert audit also covers
every documented parameter.
