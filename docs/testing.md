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

`just fuzz-image-decoders` applies the same sanitizer-backed, deterministic
20,000-input campaign to the in-memory PNG, JPEG, TGA, BMP, TIFF, OpenEXR,
Radiance HDR and PSD dispatch paths. Its fixed corpus contains a content marker
for every decoder plus a valid PNG; coverage-guided mutations are generated
only for the duration of the run. Inputs are capped at 1 MiB, decoded output at
8 MiB and dimensions at 4096 per axis. A failure is retained under
`build/fuzz-artifacts` for promotion into the permanent regression suite.

Linux runs the fuzz recipes natively. macOS runs the identical Linux Clang gate
in the cached `tools/docker/fuzz.Dockerfile` image because Apple Command Line
Tools do not ship libFuzzer. The Docker path keeps the source tree read-only and
reuses a named build volume; Docker is therefore the only additional local
prerequisite on macOS.

The determinism gate is registry-driven. Each category in
`tests/determinism/cases.json` supplies a command and the output files it owns.
The runner gives the command two clean output directories through
`CTEX_DETERMINISM_OUTPUT_DIR` and compares every declared file byte for byte.

Pass one or more category names to `tools/check_determinism.py` to run a focused
subset. The labeled shader-emission and project-I/O scenario suites use this
mode so unrelated later categories do not substitute for, or block, their
reproducibility coverage.

The project-save category writes a versioned sparse container twice and compares
the complete binary output. The texture-export category likewise compares both
an encoded texture and its machine-readable report. Empty categories are still
rejected by name and owning task rather than counted as a pass.

Capability scenario suites use CTest labels and checked Markdown matrices.
For example, `just test-texture-document-scenarios` runs the complete document
suite, while its matrix test refuses missing, renamed, duplicate, empty, or
unknown executable evidence for any OpenSpec scenario.
