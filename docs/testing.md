# Testing and determinism

`just test` builds the native headless preset, runs every CTest unit/integration
test and then runs the Python gate tests. `just test-sanitize` repeats the CTest
suite with AddressSanitizer and UndefinedBehaviorSanitizer enabled. CI gives the
sanitizer configuration its own job.

`just fuzz-project-container` builds the project-container reader with Clang,
libFuzzer, AddressSanitizer, and UndefinedBehaviorSanitizer, then runs a fixed
20,000-input campaign from a deterministic seed corpus. Inputs are capped at 1
MiB, parser-owned decoded allocations at 8 MiB, and each individual decoded
payload and record count has a stricter ceiling. CI runs this gate after the
sanitized test suite; any crash, out-of-bounds access, unexpected exception,
timeout, or memory-limit breach fails the job.

The determinism gate is registry-driven. Each category in
`tests/determinism/cases.json` supplies a command and the output files it owns.
The runner gives the command two clean output directories through
`CTEX_DETERMINISM_OUTPUT_DIR` and compares every declared file byte for byte.

Pass one or more category names to `tools/check_determinism.py` to run a focused
subset. The labeled shader-emission and project-I/O scenario suites use this
mode so unrelated later categories do not substitute for, or block, their
reproducibility coverage.

The texture-export category deliberately remains red until its owning task
registers real outputs. An empty category is reported by name and task rather
than counted as a pass. Project-save now writes a versioned sparse container
twice and compares the complete binary output.
