# Shader emission scenario coverage

`just test-shader-emission-scenarios` runs every CTest carrying the
`shader-emission-scenario` label, including external SPIR-V validation, the
focused shader determinism registry, and the vendored dependency audit.

| OpenSpec scenario | CTest | Covered behavior |
|---|---|---|
| Headless emission | `material-emission` | A graph produces WGSL, MSL, SPIR-V and HLSL artifacts without a graphics backend |
| A wgpu host | `material-emission`, `kong-context` | Complete WGSL stages and stable entry points are produced through the headless compiler path |
| An unsupported target | `material-emission`, `kong-context` | Diagnostic names the unknown request and all four available targets |
| A host can execute without guessing | `material-emission`, `pass-plan` | Output, resource accesses, bindings, vertex layout, state and draw are explicit |
| Re-emitting after a value change | `material-emission` | Shader value changes while the complete pass plan remains equal |
| Fan-out | `graph-emission` | One declaration feeds three references |
| Name collision across groups | `graph-emission` | Complete group paths distinguish identical internal node IDs |
| Stack compiled into one pass | `feature-emission` | Eight layers remain in one pass under the declared budget |
| Stack exceeds the binding budget | `feature-emission` | Greedy split carries explicit intermediate generations and dependencies |
| Device without float filtering | `feature-emission`, `material-emission` | Nearest sampling is emitted and reported by code |
| Unchanged graph | `emission-cache`, `material-emission` | Second request returns the same immutable cached result |
| Cache invalidated by feature set | `emission-cache`, `material-emission` | Feature and target changes produce distinct entries |
| Concurrent emission | `concurrent-emission` | Four graphs and four target stacks match serial baselines |
| Inspecting roughness | `preview-emission` | Roughness inspection is unlit and isolated from lit preview caching |
| A host binds the environment | `preview-emission` | Every environment texture, encoding, mip rule, uniform and binding is declared |
| No environment supplied | `preview-emission` | All targets compile the documented deterministic fallback |
| Host matches the shading model | `preview-emission` | Generated GGX terms and pass inputs match the documented parameter contract |
| Debugging a material | `graph-emission`, `material-emission` | Readable WGSL retains producing-node and complete group-path comments |
| Inspecting emission through C | `c-abi-shader-debug` | Stable reused variables and structured node attribution cross the boundary for text and SPIR-V |
| Name collision across workspace groups through C | `c-abi-shader-debug` | Qualified group paths distinguish colliding internal node IDs in public output |
| Reproducible build | `shader-emission-determinism` | Graph, complete material, Kong, layer-stack and preview bytes match across clean runs |
| Audit covers the vendored compiler | `shader-emission-licence-audit`, `c-abi-shader-debug` | Kong licence text, source discovery and pinned revisions are checked and exposed at runtime |
| Undo resource remains in use | `pass-plan` | Logical generations and derived reader/writer lifetimes survive through the final dependent pass |

The last scenario's execution-side completion record and physical-allocation
retirement rule belongs to task 7.3. Likewise, task 7.5 adds executed image
parity for the documented preview BRDF, and task 18.2 runs WGSL in the reference
wgpu host. The emission-side declarations and fixtures are already labeled here
so those later tests extend the same scenario rather than inventing a parallel
contract.
