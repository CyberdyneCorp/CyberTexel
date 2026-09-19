# Testing and determinism

`just test` builds the native headless preset, runs every CTest unit/integration
test and then runs the Python gate tests. `just test-sanitize` repeats the CTest
suite with AddressSanitizer and UndefinedBehaviorSanitizer enabled. CI gives the
sanitizer configuration its own job.

The determinism gate is registry-driven. Each category in
`tests/determinism/cases.json` supplies a command and the output files it owns.
The runner gives the command two clean output directories through
`CTEX_DETERMINISM_OUTPUT_DIR` and compares every declared file byte for byte.

Pass one or more category names to `tools/check_determinism.py` to run a focused
subset. The labeled shader-emission scenario suite uses this mode so later empty
project-save and texture-export categories do not substitute for, or block,
shader reproducibility coverage.

The project-save and texture-export categories deliberately remain red until
their owning tasks register real outputs. An empty category is reported by name
and task rather than counted as a pass.
