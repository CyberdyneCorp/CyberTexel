# C ABI scenario coverage

`just test-c-abi-scenarios` runs the portable C ABI tests carrying the
`c-abi-scenario` label. Linux CI additionally runs the export-surface check
against the built shared library. The matrix test compares this document with
the OpenSpec capability and refuses missing, duplicate, unknown, unregistered
or unlabelled evidence.

| OpenSpec scenario | Executable evidence | Covered behavior |
| --- | --- | --- |
| Linked beside a sibling engine | `c-abi-foundation`, `c-abi-export-surface` | A C host links both `ctex_` and sibling `cyber_` symbols; the Linux export audit refuses undeclared or non-`ctex_` dynamic exports. |
| Consumable from C | `c-abi-foundation` | The public header and opaque handles compile and execute from a strict C translation unit. |
| An exception is caught at the boundary | `c-abi-boundary-contract` | A C++ allocator callback throws through an internal allocation; the `noexcept` C boundary returns internal-error plus a stable diagnostic instead of unwinding into the host. |
| Struct layout changes | `c-abi-versioned-descriptors`, `c-abi-compatibility-policy` | Older descriptor prefixes retain defaults, while the ABI policy rejects removal or mutation of committed structure prefixes. |
| Distinguishing failures | `c-abi-foundation`, `c-abi-host-logging` | Missing resources and invalid arguments retain distinct integer result and diagnostic codes. |
| Reading a diagnostic | `c-abi-foundation` | Thread-local retrieval names the failed operation and offending value without caller allocation. |
| Two-call sizing | `c-abi-caller-buffers` | A null layer-list buffer reports the complete required byte and item counts without writing. |
| Buffer too small | `c-abi-caller-buffers` | An undersized buffer reports its required size and remains byte-for-byte untouched. |
| Older caller, newer library | `c-abi-versioned-descriptors` | The library reads only the declared descriptor prefix and applies the documented default to appended fields. |
| Descriptor size is implausible | `c-abi-versioned-descriptors` | A descriptor larger than the current layout is rejected without mutating its document. |
| Host checks compatibility | `version-consistency`, `c-abi-foundation` | The version query is callable before handle creation and reports the shared semantic/ABI version components. |
| Symbol diff gate | `c-abi-compatibility-policy`, `c-abi-export-surface` | Mutation tests prove removed symbols, changed signatures, reordered fields and renumbered enums fail; Linux compares the live shared-library exports. |
| Two documents in two threads | `c-abi-two-document-concurrency` | Two worker threads populate independent documents concurrently and retain isolated results and diagnostics. |
| Independent documents | `c-abi-two-document-concurrency` | Failure state and document contents in one worker do not affect the other document. |
| Progress from a worker thread | `c-abi-cpu-execution` | Worker-issued progress and work callbacks use the registered opaque state while the executor serializes progress reporting. |
| Coverage gate | `c-abi-full-surface-coverage` | Every runtime capability requirement maps to one or more current public C operations. |
| Routing library logs | `c-abi-host-logging` | An installed sink receives severity, category, diagnostic message and the exact user-data pointer. |
| Silent by default | `c-abi-host-logging` | Removing the sink prevents later diagnostics from reaching the prior callback. |
| A host localizes a failure | `c-abi-foundation`, `c-abi-host-logging` | Stable diagnostic codes accompany English text, allowing translation without parsing prose. |
| Host allocator | `c-abi-host-allocator` | Long-lived handles allocate and deallocate through the installed aligned callbacks and preserve their user data. |
