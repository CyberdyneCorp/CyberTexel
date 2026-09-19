_default:
    @just --list

# --- prerequisites -----------------------------------------------------------

_require tool version:
    #!/usr/bin/env sh
    if ! command -v {{tool}} >/dev/null 2>&1; then
        printf 'missing prerequisite: %s (need %s or newer)\n' {{tool}} {{version}} >&2
        printf 'see CONTRIBUTING.md for the full toolchain list.\n' >&2
        exit 1
    fi

# Report every prerequisite this repository needs.
prereqs:
    @just _require python3 3.10
    @just _require openspec 1.8
    @just _require cmake 3.24
    @just _require c++ C++20
    @just _require ninja 1.10
    @just _require clang-format 14
    @echo "ok: prerequisites present for the checks that run without a build"

# --- specification -----------------------------------------------------------

# Validate every spec and change against the OpenSpec schema.
spec-validate: (_require "openspec" "1.8")
    openspec validate --all --strict

# Proposal, spec directories and task plan must agree.
gate-capability-index: (_require "python3" "3.10")
    python3 tools/check_capability_index.py

# Every documented paint parameter is implemented and has explicit behaviour evidence.
gate-paint-parameter-audit: (_require "python3" "3.10")
    python3 tests/tools/test_check_paint_parameter_audit.py
    python3 tools/check_paint_parameter_audit.py

# Counts, for the README and for sanity.
spec-stats:
    @printf 'capabilities: %s\n' "$(ls openspec/changes/bootstrap-v1-cybertexel/specs | wc -l | tr -d ' ')"
    @printf 'requirements: %s\n' "$(cat openspec/changes/bootstrap-v1-cybertexel/specs/*/spec.md | grep -c '^### Requirement:')"
    @printf 'scenarios:    %s\n' "$(cat openspec/changes/bootstrap-v1-cybertexel/specs/*/spec.md | grep -c '^#### Scenario:')"
    @printf 'tasks:        %s\n' "$(grep -c '^- \[ \]' openspec/changes/bootstrap-v1-cybertexel/tasks.md)"

# --- build and test ----------------------------------------------------------

build: (_require "cmake" "3.24") (_require "c++" "C++20") (_require "ninja" "1.10")
    cmake --preset headless
    cmake --build --preset headless

build-vulkan: (_require "cmake" "3.24") (_require "c++" "C++20") (_require "ninja" "1.10")
    cmake --preset vulkan
    cmake --build --preset vulkan

test: build
    ctest --preset headless
    python3 -m unittest discover -s tests/tools -p 'test_*.py'

test-sanitize: (_require "cmake" "3.24") (_require "c++" "C++20") (_require "ninja" "1.10")
    cmake --preset headless-sanitize
    cmake --build --preset headless-sanitize
    ctest --preset headless-sanitize

# Bounded deterministic libFuzzer gate for the untrusted project-container reader.
fuzz-project-container: (_require "clang++" "14") (_require "cmake" "3.24") (_require "ninja" "1.10") (_require "python3" "3.10")
    CC=clang CXX=clang++ cmake -S . -B build/project-container-fuzz -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCTEX_ENABLE_SANITIZERS=ON -DCTEX_BUILD_FUZZERS=ON
    cmake --build build/project-container-fuzz --target ctex_project_container_fuzz
    python3 tools/run_project_container_fuzz.py build/project-container-fuzz/ctex_project_container_fuzz

test-vulkan: build-vulkan
    ./build/vulkan/ctex_vulkan_executor_test --require-device

test-image: build
    ctest --test-dir build/headless --output-on-failure -R '^tiled-image$'

test-color: build
    ctest --test-dir build/headless --output-on-failure -R '^color-management'

test-png: build
    ctest --test-dir build/headless --output-on-failure -R '^png-io$'

test-channels: build
    ctest --test-dir build/headless --output-on-failure -R '^document-channels$'

test-document: build
    ctest --test-dir build/headless --output-on-failure -R '^texture-document$'

test-host-transport-revisions: build
    ctest --test-dir build/headless --output-on-failure -R '^host-transport-revisions$'

test-host-transport-delta: build
    ctest --test-dir build/headless --output-on-failure -R '^host-transport-delta$'

test-host-transport-readback: build
    ctest --test-dir build/headless --output-on-failure -R '^host-transport-readback$'

test-host-transport-layout: build
    ctest --test-dir build/headless --output-on-failure -R '^host-transport-layout$'

test-host-transport-format: build
    ctest --test-dir build/headless --output-on-failure -R '^host-transport-format$'

test-host-transport-snapshot: build
    ctest --test-dir build/headless --output-on-failure -R '^host-transport-snapshot$'

test-host-transport-preview: build
    ctest --test-dir build/headless --output-on-failure -R '^host-transport-preview$'

test-host-transport-identity: build
    ctest --test-dir build/headless --output-on-failure -R '^host-transport-identity$'

test-host-transport-scenarios: build
    ctest --test-dir build/headless --output-on-failure -L '^host-transport-scenario$'

test-stroke-reconstruction: build
    ctest --test-dir build/headless --output-on-failure -R '^stroke-reconstruction$'

test-stroke-preset: build
    ctest --test-dir build/headless --output-on-failure -R '^stroke-preset$'

test-stroke-model-scenarios: build
    ctest --test-dir build/headless --output-on-failure -L '^stroke-model-scenario$'

test-paint-coverage: build
    ctest --test-dir build/headless --output-on-failure -R '^paint-coverage$'

test-paint-decal-stencil: build
    ctest --test-dir build/headless --output-on-failure -R '^paint-decal-stencil$'

test-paint-projection: build
    ctest --test-dir build/headless --output-on-failure -R '^paint-projection$'

test-paint-text: build
    ctest --test-dir build/headless --output-on-failure -R '^paint-text$'

test-paint-particle: build
    ctest --test-dir build/headless --output-on-failure -R '^paint-particle$'

test-paint-picker: build
    ctest --test-dir build/headless --output-on-failure -R '^paint-picker$'

test-paint-colour-id: build
    ctest --test-dir build/headless --output-on-failure -R '^paint-colour-id$'

test-paint-selection: build
    ctest --test-dir build/headless --output-on-failure -R '^paint-selection$'

test-paint-parameters: build
    ctest --test-dir build/headless --output-on-failure -R '^paint-parameters$'

test-paint-rejection: build
    ctest --test-dir build/headless --output-on-failure -R '^paint-rejection$'

test-paint-deposition: build
    ctest --test-dir build/headless --output-on-failure -R '^paint-deposition$'

test-paint-blending: build
    ctest --test-dir build/headless --output-on-failure -R '^paint-blending$'

test-paint-blur-smear: build
    ctest --test-dir build/headless --output-on-failure -R '^paint-blur-smear$'

test-paint-brush-eraser: build
    ctest --test-dir build/headless --output-on-failure -R '^paint-brush-eraser$'

test-paint-clone: build
    ctest --test-dir build/headless --output-on-failure -R '^paint-clone$'

test-paint-fill: build
    ctest --test-dir build/headless --output-on-failure -R '^paint-fill$'

test-paint-masking: build
    ctest --test-dir build/headless --output-on-failure -R '^paint-masking$'

test-paint-surface-cache: build
    ctest --test-dir build/headless --output-on-failure -R '^paint-surface-cache$'

test-paint-seam-dilation: build
    ctest --test-dir build/headless --output-on-failure -R '^paint-seam-dilation$'

test-paint-seam-filter: build
    ctest --test-dir build/headless --output-on-failure -R '^paint-seam-filter$'

test-paint-preview: build
    ctest --test-dir build/headless --output-on-failure -R '^paint-preview$'

test-paint-work: build
    ctest --test-dir build/headless --output-on-failure -R '^paint-work$'

test-paint-engine-scenarios: build
    ctest --test-dir build/headless --output-on-failure -L '^paint-engine-scenario$'

test-mesh: build
    ctest --test-dir build/headless --output-on-failure -R '^mesh-ingest$'

test-picking-index: build
    ctest --test-dir build/headless --output-on-failure -R '^picking-spatial-index$'

test-picking-rays: build
    ctest --test-dir build/headless --output-on-failure -R '^picking-ray-construction$'

test-picking-hit: build
    ctest --test-dir build/headless --output-on-failure -R '^picking-hit-record$'

test-picking-occlusion: build
    ctest --test-dir build/headless --output-on-failure -R '^picking-occlusion$'

test-picking-uv: build
    ctest --test-dir build/headless --output-on-failure -R '^picking-uv-space$'

test-picking-snap: build
    ctest --test-dir build/headless --output-on-failure -R '^picking-surface-snap$'

test-picking-regions: build
    ctest --test-dir build/headless --output-on-failure -R '^picking-region-queries$'

test-picking-boundaries: build
    ctest --test-dir build/headless --output-on-failure -R '^picking-boundary-determinism$'

test-picking-batch: build
    ctest --test-dir build/headless --output-on-failure -R '^picking-batch$'

test-picking-scenarios: build
    ctest --test-dir build/headless --output-on-failure -L '^picking-scenario$'

test-graph-document: build
    ctest --test-dir build/headless --output-on-failure -R '^material-graph-(document|output)$'

test-graph-cycles: build
    ctest --test-dir build/headless --output-on-failure -R '^material-graph-cycle-detection$'

test-graph-sockets: build
    ctest --test-dir build/headless --output-on-failure -R '^material-graph-socket-links$'

test-graph-catalogue: build
    ctest --test-dir build/headless --output-on-failure -R '^material-graph-catalogue$'

test-graph-groups: build
    ctest --test-dir build/headless --output-on-failure -R '^material-graph-groups$'

test-graph-validation: build
    ctest --test-dir build/headless --output-on-failure -R '^material-graph-validation$'

test-graph-host-nodes: build
    ctest --test-dir build/headless --output-on-failure -R '^material-graph-host-nodes$'

test-material-library: build
    ctest --test-dir build/headless --output-on-failure -R '^material-graph-library$'

test-portable-nodes: build
    ctest --test-dir build/headless --output-on-failure -R '^material-graph-portable-nodes$'

test-material-graph-scenarios: build
    ctest --test-dir build/headless --output-on-failure -L '^material-graph-scenario$'

test-kong-context: (_require "spirv-val" "SPIRV-Tools") build
    ctest --test-dir build/headless --output-on-failure -R '^kong-(context|spirv-)'

test-graph-emission: build
    ctest --test-dir build/headless --output-on-failure -R '^graph-emission$'

test-pass-plan: build
    ctest --test-dir build/headless --output-on-failure -R '^pass-plan$'

test-feature-emission: (_require "spirv-val" "SPIRV-Tools") build
    ctest --test-dir build/headless --output-on-failure -R '^feature-emission'

test-emission-cache: build
    ctest --test-dir build/headless --output-on-failure -R '^emission-cache$'

test-concurrent-emission: build
    ctest --test-dir build/headless --output-on-failure -R '^concurrent-emission$'

test-preview-emission: (_require "spirv-val" "SPIRV-Tools") build
    ctest --test-dir build/headless --output-on-failure -R '^preview-emission'

test-material-emission: (_require "spirv-val" "SPIRV-Tools") build
    ctest --test-dir build/headless --output-on-failure -R '^material-emission'

test-executor-registry: build
    ctest --test-dir build/headless --output-on-failure -R '^executor-(registry|environment-pin|cpu-reference|bounded-execution|vulkan|host-execution|emission-features|parity-tolerances)$'

test-execution-backend-scenarios: build
    ctest --test-dir build/headless --output-on-failure -L '^execution-backends-scenario$'

test-mesh-map-scenarios: build
    ctest --test-dir build/headless --output-on-failure -L '^mesh-maps-scenario$'

test-project-io-scenarios: build
    ctest --test-dir build/headless --output-on-failure -L '^project-io-scenario$'

test-export-presets: build
    ctest --test-dir build/headless --output-on-failure -R '^export-presets$'

test-texture-export-formats: build
    ctest --test-dir build/headless --output-on-failure -R '^texture-export-formats$'

test-texture-export-plan: build
    ctest --test-dir build/headless --output-on-failure -R '^texture-export-plan$'

test-texture-export-execution: build
    ctest --test-dir build/headless --output-on-failure -R '^texture-export-(execution|report-|determinism)'

test-shader-emission-scenarios: (_require "spirv-val" "SPIRV-Tools") build
    ctest --test-dir build/headless --output-on-failure -L '^shader-emission-scenario$'

examples:
    @just _unimplemented examples 16.2

bench:
    @just _unimplemented bench 17.3

format: (_require "clang-format" "14")
    find src include tests examples -type f \( -name '*.c' -o -name '*.h' -o -name '*.cpp' -o -name '*.hpp' \) -print0 | xargs -0 clang-format -i

format-check: (_require "clang-format" "14")
    find src include tests examples -type f \( -name '*.c' -o -name '*.h' -o -name '*.cpp' -o -name '*.hpp' \) -print0 | xargs -0 clang-format --dry-run --Werror

clean:
    rm -rf build out dist target

# --- gates -------------------------------------------------------------------
#
# Named by openspec/changes/bootstrap-v1-cybertexel/tasks.md and required by
# build-packaging. A gate whose implementing task is not done exits non-zero
# and names that task: a gate that passes before it exists is exactly the
# failure device-gate is written to prevent.

_unimplemented name task:
    @printf 'gate "%s" is not implemented yet.\n' {{name}} >&2
    @printf 'It is delivered by task %s in openspec/changes/bootstrap-v1-cybertexel/tasks.md.\n' {{task}} >&2
    @exit 1

# Module dependency rule: no cycles, nothing depends on exec, no backend leaks.
gate-layering:
    python3 tests/tools/test_check_layering.py
    python3 tools/check_layering.py

# Permissive-only dependencies, attribution complete, vendored trees audited.
gate-licence:
    python3 tests/tools/test_check_licenses.py
    python3 tools/check_licenses.py

# One version, consumed by build, ABI query and every binding manifest.
gate-version-consistency: build
    python3 tests/tools/test_check_version.py
    python3 tools/check_version.py
    ctest --test-dir build/headless --output-on-failure -R '^version-consistency$'

# Emitted shaders, saved documents and exported textures are byte-identical.
gate-determinism:
    python3 tools/check_determinism.py

# Every C entry point reachable from Python, Swift and Rust.
gate-binding-parity:
    @just _unimplemented gate-binding-parity 14.13

# No symbol or descriptor removed without a major version bump.
gate-abi-diff:
    @just _unimplemented gate-abi-diff 14.4

# Every capability has a numbered example that runs and asserts.
gate-example-coverage:
    @just _unimplemented gate-example-coverage 16.5

# Every available executor agrees with the CPU reference within the declared
# tolerance; compiled routes without a device are reported as unmeasured.
gate-parity:
    just build
    ./build/headless/ctex_executor_parity_gate

# Budgeted operations measured on a named reference device.
# Device-dependent: run where the hardware exists.
gate-budgets:
    @just _unimplemented gate-budgets 17.6

# --- aggregate ---------------------------------------------------------------

# Everything that needs no device. This is what CI runs and what a contributor
# runs before pushing.
check: spec-validate gate-capability-index gate-paint-parameter-audit gate-layering gate-licence \
       gate-version-consistency gate-determinism gate-binding-parity \
       gate-example-coverage gate-abi-diff format-check

# Everything that can be checked before there is any code.
check-spec: spec-validate gate-capability-index
