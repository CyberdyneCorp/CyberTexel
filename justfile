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
    @echo "ok: prerequisites present for the checks that run without a build"

# --- specification -----------------------------------------------------------

# Validate every spec and change against the OpenSpec schema.
spec-validate: (_require "openspec" "1.8")
    openspec validate --all --strict

# Proposal, spec directories and task plan must agree.
gate-capability-index: (_require "python3" "3.10")
    python3 tools/check_capability_index.py

# Counts, for the README and for sanity.
spec-stats:
    @printf 'capabilities: %s\n' "$(ls openspec/changes/bootstrap-v1-cybertexel/specs | wc -l | tr -d ' ')"
    @printf 'requirements: %s\n' "$(cat openspec/changes/bootstrap-v1-cybertexel/specs/*/spec.md | grep -c '^### Requirement:')"
    @printf 'scenarios:    %s\n' "$(cat openspec/changes/bootstrap-v1-cybertexel/specs/*/spec.md | grep -c '^#### Scenario:')"
    @printf 'tasks:        %s\n' "$(grep -c '^- \[ \]' openspec/changes/bootstrap-v1-cybertexel/tasks.md)"

# --- build and test ----------------------------------------------------------

build:
    @just _unimplemented build 1.1

test: build
    @just _unimplemented test 1.7

examples:
    @just _unimplemented examples 16.2

bench:
    @just _unimplemented bench 17.3

format:
    @just _unimplemented format 1.1

format-check:
    @just _unimplemented format-check 1.1

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
    @just _unimplemented gate-layering 1.2

# Permissive-only dependencies, attribution complete, vendored trees audited.
gate-licence:
    @just _unimplemented gate-licence 1.4

# One version, consumed by build, ABI query and every binding manifest.
gate-version-consistency:
    @just _unimplemented gate-version-consistency 1.3

# Emitted shaders, saved documents and exported textures are byte-identical.
gate-determinism:
    @just _unimplemented gate-determinism 1.7

# Every C entry point reachable from Python, Swift and Rust.
gate-binding-parity:
    @just _unimplemented gate-binding-parity 14.13

# No symbol or descriptor removed without a major version bump.
gate-abi-diff:
    @just _unimplemented gate-abi-diff 14.4

# Every capability has a numbered example that runs and asserts.
gate-example-coverage:
    @just _unimplemented gate-example-coverage 16.5

# Every executor agrees with the CPU reference within the declared tolerance.
# Device-dependent: run where the hardware exists.
gate-parity:
    @just _unimplemented gate-parity 7.6

# Budgeted operations measured on a named reference device.
# Device-dependent: run where the hardware exists.
gate-budgets:
    @just _unimplemented gate-budgets 17.6

# --- aggregate ---------------------------------------------------------------

# Everything that needs no device. This is what CI runs and what a contributor
# runs before pushing.
check: spec-validate gate-capability-index gate-layering gate-licence \
       gate-version-consistency gate-determinism gate-binding-parity \
       gate-example-coverage gate-abi-diff format-check

# Everything that can be checked before there is any code.
check-spec: spec-validate gate-capability-index
