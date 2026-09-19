# Executor discovery and selection

`ctex/exec/executor.hpp` defines the process-local registry shared by the three
execution routes. An executor publishes a stable identifier, display name,
device name, route, runtime availability, and the
[features consumed by emission](executor-capabilities.md). Registration is instance-owned,
rejects incomplete or duplicate descriptors, and enumeration is sorted by
identifier so plugin discovery order cannot leak into a UI or cache key.

The registry distinguishes compiled-in executors from executors usable at this
instant. `enumerate()` includes unavailable devices with either
`device-unavailable` or `host-not-attached`; `available()` contains only usable
entries. Explicit selection may retain an unavailable compiled-in executor so
an operation can report its failure and apply recovery policy rather than
silently pretending it was never selected.

## Default policy

Without a pin, automatic selection prefers an available owned-GPU executor,
then the CPU reference, then an attached host-executed route. Equal routes are
ordered by stable identifier. `pin_default` overrides that policy until cleared.

A consumer without backend UI can set:

```text
CTEX_EXECUTOR=<stable executor identifier>
```

A recognized compiled-in identifier becomes the process selection, including
its explicit availability state. An empty value uses automatic policy. An
unknown value leaves automatic selection in place and returns a message naming
the ignored request; environment input never invents an executor.

## Failure and fallback report

`ExecutorFallbackReport` records the failed executor, typed failure, detail,
recovery state, disposition, fallback identity, and a human-readable message.
The checked factory refuses to describe CPU fallback unless recovery has first
been restored and the CPU executor is named. It can instead report
`recovery-required` or `no-fallback` without claiming partial work was committed.

The mandatory [CPU reference executor](cpu-reference-executor.md) supplies the
always-available `cpu` implementation and independent raster buffers. Host
[submission and completion](host-execution.md) provides revision-safe atomic
publication, resource retirement, recovery and actual fallback. Device feature
reporting feeds emission through the same descriptor. The optional owned-GPU
provider arrives in 7.8. All implementations register against this interface
rather than adding parallel selection or capability mechanisms.
