# Execution backend scenario coverage

`just test-execution-backend-scenarios` runs every CTest carrying the
`execution-backends-scenario` label. The suite maps every OpenSpec scenario to
executable coverage.

| OpenSpec scenario | CTest | Covered behavior |
|---|---|---|
| Host owns the device | `executor-host-execution` | Attachment changes availability while submissions contain logical resources and states, never an API handle |
| Build with no GPU backend | `executor-vulkan`, `executor-cpu-reference` | Default build uses the no-dependency Vulkan stub and retains the always-available CPU route |
| CI machine with no GPU | `execution-backends-scenario` suite | Complete implemented executor behavior runs without loading a graphics API |
| Reference rasterization | `executor-cpu-reference`, `c-abi-cpu-reference-raster` | CPU-owned clipped viewport depth/UV and UV-space projected buffers cross C with atomic bounded output |
| Tolerance is a number | `executor-parity-tolerances` | Exact 8-bit, 16-bit, float and filtered bounds plus boundary comparisons |
| A backend drifts | `executor-parity-gate` | Excess deviation fails with fixture, channel and measured values |
| Unavailable device is reported, not skipped silently | `executor-parity-gate`, `executor-vulkan` | Unavailable routes are explicit and never counted as passing |
| Device lost mid-operation | `executor-host-execution`, `executor-registry` | Recovery restores the commit before a device-lost Vulkan-to-CPU fallback report is admitted |
| Selection without a rebuild | `executor-environment-pin`, `executor-registry` | `CTEX_EXECUTOR` selects a compiled route and unknown input preserves automatic policy |
| Emission matches the device | `executor-emission-features` | Selected descriptor limits replace unrelated request features and alter emitted passes/filtering |
| Host returns the wrong size | `executor-host-execution` | Actual and expected extents are named and revision remains unchanged |
| Cancelling an expensive fill | `executor-bounded-execution` | A 16,384-unit staged fill cancels halfway with no commit or document mutation |
| Single-threaded reproducibility | `executor-bounded-execution` | One-worker and four-worker executions commit byte-identical results |
| Over budget | `executor-bounded-execution` | Required and ceiling bytes are named before storage allocation or work |
| Layering violation | `execution-backends-layering` | A Vulkan include outside `exec` fails naming its file and line |
| Late completion after cancellation | `executor-host-execution` | Output stays pinned until late completion, then is discarded without publication |
| Ordinary painting stays on the device | `executor-host-execution` | Logical host-resident output remains authoritative; publication and undo-generation retirement move no pixels |
| Device loss after a committed stroke | `executor-host-execution` | Last checkpointed revision survives while uncommitted submissions are cancelled and released |

The 16K fill is an execution-layer staged-operation stand-in because the paint
tool itself lands in tasks 9–10. Its cancellation, progress, worker and memory
semantics are the same contract those operations must use. Likewise, the
enabled Vulkan configuration separately runs `just test-vulkan` against a real
or software device; future Vulkan paint renderers extend the parity corpus
rather than substituting CPU output under the Vulkan identity.
