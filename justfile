_default:
    @just --list

# --- specification -----------------------------------------------------------

# Validate every spec and change against the OpenSpec schema.
spec-validate:
    openspec validate --all --strict

# Proposal, spec directories and task plan must agree.
gate-capability-index:
    python3 tools/check_capability_index.py

# Counts, for the README and for sanity.
spec-stats:
    @printf 'capabilities: %s\n' "$(ls openspec/changes/bootstrap-v1-cybertexel/specs | wc -l | tr -d ' ')"
    @printf 'requirements: %s\n' "$(cat openspec/changes/bootstrap-v1-cybertexel/specs/*/spec.md | grep -c '^### Requirement:')"
    @printf 'scenarios:    %s\n' "$(cat openspec/changes/bootstrap-v1-cybertexel/specs/*/spec.md | grep -c '^#### Scenario:')"
    @printf 'tasks:        %s\n' "$(grep -c '^- \[ \]' openspec/changes/bootstrap-v1-cybertexel/tasks.md)"

# Everything that can be checked before there is code.
check: spec-validate gate-capability-index

# --- gates -------------------------------------------------------------------
#
# These are named by openspec/changes/bootstrap-v1-cybertexel/tasks.md. They
# fail until the task that implements them is done, deliberately: a gate that
# passes before it exists is exactly the failure device-gate warns about.

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

# Every executor agrees with the CPU reference within the declared tolerance.
gate-parity:
    @just _unimplemented gate-parity 7.6

# Emitted shaders, saved documents and exported textures are byte-identical.
gate-determinism:
    @just _unimplemented gate-determinism 1.7

# Budgeted operations measured on a named reference device.
gate-budgets:
    @just _unimplemented gate-budgets 17.6

# Every C entry point reachable from Python, Swift and Rust.
gate-binding-parity:
    @just _unimplemented gate-binding-parity 14.13

# Every capability has a numbered example that runs and asserts.
gate-example-coverage:
    @just _unimplemented gate-example-coverage 16.5

# --- build -------------------------------------------------------------------

build:
    @just _unimplemented build 1.1

test:
    @just _unimplemented test 1.7
