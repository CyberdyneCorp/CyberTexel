# Headless CLI scenario coverage

This matrix maps every `cli-headless` OpenSpec scenario to executable evidence.
The `cli-headless-scenario` CTest label is the capability gate; `just test-cli`
builds all of its native fixtures and runs the same tests on the first-release
Linux and macOS platforms in CI. Windows is tracked separately.

| OpenSpec scenario | Executable evidence | What is asserted |
| --- | --- | --- |
| Running on a build machine | `cli-headless-skeleton` | All six commands run without a GPU and report the CPU reference executor or a documented fallback. |
| Re-exporting after a mesh update | `cli-headless-skeleton`, `cli-headless-obj-mesh` | A replacement OBJ is parsed and each keep, clear and reproject reconciliation policy exports both texture sets. |
| Unknown preset | `cli-headless-skeleton` | The invalid-argument result names the unknown preset, lists available presets and leaves no output. |
| Missing mesh map | `cli-headless-skeleton` | A provider refusal names the ambient-occlusion map, returns the missing-resource code and leaves no project. |
| Pipeline consumption | `cli-headless-skeleton` | JSON reports enumerate resolved inputs, operations, output metadata, executor, fallback, clamping and timing fields. |
| Piping a report | `cli-headless-skeleton` | JSON remains alone on stdout while script and process diagnostics are routed to stderr. |
| Repeat export | `cli-headless-skeleton` | Separate repeated exports have identical paths relative to their roots and byte-identical file contents. |
| Forcing the reference executor | `cli-headless-skeleton` | `--executor cpu` overrides the environment and the report names CPU with no fallback. |
| Over budget on a farm node | `cli-headless-skeleton` | Memory and texel preflights return the budget exit code before creating output. |
| Interrupted export | `cli-headless-skeleton` | A twenty-texture export is interrupted in flight, returns the cancellation code and leaves neither a destination nor staging output. |
| A pipeline-specific operation | `cli-headless-skeleton` | `run` executes the installed Python binding, modifies the document and publishes the result atomically. |
| Flag added without documentation | `cli-headless-skeleton` | The help audit compares every implemented global and command option with command help and its default. |
| Release gate | `just test-cli` | The CI desktop OS matrix invokes the complete CLI recipe on Linux and macOS. |
