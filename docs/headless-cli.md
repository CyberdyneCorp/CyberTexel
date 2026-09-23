# Headless command line

The native build installs a `cybertexel` executable alongside the C library.
It is windowless and defines six command routes:

- `export` writes texture sets from a document;
- `bake-request` requests and binds maps from a configured provider;
- `apply` applies a smart material or preset to a named texture set;
- `run` executes a Python script against a document;
- `info` reports document contents and estimated sizes;
- `validate` checks a document, material, or preset without producing output.

The current skeleton implements complete command selection and argument
validation. After a valid invocation reaches its selected route it returns the
unsupported-operation outcome until roadmap task 15.2 supplies that operation.
It never creates output in this state.

Run `cybertexel --help` for the command list or
`cybertexel <command> --help` for required and optional command arguments.
Shared options select text or JSON reporting, quiet output, the executor,
memory and texel ceilings, and the worker bound. Values are checked before
dispatch; invalid input is named on the error stream.

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
suite.
