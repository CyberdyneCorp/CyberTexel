# Material node catalogue

`ctex/graph/catalogue.hpp` exposes the immutable built-in catalogue. Every node
type has a stable `ctex.*` identifier, version, category, typed input and output
sockets, and property declarations. Choice properties include their allowed
values. `make_builtin_node` copies a declaration into a regular `GraphNode` with
its defaults and an optional editor position.

The catalogue declares structure and portable semantics. `portable_nodes.hpp`
provides the conformance CPU formulas for seeded Noise and all twenty Blend
modes; other catalogue entries remain declarations until their owning executor
tasks land. Resource validation and shader emission remain separate from the
catalogue, so a node's presence does not by itself imply executable semantics.

## Built-in families

| Family | Built-in nodes |
|---|---|
| Input | Constant Value, Constant Colour, Texture Coordinate, UV Set, Geometry, Object Info, Mesh Map, Layer Reference, Anchor Point, Picker |
| Texture | Image, Noise, Voronoi, Gradient, Checker, Brick, Wave, Magic, Gabor, Tile Sheet |
| Colour and filter | Mix, Blend, Levels, Curves, Colour Ramp, Hue/Saturation/Value, Brightness/Contrast, Gamma, Invert, Quantize, Replace Colour, Colour Mask, Separate Colour, Combine Colour, Blur, Sharpen, Warp, Grayscale Conversion |
| Vector and math | Math, Vector Math, Map Range, Clamp, Mapping, Normal Map, Mix Normal Map, Bump, Separate XYZ, Combine XYZ, Vector Rotate, Vector Transform, Float Curve, Vector Curves |

The Blend node exposes the same complete set used by the shared
`blend_colour` reference formula:
Normal, Darken, Multiply, Color Burn, Lighten, Screen, Color Dodge, Add, Overlay,
Soft Light, Linear Light, Difference, Exclusion, Subtract, Divide, Hue,
Saturation, Color, Value, and Pass Through. Node conformance fixtures and the
document layer stack consume that one formula surface rather than maintaining
independent CPU tables. The ordered definitions also carry the explicit formula
text used by the texture-document specification and host-facing catalogues.

Seeded Noise uses stable integer hashing, smooth value-noise interpolation and
bounded fractal octaves. Equal coordinates, seed, scale, detail, roughness,
lacunarity and distortion produce byte-identical CPU results. Its scalar factor
and decorrelated RGB result remain normalized to `[0, 1]`.

## Scalar Math operations

Inputs are `a`, `b`, and `c`. Undefined real-domain operations and division by
zero produce zero as stated below, avoiding backend-specific NaN behavior.

| Operation | Formula |
|---|---|
| Add | `a + b` |
| Subtract | `a - b` |
| Multiply | `a * b` |
| Divide | `b == 0 ? 0 : a / b` |
| Multiply Add | `a * b + c` |
| Power | `pow(a, b)` in the real domain; otherwise `0` |
| Logarithm | `a > 0 && b > 0 && b != 1 ? log(a) / log(b) : 0` |
| Square Root | `sqrt(max(a, 0))` |
| Inverse Square Root | `a > 0 ? 1 / sqrt(a) : 0` |
| Absolute | `abs(a)` |
| Exponent | `exp(a)` |
| Minimum | `min(a, b)` |
| Maximum | `max(a, b)` |
| Less Than | `a < b ? 1 : 0` |
| Greater Than | `a > b ? 1 : 0` |
| Sign | `a < 0 ? -1 : (a > 0 ? 1 : 0)` |
| Compare | `abs(a - b) <= max(c, 0) ? 1 : 0` |
| Smooth Minimum | For `c > 0`, `h = max(c - abs(a - b), 0) / c`, then `min(a, b) - h*h*c/4`; otherwise `min(a, b)` |
| Smooth Maximum | `-smooth_minimum(-a, -b, c)` |
| Round | `floor(a + 0.5)` |
| Floor | `floor(a)` |
| Ceil | `ceil(a)` |
| Truncate | `trunc(a)` |
| Fraction | `a - floor(a)` |
| Modulo | `b == 0 ? 0 : a - b * floor(a / b)` |
| Wrap | `b == c ? b : a - (c-b) * floor((a-b)/(c-b))` |
| Snap | `b == 0 ? 0 : floor(a / b) * b` |
| Ping-Pong | `b <= 0 ? 0 : b - abs(modulo(a, 2*b) - b)` |
| Sine | `sin(a)` |
| Cosine | `cos(a)` |
| Tangent | `tan(a)` |
| Arcsine | `asin(clamp(a, -1, 1))` |
| Arccosine | `acos(clamp(a, -1, 1))` |
| Arctangent | `atan(a)` |
| Arctan2 | `atan2(a, b)` |
| Hyperbolic Sine | `sinh(a)` |
| Hyperbolic Cosine | `cosh(a)` |
| Hyperbolic Tangent | `tanh(a)` |
| To Radians | `a * pi / 180` |
| To Degrees | `a * 180 / pi` |

## Vector Math operations

Unless stated otherwise, an operation is component-wise. `a`, `b`, and `c` are
vectors and `scale` is scalar. Scalar results use the node's Value output.

| Operation | Formula |
|---|---|
| Add | `a + b` |
| Subtract | `a - b` |
| Multiply | `a * b` |
| Divide | `a / b`; a zero divisor component produces `0` |
| Multiply Add | `a * b + c` |
| Cross Product | `cross(a, b)` |
| Project | `dot(a, b) / dot(b, b) * b`; zero `b` produces zero |
| Reflect | `a - 2 * dot(a, n) * n`, where `n = normalize(b)` |
| Refract | `refract(a, normalize(b), scale)`; total internal reflection produces zero |
| Faceforward | `dot(c, b) < 0 ? a : -a` |
| Dot Product | `dot(a, b)` on Value |
| Distance | `length(a - b)` on Value |
| Length | `length(a)` on Value |
| Scale | `a * scale` |
| Normalize | `length(a) == 0 ? zero : a / length(a)` |
| Absolute | `abs(a)` |
| Minimum | `min(a, b)` |
| Maximum | `max(a, b)` |
| Floor | `floor(a)` |
| Ceil | `ceil(a)` |
| Fraction | `a - floor(a)` |
| Modulo | `a - b * floor(a / b)`; a zero divisor component produces `0` |
| Wrap | Wrap each component of `a` between the corresponding `b` and `c` bounds |
| Snap | `floor(a / b) * b`; a zero step component produces `0` |
| Sine | `sin(a)` |
| Cosine | `cos(a)` |
| Tangent | `tan(a)` |

## Normal-map composition

Mix Normal Map accepts decoded unit tangent-space normals `n1` and `n2` and
offers all three required modes. Every result is normalized.

| Mode | Formula before normalization |
|---|---|
| Partial Derivative | `(n1.xy * n2.z + n2.xy * n1.z, n1.z * n2.z)` |
| Whiteout | `(n1.xy + n2.xy, n1.z * n2.z)` |
| Reoriented | With `t = n1 + (0,0,1)` and `u = n2 * (-1,-1,1)`, `t * dot(t,u) / t.z - u` |

The Factor input blends from `n1` to the selected composed result before the
final normalization. Tangent-basis validation and conversion are owned by the
mesh-map and graph-validation tasks.
