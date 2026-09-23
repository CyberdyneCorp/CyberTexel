# Examples scenario coverage

`just test-examples-scenarios` runs every CTest carrying the `examples-scenario`
label; those checks need no wheel. `just examples` builds and installs the wheel,
runs each numbered example twice, compares its committed output and records the
C ABI call trace that the feature-coverage gate decides on.

| OpenSpec scenario | Executable evidence | What is asserted |
| --- | --- | --- |
| One artefact, two jobs | `examples-runner-policy` | Each example asserts its own result and writes its committed output; either failure fails the run. |
| New capability added | `examples-capability-coverage` | A capability with no numbered example fails the gate by name. |
| A blend mode changes | `examples-runner-policy` | An output that no longer matches the committed form is reported with the differing file and its measured pixel error. |
| A change breaks a picture | `examples-runner-policy` | Comparison runs against committed output at the declared tolerance rather than deferring the discovery. |
| An intended visual change | `examples-runner-policy` | `update` restages committed output atomically and only from the CPU reference executor. |
| Clean environment | `examples-readability` | No example imports anything beyond the installed wheel, numpy and the standard library, and none edits `sys.path`. |
| Mesh maps without a baker | `examples-fixture-provenance` | The committed fixture map set satisfies the mesh-map example with no bake provider attached. |
| Asset provenance | `examples-fixture-provenance` | Every fixture asset appears in the attribution file with its origin and licence. |
| Particle example | `examples-runner-policy` | Every example runs twice per comparison with a fixed seed and hash seed; differing output fails as non-deterministic. |
| Examples as a parity check | `examples-runner-policy` | The runner forwards its executor selection through `CTEX_EXECUTOR`, and a non-CPU run cannot restage the reference output. |
| Learning the API | `examples-readability` | Each example carries a header docstring naming its capabilities, stays within the readable line ceiling and imports no repository-local helper. |
| The full path | `examples-capability-coverage` | The end-to-end example declares the mesh, layer, graph, smart-material and export capabilities it carries. |
| Host-executed contract exercised headlessly | `examples-capability-coverage` | The host-executed example declares `execution-backends` and `host-transport` and runs with no device. |
| Example feeds the gate | `examples-feature-coverage-policy` | Example evidence is a recorded call trace, so a timing or a declaration cannot stand in for an exercised operation. |
| Reading the gallery | `examples-gallery` | Every numbered example appears in the published gallery with its output, description and capability. |

## Feature coverage

`examples-feature-coverage-policy` checks the decision rules; `just examples`
applies them to the recorded traces. A C ABI symbol counts as exercised only when
an example actually called it. Symbols no example reaches yet are named
individually in [`examples/feature_coverage.json`](../examples/feature_coverage.json)
with the task that closes them, so the uncovered surface is reported rather than
absent. Removing an entry that is now exercised is mandatory; the gate fails on a
stale deferral.
