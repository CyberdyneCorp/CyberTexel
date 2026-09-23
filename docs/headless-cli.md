# Headless command line

The native build installs a `cybertexel` executable alongside the C library.
It is windowless and defines six command routes:

- `export` writes texture sets from a document;
- `bake-request` requests and binds maps from a configured provider;
- `apply` applies a smart material or preset to a named texture set;
- `run` executes a Python script against a document;
- `info` reports document contents and estimated sizes;
- `validate` checks a document, material, or preset without producing output.

The current implementation provides complete command selection and argument
validation. `validate` fully parses project documents, smart materials and
stroke presets. `info` parses project containers and their canonical live
texture-document assets, reporting texture sets, layer entries, atlases,
editable entries, applied presets, tiled-image storage, occupied tiles, decoded
image size, resources, assets and forward-preserved sections. `apply` opens the
single live texture document in a project, validates and resolves a canonical
smart material, applies it to the named texture set, and atomically writes a
new project. Required material resources must have project metadata and must be
packed or readable relative to the input project; otherwise the command returns
the missing-resource outcome without touching its output. Repeated applications
receive deterministic suffixes while retaining their preset origin.

`export` opens that same live document, resolves a built-in export preset,
samples the document's current flattened typed channels, and encodes every
planned texture through the native export pipeline. It writes into a new staged
directory and publishes that directory only after all outputs are complete, so
an error cannot expose a partial output set. Its JSON report identifies every
file with byte size, dimensions, format, colour space and bit depth. Existing
output directories are refused instead of being merged or overwritten. Mesh
replacement arguments remain reserved and return the unsupported-operation
outcome until the replacement-mesh input path is implemented.

`run` opens the project's single texture-document asset through the installed
Python binding and executes the selected script. A script defines
`main(document)` and may use the supplied `cybertexel.Document` plus any other
public Python API. Normal and file-descriptor-level script output is redirected
to the diagnostic stream so a JSON report remains the only standard output.
After successful execution, the edited document is written into the original
container, revalidated by the parent process and atomically published at
`--output`; a script exception or malformed result leaves an existing output
unchanged. `CTEX_PYTHON` selects the interpreter and defaults to `python3` on
POSIX or `python` on Windows. That interpreter must have the CyberTexel wheel
installed.

`bake-request` currently returns the unsupported-operation outcome until the
remaining part of roadmap task 15.2 supplies provider attachment. Replacement
mesh export also remains unsupported.

Run `cybertexel --help` for the command list or
`cybertexel <command> --help` for required and optional command arguments.
Shared options select text or JSON reporting, quiet output, the executor,
memory and texel ceilings, and the worker bound. Values are checked before
dispatch; invalid input is named on the error stream.

`--executor` takes `cpu`, `auto`, or `host` and overrides `CTEX_EXECUTOR`.
Without the flag, the environment value is used; without either, automatic
selection chooses the best available executor. The standalone binary always
registers the CPU reference executor. A requested host executor therefore
falls back to CPU and records the request, selection source, selected executor,
fallback, and reason in the report.

`--memory-ceiling`, `--texel-ceiling`, and `--workers` require positive integer
values. Input size and project-reader allocations are checked against the
memory ceiling before work proceeds. `info` also totals logical image texels
with checked arithmetic and refuses a document above the texel ceiling. JSON
reports include the effective limits, resolved inputs, operations, outputs,
clamped parameters, executor decision, exit code, diagnostic when applicable,
and elapsed time. With `--quiet`, text reports are suppressed; diagnostics
remain on standard error. Supplying `--report json` keeps the JSON document by
itself on standard output, including for argument and runtime failures.

The stable process outcomes begin with:

| Code | Outcome |
| ---: | --- |
| 0 | success |
| 2 | invalid arguments |
| 3 | missing or unreadable input |
| 4 | unsupported operation |
| 5 | unresolved resource or mesh map |
| 6 | cancellation |
| 7 | declared budget exceeded |
| 70 | internal error |

`just test-cli` builds the executable and runs its process-level regression
suite. The suite repeats `export`, `apply`, and Python `run` from identical
inputs and byte-compares every resulting texture and project, in addition to
treating command help as an interface: every implemented command option and
every global option must appear in `--help`, together with accepted values and
defaults.
