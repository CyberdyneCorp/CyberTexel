# CPU reference executor

`ctex/exec/cpu_reference.hpp` provides the mandatory, always-available `cpu`
executor. It owns no graphics device and makes no graphics API calls. Its output
defines correctness for other executors; it is an independent implementation,
not a fallback implementation of a host or GPU path.

## Operation contract

Every executable library operation must provide `CpuOperation` semantics. The
CPU executor accepts that contract directly, executes it once, and returns a
record containing the stable operation identifier. A feature is not complete
if it supplies only shader or GPU semantics. Later feature tasks add their CPU
operation types alongside their host pass plans; capability reporting in 7.4
will expose that inventory rather than creating a second dispatch system.

`CpuReferenceExecutor` has a permanently `available` descriptor and names the
system CPU rather than a graphics device. Selection and fallback continue to use
the common executor registry. Actual recovery-before-fallback remains owned by
task 7.3.

## Independent rasterization

The CPU path consumes a backend-neutral `CpuRasterMeshView`: object-space
positions, one selected UV set, and indexed triangles. A column-major OpenGL
view-projection matrix uses clip depth `[-w, +w]`; viewport coordinates have a
top-left origin.

`rasterize_viewport` clips triangles against all six homogeneous clip planes,
rasterizes pixel centers, depth-tests them, and produces its own arrays for:

- depth, mapped from NDC to `[0, 1]` with `1` as the clear value;
- perspective-correct UV;
- explicit byte coverage; and
- source triangle identity.

Equal-depth ownership is resolved by the lowest source triangle index, making
shared edges deterministic and independent of traversal details.

`rasterize_uv` rasterizes the selected UV set into a requested texture tile. It
produces coverage and triangle identity plus each texel's independently
projected viewport position and camera depth. `tile_origin` supports integer UV
tiles without changing the source coordinates. Overlapping UVs use the lowest
triangle index until the mesh overlap-policy work in task 4.5 supplies a richer
diagnostic.

These buffers are the inputs for the depth, angle, backface, alpha, and stroke
capsule tests introduced by the paint engine in tasks 9.7–9.9. The CPU path will
apply those tests itself; it never consumes a host-rendered depth or UV buffer.
The executor parity fixture and numeric tolerances arrive in tasks 7.5–7.6.

Input validation rejects zero dimensions, incomplete or out-of-range triangles,
mismatched attributes, non-finite mesh/camera data, and non-finite transformed
coordinates before exposing a result.
