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
    @printf 'capabilities: %s\n' "$(ls openspec/specs | wc -l | tr -d ' ')"
    @printf 'requirements: %s\n' "$(cat openspec/specs/*/spec.md | grep -c '^### Requirement:')"
    @printf 'scenarios:    %s\n' "$(cat openspec/specs/*/spec.md | grep -c '^#### Scenario:')"
    @printf 'completed tasks: %s\n' "$(grep -c '^- \[x\]' openspec/changes/archive/2026-09-24-bootstrap-v1-cybertexel/tasks.md)"

# --- build and test ----------------------------------------------------------

build: (_require "cmake" "3.24") (_require "c++" "C++20") (_require "ninja" "1.10")
    cmake --preset headless
    cmake --build --preset headless

build-vulkan: (_require "cmake" "3.24") (_require "c++" "C++20") (_require "ninja" "1.10")
    cmake --preset vulkan
    cmake --build --preset vulkan

# Build, inspect, smoke-test and archive the current desktop platform package.
package-native: (_require "python3" "3.10") (_require "cmake" "3.24") (_require "ninja" "1.10")
    #!/usr/bin/env sh
    set -eu
    if [ "${OS:-}" = "Windows_NT" ]; then preset=windows-x64; \
    elif [ "$(uname -s)" = "Darwin" ]; then preset=macos-universal; \
    else preset=linux-x64; fi
    python3 tools/build_platform_package.py "$preset"

# On Linux this builds directly; elsewhere it builds in the same container CI
# uses, so the Linux package is exercised without waiting for a CI run.
package-linux:
    #!/usr/bin/env bash
    set -euo pipefail
    if [[ "$(uname -s)" == "Linux" ]]; then
        just _require python3 3.10
        just _require cmake 3.24
        just _require ninja 1.10
        python3 tools/build_platform_package.py linux-x64
    else
        if ! command -v docker >/dev/null 2>&1; then
            printf 'missing prerequisite: docker (needed to build the Linux package off Linux)\n' >&2
            exit 1
        fi
        mkdir -p dist build/packages
        docker build --tag cybertexel-package-toolchain \
            --file tools/docker/package.Dockerfile tools/docker
        docker run --rm \
            --mount "type=bind,source=$PWD,target=/src" \
            cybertexel-package-toolchain \
            bash -lc 'cd /src && python3 tools/build_platform_package.py linux-x64'
    fi

package-macos: (_require "python3" "3.10") (_require "cmake" "3.24") (_require "ninja" "1.10")
    python3 tools/build_platform_package.py macos-universal

package-windows: (_require "python3" "3.10") (_require "cmake" "3.24") (_require "ninja" "1.10")
    python3 tools/build_platform_package.py windows-x64

package-ios: (_require "python3" "3.10") (_require "cmake" "3.24") (_require "ninja" "1.10")
    python3 tools/build_platform_package.py ios-arm64

package-android: (_require "python3" "3.10") (_require "cmake" "3.24") (_require "ninja" "1.10")
    python3 tools/build_platform_package.py android-arm64

test: build
    ctest --preset headless
    python3 -m unittest discover -s tests/tools -p 'test_*.py'

test-sanitize: (_require "cmake" "3.24") (_require "c++" "C++20") (_require "ninja" "1.10")
    cmake --preset headless-sanitize
    cmake --build --preset headless-sanitize
    CTEX_DETERMINISM_BINARY_DIR=build/headless-sanitize CTEX_SANITIZER_ASAN_LIBRARY="$(c++ -print-file-name=libasan.so)" ctest --preset headless-sanitize

_fuzz target runner build_dir:
    #!/usr/bin/env bash
    set -euo pipefail
    if [[ "$(uname -s)" == "Darwin" ]]; then
        if ! command -v docker >/dev/null 2>&1; then
            printf 'missing prerequisite: docker (needed for libFuzzer on macOS)\n' >&2
            exit 1
        fi
        mkdir -p build/fuzz-artifacts
        docker build --tag cybertexel-fuzz-toolchain --file tools/docker/fuzz.Dockerfile tools/docker
        docker run --rm \
            --mount "type=bind,source=$PWD,target=/src,readonly" \
            --mount "type=bind,source=$PWD/build/fuzz-artifacts,target=/artifacts" \
            --mount "type=volume,source=cybertexel-fuzz-build,target=/work" \
            cybertexel-fuzz-toolchain \
            bash -lc 'cmake -S /src -B /work/build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCTEX_ENABLE_SANITIZERS=ON -DCTEX_BUILD_FUZZERS=ON && cmake --build /work/build --target {{target}} && cd /src && python3 {{runner}} /work/build/{{target}} --artifact-dir /artifacts/{{target}}'
    else
        for prerequisite in clang clang++ cmake ninja python3; do
            if ! command -v "$prerequisite" >/dev/null 2>&1; then
                printf 'missing prerequisite: %s\n' "$prerequisite" >&2
                exit 1
            fi
        done
        cmake -S . -B {{build_dir}} -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCTEX_ENABLE_SANITIZERS=ON -DCTEX_BUILD_FUZZERS=ON
        cmake --build {{build_dir}} --target {{target}}
        python3 {{runner}} {{build_dir}}/{{target}}
    fi

# Bounded deterministic libFuzzer gate for the untrusted project-container reader.
fuzz-project-container:
    just _fuzz ctex_project_container_fuzz tools/run_project_container_fuzz.py build/project-container-fuzz

# Bounded deterministic libFuzzer gate for every in-memory image decoder.
fuzz-image-decoders:
    just _fuzz ctex_image_decode_fuzz tools/run_image_decode_fuzz.py build/image-decode-fuzz

test-vulkan: build-vulkan
    ./build/vulkan/ctex_vulkan_executor_test --require-device

# Focused cross-platform build and process smoke suite for the installed CLI surface.
test-cli: (_require "cmake" "3.24") (_require "python3" "3.10") (_require "ninja" "1.10")
    cmake --preset headless
    cmake --build --preset headless --target cybertexel_cli ctex_texture_document_io_test cybertexel_c ctex_cli_bake_provider ctex_cli_obj_mesh_test
    ctest --test-dir build/headless --output-on-failure -R '^cli-headless-'

test-image: build
    ctest --test-dir build/headless --output-on-failure -R '^tiled-image$'

test-color: build
    ctest --test-dir build/headless --output-on-failure -R '^color-management'

test-png: build
    ctest --test-dir build/headless --output-on-failure -R '^png-io$'

test-image-io-scenarios: build
    ctest --test-dir build/headless --output-on-failure -L '^image-io-scenario$'

test-channels: build
    ctest --test-dir build/headless --output-on-failure -R '^document-channels$'

test-document: build
    ctest --test-dir build/headless --output-on-failure -R '^texture-document$'

test-udim: build
    ctest --test-dir build/headless --output-on-failure -R '^texture-set-udim$'

test-atlas: build
    ctest --test-dir build/headless --output-on-failure -R '^texture-set-atlas$'

test-texture-document-scenarios: build
    ctest --test-dir build/headless --output-on-failure -L '^texture-document-scenario$'

test-smart-material-serialization: build
    ctest --test-dir build/headless --output-on-failure -R '^smart-material-serialization$'

test-smart-mask-instances: build
    ctest --test-dir build/headless --output-on-failure -R '^smart-mask-instances$'

test-smart-material-resources: build
    ctest --test-dir build/headless --output-on-failure -R '^smart-material-resources$'

test-smart-material-scenarios: build
    ctest --test-dir build/headless --output-on-failure -L '^smart-materials-scenario$'

test-c-api: build
    ctest --test-dir build/headless --output-on-failure -R '^c-abi-(foundation|caller-buffers|versioned-descriptors|two-document-concurrency|host-logging|host-allocator|boundary-contract|export-surface|compatibility)$'

test-c-abi-scenarios: build
    ctest --test-dir build/headless --output-on-failure -L '^c-abi-scenario$'

test-python-binding: (_require "uv" "Python wheel builder") (_require "cmake" "3.24")
    uv run --no-project --python 3.12 -- python tools/test_python_wheel.py

test-swift-binding: (_require "swift" "Swift 5.9") (_require "cmake" "3.24")
    python3 tools/test_swift_package.py

test-rust-binding: (_require "cargo" "Rust stable") (_require "cmake" "3.24")
    cargo fmt --all --manifest-path rust/Cargo.toml -- --check
    cargo clippy --manifest-path rust/Cargo.toml --workspace --all-targets -- -D warnings
    cargo test --manifest-path rust/Cargo.toml --workspace
    cargo run --manifest-path rust/Cargo.toml -p cybertexel-sys --example workflow
    python3 tools/check_rust_unsafe.py

# Full cross-language scenario gate. Swift makes this aggregate macOS-only;
# platform CI runs each constituent recipe in its native job.
test-language-binding-scenarios: test-python-binding test-swift-binding test-rust-binding examples gate-binding-parity build
    ctest --test-dir build/headless --output-on-failure -L '^language-bindings-scenario$'

generate-rust-sys: (_require "bindgen" "bindgen-cli 0.72.1")
    python3 tools/generate_rust_sys.py

generate-python-capi: (_require "uv" "Python tool runner")
    python3 tools/generate_python_capi.py

test-preset-shelf-library: build
    ctest --test-dir build/headless --output-on-failure -R '^preset-shelf-library$'

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

test-resource-residency-scenarios: build
    ctest --test-dir build/headless --output-on-failure -L '^resource-residency-scenario$'

# `examples` evidence that needs no wheel. `just examples` runs the examples.
test-examples-scenarios: build
    ctest --test-dir build/headless --output-on-failure -L '^examples-scenario$'

# `build-packaging` policy evidence: the rules the gate recipes apply.
test-build-packaging-scenarios: build
    ctest --test-dir build/headless --output-on-failure -L '^build-packaging-scenario$'

# Desktop WGSL reference host: runs the emitted pass plan on a real wgpu device.
# Exit code 3 means no adapter was available, which is unmeasured, not a pass.
host-desktop: host-desktop-build
    CTEX_RUN_DATE="$(date -u +%Y-%m-%d)" CTEX_RUN_COMMIT="$(git rev-parse HEAD)" \
    cargo run --manifest-path hosts/desktop-wgpu/Cargo.toml --release -- \
        --report build/reference-hosts/desktop-wgsl.json \
        --measurements benchmarks/results/host-desktop-wgsl.json

# Input-to-visible latency on the desktop reference host.
#
# This needs a VISIBLE window: a hidden or occluded window is throttled by the
# compositor, and the host refuses to report throttled frames as an interaction
# measurement. Run it from an interactive session, not from a detached shell.
host-desktop-benchmark frames="600": host-desktop-build
    CTEX_RUN_DATE="$(date -u +%Y-%m-%d)" CTEX_RUN_COMMIT="$(git rev-parse HEAD)" \
    cargo run --manifest-path hosts/desktop-wgpu/Cargo.toml --release -- \
        --benchmark --frames {{frames}} \
        --report build/reference-hosts/desktop-wgsl-benchmark.json \
        --measurements benchmarks/results/host-desktop-wgsl-latency.json

# Build and lint the desktop reference host without requiring a device.
host-desktop-build: (_require "cargo" "Rust stable")
    cargo fmt --manifest-path hosts/desktop-wgpu/Cargo.toml -- --check
    cargo clippy --manifest-path hosts/desktop-wgpu/Cargo.toml --all-targets -- -D warnings
    cargo build --manifest-path hosts/desktop-wgpu/Cargo.toml --release

# Mobile MSL reference host: runs on macOS Metal and cross-builds for iPad.
# Exit code 3 means no Metal device was available, which is unmeasured.
host-ipad: (_require "swift" "Swift 5.9") (_require "cmake" "3.24")
    python3 tools/run_metal_reference_host.py

# Both reference hosts. Task 18.2.
hosts: host-desktop host-ipad

# Run the measured release gate on the named Mac with its connected iPad.
# The desktop benchmark needs the runner's visible Aqua login session.
gate-reference-devices: (_require "python3" "3.10") (_require "cargo" "Rust stable") (_require "xcodebuild" "Xcode 27")
    python3 tools/run_reference_device_gate.py all

gate-reference-desktop: (_require "python3" "3.10") (_require "cargo" "Rust stable")
    python3 tools/run_reference_device_gate.py desktop

gate-reference-ipad: (_require "python3" "3.10") (_require "xcodebuild" "Xcode 27")
    python3 tools/run_reference_device_gate.py ipad

# Paint a model you own and render the result. Not part of `just check`: it
# needs Blender and an asset the repository does not carry. See demos/README.md.
demo-paint model texture="" extent="1024": (_require "uv" "Python wheel builder")
    #!/usr/bin/env bash
    set -euo pipefail
    blender=/Applications/Blender.app/Contents/MacOS/Blender
    if [[ ! -x "$blender" ]]; then blender="$(command -v blender || true)"; fi
    if [[ -z "$blender" ]]; then
        printf 'missing prerequisite: blender (needed to read the model and render it)\n' >&2
        exit 1
    fi
    just test-python-binding
    mkdir -p demos/output
    "$blender" --background --factory-startup --python demos/extract_mesh_blender.py -- \
        "{{model}}" demos/output/mesh.npz
    wheel="$(ls build/python-dist/cybertexel-*.whl)"
    if [[ -n "{{texture}}" ]]; then base=(--base-colour "{{texture}}"); else base=(); fi
    uv run --no-project --with "$wheel" --with numpy -- python demos/paint_fbx_model.py \
        --mesh demos/output/mesh.npz --output demos/output --extent {{extent}} "${base[@]}"
    "$blender" --background --factory-startup --python demos/render_blender.py -- \
        "{{model}}" demos/output/painted_base_color.png demos/output/render_painted.png
    printf '\nresults:\n  demos/output/painted_base_color.png\n  demos/output/render_painted.png\n  demos/output/summary.json\n'

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

test-paint-tools-scenarios: build
    ctest --test-dir build/headless --output-on-failure -L '^paint-tools-scenario$'

test-mesh-and-texture-set-scenarios: build
    ctest --test-dir build/headless --output-on-failure -L '^mesh-and-texture-sets-scenario$'

test-editable-authoring-scenarios: build
    ctest --test-dir build/headless --output-on-failure -L '^editable-authoring-scenario$'

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

test-texture-export-scenarios: build
    ctest --test-dir build/headless --output-on-failure -L '^texture-export-scenario$'

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

examples: test-python-binding
    python3 tools/check_example_fixtures.py
    python3 tools/check_example_readability.py
    rm -rf build/example-traces
    CTEX_SYMBOL_TRACE="$PWD/build/example-traces" python3 tools/run_python_examples.py compare
    python3 tests/tools/test_check_example_features.py
    python3 tools/check_example_features.py
    python3 tools/check_example_gallery.py

# Measure the library-side budgets on a named reference device. Figures from
# any other machine are informational, so the device id is required.
bench device="" : (_require "uv" "Python wheel builder") (_require "python3" "3.10")
    #!/usr/bin/env sh
    set -eu
    if [ -n "{{device}}" ]; then device="{{device}}"; \
    else device=""; fi
    if [ -z "$device" ]; then
        printf 'name the reference device: just bench <device-id>\n' >&2
        printf 'ids come from benchmarks/device_gate.json\n' >&2
        exit 2
    fi
    just test-python-binding
    python3 tools/run_library_bench.py --device "$device"

format: (_require "clang-format" "14")
    find src include tests examples -type f \( -name '*.c' -o -name '*.h' -o -name '*.cpp' -o -name '*.hpp' \) -print0 | xargs -0 clang-format -i

format-check: (_require "clang-format" "14")
    find src include tests examples -type f \( -name '*.c' -o -name '*.h' -o -name '*.cpp' -o -name '*.hpp' \) -print0 | xargs -0 clang-format --dry-run --Werror

clean:
    rm -rf build out dist target

# --- gates -------------------------------------------------------------------
#
# Named by openspec/changes/archive/2026-09-24-bootstrap-v1-cybertexel/tasks.md and required by
# build-packaging. A gate whose implementing task is not done exits non-zero
# and names that task: a gate that passes before it exists is exactly the
# failure device-gate is written to prevent.

_unimplemented name task:
    @printf 'gate "%s" is not implemented yet.\n' {{name}} >&2
    @printf 'It is delivered by task %s in openspec/changes/archive/2026-09-24-bootstrap-v1-cybertexel/tasks.md.\n' {{task}} >&2
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
    python3 tests/tools/test_check_binding_parity.py
    python3 tools/check_binding_parity.py

# No symbol or descriptor removed without a major version bump.
gate-abi-diff: build
    python3 tests/tools/test_check_abi.py
    python3 tools/check_abi.py

# Every runtime capability must name the public C operations that expose it.
gate-c-api-coverage:
    python3 tests/tools/test_check_capi_coverage.py
    python3 tools/check_capi_coverage.py

# Every capability has a numbered example that runs and asserts.
gate-example-coverage:
    python3 tests/tools/test_check_example_coverage.py
    python3 tools/check_example_coverage.py

# Both reference hosts account for every structural field the emitted pass
# plan carries, and CI invokes them. Task 18.2.
gate-reference-hosts: (_require "python3" "3.10")
    python3 tools/check_reference_hosts.py
    python3 tests/tools/test_reference_device_gate.py

# The justfile is the single definition of every routine command, every named
# gate is reachable, and CI invokes recipes rather than repeating them.
gate-task-runner: (_require "python3" "3.10")
    python3 tools/check_task_runner.py

# Platform packages this release slice claims, each smoke-tested by a program
# that links it. Deferred platforms are reported by name with their decision.
gate-packages preset="": (_require "python3" "3.10")
    python3 tests/tools/test_check_packages.py
    @if [ -n "{{preset}}" ]; then python3 tools/check_packages.py --preset "{{preset}}"; else python3 tools/check_packages.py; fi

# Validate the three ZIPs selected for the initial GitHub release.
gate-release-assets directory="dist": (_require "python3" "3.10")
    python3 tests/tools/test_verify_release_assets.py
    python3 tools/verify_release_assets.py "{{directory}}"

# The same commit built twice produces identical libraries, or the differing
# input is named in docs/reproducible-builds.md.
gate-reproducible: (_require "python3" "3.10") (_require "cmake" "3.24") (_require "ninja" "1.10")
    python3 tools/check_reproducible_build.py

# Every available executor agrees with the CPU reference within the declared
# tolerance; compiled routes without a device are reported as unmeasured.
gate-parity:
    just build
    ./build/headless/ctex_executor_parity_gate

# Validate the budget policy everywhere, then decide measurements only on an
# exact named reference device. Set CTEX_DEVICE_GATE_RESULTS to its result JSON.
gate-budgets: (_require "python3" "3.10")
    python3 tests/tools/test_device_gate.py
    python3 tools/device_gate.py validate
    python3 tools/device_gate.py document
    @if [ -z "${CTEX_DEVICE_GATE_RESULTS:-}" ]; then printf 'missing prerequisite: CTEX_DEVICE_GATE_RESULTS (dated reference-device result JSON)\n' >&2; exit 2; fi
    @baselines="${CTEX_DEVICE_GATE_BASELINES:-benchmarks/baselines.json}"; \
     if [ -f "$baselines" ]; then python3 tools/device_gate.py gate --results "$CTEX_DEVICE_GATE_RESULTS" --baselines "$baselines"; else python3 tools/device_gate.py gate --results "$CTEX_DEVICE_GATE_RESULTS"; fi

# --- aggregate ---------------------------------------------------------------

# Everything that needs no device. This is what CI runs and what a contributor
# runs before pushing.
check: spec-validate gate-capability-index gate-paint-parameter-audit gate-layering gate-licence \
       gate-version-consistency gate-determinism gate-binding-parity \
       gate-example-coverage gate-abi-diff gate-task-runner gate-reference-hosts format-check

# Everything that can be checked before there is any code.
check-spec: spec-validate gate-capability-index
