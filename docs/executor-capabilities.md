# Executor capability reporting

Every `ExecutorDescriptor` carries the complete `DeviceFeatureSet` consumed by
shader emission:

- total per-stage binding budget;
- maximum texture dimension;
- supported texture formats;
- floating-point filtering support; and
- compute availability.

The registry rejects an executor whose binding budget cannot represent a basic
pass, whose maximum texture dimension is zero, or whose format set is empty.
Capabilities are returned as part of the same stable descriptor used for
enumeration and selection, so a caller cannot accidentally associate a device
name from one executor with limits from another.

`ctex/exec/emission_features.hpp` provides `emission_request_for` overloads for
layer-stack, material and preview emission. Each takes the selected executor and
an otherwise complete request, then replaces the request's feature set with the
descriptor report. This is the supported selection-to-emission seam; callers do
not need a parallel capability registry.

The CPU reference reports every library texture format, unrestricted logical
binding and dimension ranges, floating-point filtering, and no shader-compute
device. These limits describe its device-free reference semantics rather than a
physical GPU. A configured host executor reports the limits of the host-owned
device even while detached; detachment changes availability, not device
identity or cached emission keys.

Emission continues to enforce the reported limits: unsupported formats and
oversized textures are refused, binding pressure causes explicit pass splitting,
and missing floating-point filtering returns the documented nearest-filter
workaround. `compute_available` remains available for operations that gain a
compute path later.
