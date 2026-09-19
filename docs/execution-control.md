# Bounded executor work

`ctex/exec/execution_control.hpp` defines the common cancellation, progress,
worker and memory controls for long-running executor work. The CPU reference
implements them through `CpuReferenceExecutor::execute_bounded` and the staged
`CpuBoundedOperation` contract.

## Staged publication

A bounded CPU operation declares a `CpuWorkPlan` before execution:

- the number of independently schedulable work items;
- shared staging bytes; and
- scratch bytes required by each worker.

Work items receive only the executor-owned shared staging span and their own
worker scratch span. They may run concurrently and must write deterministic,
non-overlapping portions of shared staging. They must not publish document state.
The executor calls the operation's non-throwing `commit` exactly once, after all
work succeeds and one final cancellation poll remains false. Cancellation or a
work-item exception never calls `commit`, so a conforming operation leaves the
document byte-identical to its pre-operation state.

Cancellation is cooperative and is polled before each work item. Operations
must therefore divide expensive work into bounded items. Already-running items
may finish after cancellation is requested, but their staging is discarded.

## Progress and workers

`ExecutionControl::maximum_workers` is a hard upper bound on the CPU worker
threads created for the operation. The actual count is the smaller of the bound
and the work-item count. A bound of one takes the same path with one worker; the
committed result must be byte-identical to a larger bound.

Progress begins at zero, is reported at each non-zero `progress_interval`, and
ends at either the total or the processed count observed at cancellation.
Callbacks can arrive on a CPU worker thread. The executor serializes callbacks
for one operation and reports monotonically increasing counts.

## Memory admission

Before allocating executor working storage or starting a worker, the CPU path
calculates:

```text
shared bytes + actual worker count * scratch bytes per worker
```

Overflow or a value above `memory_ceiling_bytes` returns
`memory_ceiling_exceeded`. The diagnostic names the operation, required bytes,
and configured ceiling. Equality is admitted. The reported figure covers the
operation's executor-managed staging and scratch payload; standard-library
thread bookkeeping and platform thread stacks are not part of that declared
payload.

An already-requested cancellation is reported after memory preflight and before
working-storage allocation. Zero worker bounds and zero progress intervals are
invalid rather than silently changed.
