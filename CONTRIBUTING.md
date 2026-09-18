# Contributing

## Everything goes through `just`

A recipe is the single definition of its command. CI invokes the same recipes a
contributor runs, so a check cannot pass locally and differ in CI. If you find
yourself typing a `cmake`, `ctest`, `pytest` or `cargo` invocation by hand, that
is a missing recipe rather than a thing to remember.

```
just              # list every recipe
just prereqs      # report missing tools by name
just check-spec   # specification checks; needs no build
just build
just test
just examples
just check        # everything that needs no device — run this before pushing
```

Two gates depend on hardware and are excluded from `just check`; run them where
the device exists:

```
just gate-parity    # executors against the CPU reference
just gate-budgets   # budgeted operations on a named reference device
```

## Gates fail until they are built

A gate whose implementing task is not yet done exits non-zero and names that
task. This is deliberate: a gate that passes before it exists checks nothing
while reporting success, which is the failure the `device-gate` capability is
written to prevent. Do not stub one to green.

## Prerequisites

| Tool | Minimum | Needed for |
|---|---|---|
| [`just`](https://github.com/casey/just) | 1.0 | every recipe |
| Python | 3.10 | specification gates, bindings, examples |
| [`openspec`](https://github.com/Fission-AI/OpenSpec) | 1.8 | specification validation |
| CMake | 3.24 | building, once task 1.1 lands |
| A C++20 toolchain | — | building |
| Rust | stable | the Rust bindings |
| Swift | 5.9 | the Swift package |

Only the first three are needed today, because there is no code yet.

## The specification comes first

This repository is spec-first. `openspec/changes/bootstrap-v1-cybertexel/` holds
the founding change; `tasks.md` and its checkboxes are the authority on what is
done.

Before writing code for a task:

1. Read the capability spec it belongs to. The requirement is the contract; the
   scenarios are the tests you owe.
2. If the spec is wrong or incomplete, change the spec in the same pull request
   rather than writing code that disagrees with it.
3. Every capability must keep at least one scenario per requirement and one task
   group that turns its scenarios into tests. `just gate-capability-index`
   enforces both.

## Commits and pull requests

- One task per commit keeps resume boundaries clean. Tick the box in the same
  commit or the next one.
- Commit subject: `feat(<module>): <task>`, or `fix`, `docs`, `test`, `chore`.
- A pull request needs a body explaining what changed and why.
- A behavioural change updates the specification in the same pull request.
- A change that alters an example's committed output updates that output in the
  same commit.
