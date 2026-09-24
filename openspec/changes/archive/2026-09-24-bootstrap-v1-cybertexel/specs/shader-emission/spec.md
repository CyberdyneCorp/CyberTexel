# shader-emission — Graph To Shader Source And A Pass Plan

## Purpose
Compile material graphs into portable shader artifacts and pass plans.

## ADDED Requirements

### Requirement: Emission produces shader artifacts, never a device object
Emission SHALL produce WGSL/MSL/HLSL source text or a SPIR-V binary module, as requested, and a description of the work, and SHALL NOT create, bind or reference any GPU device object. The emission module SHALL be usable in a build with no graphics backend compiled in.

#### Scenario: Headless emission
- **WHEN** the library is built with every GPU backend disabled
- **THEN** emission SHALL still compile a graph to the requested artifact for every supported target

### Requirement: Target languages
Emission SHALL target WGSL, MSL, SPIR-V and HLSL. WGSL SHALL be supported on every platform because it is the language `wgpu` hosts consume.

#### Scenario: A wgpu host
- **WHEN** a host requests WGSL for a material
- **THEN** valid WGSL SHALL be produced and SHALL compile under the host's shader compiler

#### Scenario: An unsupported target
- **WHEN** a target language this build does not support is requested
- **THEN** the call SHALL fail naming the requested and the available targets

### Requirement: Pass plan
Emission SHALL produce, alongside the source, an ordered pass plan naming for each pass: the entry points, the render targets with their formats and sizes, the texture and sampler bindings in declared order with their roles, the uniform block layout with field offsets, the vertex attribute layout, the draw call, and the depth and blend state.

#### Scenario: A host can execute without guessing
- **WHEN** a host receives a pass plan
- **THEN** every resource it must create and every binding it must set SHALL be named in the plan, with no ordering left implicit

### Requirement: Stable binding layout
For a given graph, target language and feature set, the binding indices in the pass plan SHALL be stable across emissions, so that a host may cache pipeline layouts.

#### Scenario: Re-emitting after a value change
- **WHEN** only a constant value changes in the graph
- **THEN** the binding layout SHALL be unchanged and the host's cached pipeline layout SHALL remain valid

### Requirement: Result variable naming and reuse
Each emitted intermediate result SHALL be named deterministically from the node's identity, qualified by every enclosing group, and SHALL be emitted once however many consumers read it.

#### Scenario: Fan-out
- **WHEN** one node feeds three inputs
- **THEN** its expression SHALL appear once in the emitted source and be referenced three times

#### Scenario: Name collision across groups
- **WHEN** two identically named nodes exist in different groups
- **THEN** their emitted variable names SHALL differ

### Requirement: Layer stack emission
The layer stack SHALL be emitted as compositing code rather than executed as a sequence of separate blend passes where the texture binding budget allows.

#### Scenario: Stack compiled into one pass
- **WHEN** a stack of eight layers fits the device's binding budget
- **THEN** it SHALL be composited in one pass

#### Scenario: Stack exceeds the binding budget
- **WHEN** the stack would need more bindings than the declared budget allows
- **THEN** emission SHALL split it into multiple passes, carry the intermediate result forward, and the pass plan SHALL show the split

### Requirement: Feature-gated emission
Emission SHALL take a declared device feature set — binding budget, supported texture formats, floating-point filtering, compute availability — and SHALL produce source valid for it, reporting any capability it had to work around.

#### Scenario: Device without float filtering
- **WHEN** the feature set reports no linear filtering of float textures
- **THEN** emission SHALL produce source that does not rely on it and SHALL report the workaround

### Requirement: Emission cache
Emission results SHALL be cached, keyed by the graph content, the target language and the feature set, and a cache hit SHALL return the identical source and pass plan.

#### Scenario: Unchanged graph
- **WHEN** emission is requested twice for an unchanged graph and feature set
- **THEN** the second call SHALL be served from cache and return identical output

#### Scenario: Cache invalidated by feature set
- **WHEN** the same graph is emitted for two different feature sets
- **THEN** two distinct cache entries SHALL be produced

### Requirement: Thread safety
Emission SHALL be safe to invoke concurrently from multiple threads. Any vendored compiler carrying global state SHALL be wrapped so that concurrent invocation is supported rather than serialized by the caller.

#### Scenario: Concurrent emission
- **WHEN** four threads emit four different graphs simultaneously
- **THEN** each SHALL receive correct output and the results SHALL match those from serial emission

### Requirement: Preview shading
Emission SHALL be able to produce a preview shader that composites the document's channels into a lit result, and single-channel inspection shaders for each channel.

#### Scenario: Inspecting roughness
- **WHEN** a roughness inspection shader is requested
- **THEN** it SHALL present the roughness channel without lighting

### Requirement: Declared lighting inputs
The preview shader SHALL declare its lighting inputs in the pass plan: an environment radiance map with its mip convention, a diffuse irradiance representation with its encoding, a specular BRDF lookup, an environment rotation and intensity, and any analytic lights with their parameters. The shading model — the BRDF, its parameterization and its energy conventions — SHALL be documented so a host can match it in its own passes.

#### Scenario: A host binds the environment
- **WHEN** a host receives the preview pass plan
- **THEN** every lighting resource it must supply SHALL be named with its expected format, encoding and binding index

#### Scenario: No environment supplied
- **WHEN** a host supplies no environment resources
- **THEN** the preview SHALL render with the documented fallback lighting rather than producing an undefined result

#### Scenario: Host matches the shading model
- **WHEN** a host renders the same material in its own pass
- **THEN** the documented BRDF and conventions SHALL be sufficient to match the preview within the parity tolerance

### Requirement: Emitted source is inspectable
Textual emitted source SHALL be retrievable by a host and by a test, and SHALL carry comments identifying which node produced which block. SPIR-V SHALL expose node attribution through companion debug metadata and SHALL NOT be represented as source text.

#### Scenario: Debugging a material
- **WHEN** a host retrieves the emitted WGSL
- **THEN** it SHALL be readable text with node attribution comments

### Requirement: Deterministic output
For a given graph, target language and feature set, the emitted source SHALL be byte-identical across runs and across platforms.

#### Scenario: Reproducible build
- **WHEN** the same graph is emitted on two machines
- **THEN** the emitted source SHALL be byte-identical

### Requirement: Third-party shader backend attribution
Where a third-party compiler is vendored to produce a target language, it SHALL be recorded in the attribution file with its licence text and revision, and SHALL be covered by the dependency audit.

#### Scenario: Audit covers the vendored compiler
- **WHEN** the licence audit runs
- **THEN** the vendored shader compiler SHALL appear with its own licence text and pinned revision

### Requirement: Resource lifetimes and synchronization in pass plans
Each pass plan SHALL declare logical resource IDs and generations, tile or mip subresource ranges, read/write access, initialization or load/store behavior, dependencies, and the render draw or compute dispatch dimensions. Hosts SHALL translate these declarations into their own API barriers and queue synchronization. A resource SHALL NOT be recycled until every submitted reader and writer has completed. Backend-specific device handles SHALL NOT appear in the plan.

#### Scenario: Undo resource remains in use
- **WHEN** a tile replaced by undo is still read by an in-flight preview
- **THEN** the plan and completion protocol SHALL retain that generation until the preview completes and SHALL prevent reuse of its storage in the meantime
