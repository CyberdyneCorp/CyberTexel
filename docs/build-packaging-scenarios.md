# Build and packaging scenario coverage

`just test-build-packaging-scenarios` runs every CTest carrying the
`build-packaging-scenario` label. Those tests decide the rules; the gates that
apply them are `just` recipes, and CI invokes the same recipes.

| OpenSpec scenario | Executable evidence | What is asserted |
| --- | --- | --- |
| Preset build | `build-packaging-package-builder` | Every shipped preset configures, builds and installs with the project's own warnings as errors. |
| Minimal configuration | `build-packaging-layering-policy` | No module below the C ABI reaches a graphics backend, so the core configures and tests with no GPU SDK. |
| A contributor builds from a clean clone | `build-packaging-task-runner` | `build`, `test`, `format`, `examples`, `bench`, `clean` and `check` exist as recipes, so no underlying invocation has to be known. |
| Every gate is reachable by name | `build-packaging-task-runner` | Each gate the specification names appears as a recipe whose name identifies it. |
| Gate invoked before it is built | `build-packaging-task-runner` | A recipe that defers to `_unimplemented` must pass the task identifier that delivers it. |
| A vacuous pass is impossible | `build-packaging-task-runner` | An unimplemented gate exits non-zero, so the aggregate `check` cannot report success for a gate that checked nothing. |
| A gate's command changes | `build-packaging-task-runner` | A CI step invokes a recipe, so a gate's implementation changes without editing a workflow. |
| Drift is detectable | `build-packaging-task-runner` | A workflow step that is not a recipe and not toolchain installation fails, naming the workflow and command. |
| Missing toolchain | `build-packaging-task-runner` | `_require` reports the missing tool and the version needed instead of the tool's own error. |
| Cycle introduced | `build-packaging-layering-policy` | A dependency cycle between modules fails the layering gate naming both modules. |
| Backend leak | `build-packaging-layering-policy` | A module outside `exec` including a graphics API header fails the gate naming the file. |
| Licence gate | `build-packaging-licence-policy` | A non-permissive dependency fails the audit naming the dependency. |
| Vendored tree | `build-packaging-licence-policy` | A vendored third-party tree must appear in the attribution file with its own licence text and pinned revision. |
| Release gate | `build-packaging-package-scope`, `build-packaging-task-runner` | A tier that could not run is reported by name; a deferred platform records its decision and never counts as passed. |
| Memory error | `build-packaging-task-runner` | The sanitizer and both fuzz recipes exist and are invoked by a CI workflow, so a memory error fails a sanitized run. |
| Non-deterministic output | `build-packaging-determinism-policy` | Emitted shaders, saved documents and exported textures must be byte-identical across two runs, and an empty category fails. |
| Version drift | `build-packaging-version-policy` | A package declaring a version other than `VERSION` fails naming the package. |
| Removed symbol | `build-packaging-abi-policy` | Removing an exported symbol without a major bump fails the diff naming the symbol. |
| Unexercised package | `build-packaging-package-scope` | An in-scope package without a recorded smoke-test outcome, or without its archive, fails rather than being published. |
| Host-executed route stays honest | `build-packaging-reference-hosts` | Both reference hosts parse the plan strictly and must name every structural field it carries; CI builds and runs them, so a plan change fails until they are updated. |
| Unreachable budget | `device-gate-policy` | A declared floor that no configuration reaches is reported as unreachable by the budget gate, never recorded as passed. |
| Repeat build | `build-packaging-task-runner` | `just gate-reproducible` builds the shipped preset twice at one path and fails on any artifact difference that `docs/reproducible-builds.md` does not name. |
| Verified release artifacts | `build-packaging-package-scope` | The `verify-first-release-assets` CI job validated all three archives from the tagged commit; the published assets were downloaded and passed the same archive and manifest gate with matching SHA-256 digests. |
| CI or device gate fails | `build-packaging-task-runner` | Publication waited for both in-scope CI and the named Mac/iPad device workflow to pass on the tagged commit; failed device attempts were not treated as passes. |
| Deferred platforms | `build-packaging-package-scope` | The release manifest records Windows and Android as deferred, and the published release has exactly the three in-scope assets. |

## Platform scope

[`release/platforms.json`](../release/platforms.json) declares the platforms this
release slice decides — `linux-x64`, `macos-universal` and `ios-arm64` — and the
platforms deliberately deferred with the decision that deferred them.
The first release gates macOS, Linux and iOS. The active
`windows-android-release` OpenSpec change carries `windows-x64` and
`android-arm64`; the platform manifest reports them as deferred by name.

## Reference hosts

[`hosts/`](../hosts) carries the desktop WGSL host over `wgpu` and the mobile
MSL host over Metal. Both execute an emitted pass plan on a real device API,
report completion so the library publishes the revision, and drive the explicit
transport path. `just gate-reference-hosts` fails when either host stops
accounting for a field the emitted plan carries; CI builds both on macOS and
Linux and runs them where an adapter exists. A runner without one reports the
run as unmeasured, which is never a pass.
