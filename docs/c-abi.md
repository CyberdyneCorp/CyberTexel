# C ABI foundations

The stable host boundary is the C header `ctex/capi.h`. Every exported symbol
uses the `ctex_` prefix and the shared `cybertexel_c` library applies a platform
export map that hides every other symbol. The regular `cybertexel` static target
remains available for native static linkage. iOS consumes that static target.

Library objects are represented by incomplete C types. A caller creates a
`ctex_document*` with `ctex_document_create` and releases it exactly once with
`ctex_document_destroy`; the structure behind that pointer is private and may
change without changing the ABI. Destroying a null handle is permitted.

Every fallible entry point returns `ctex_result`. Zero is success. The stable
nonzero categories distinguish invalid arguments, missing resources,
unsupported operations, allocation failure, budget refusal, cancellation and
unexpected internal failures. C++ exceptions are caught by the C boundary and
translated to one of those values.

Bulk results use caller-owned buffers and the same two-call contract. A null
buffer with size zero reports the exact required size without writing data. A
non-null buffer that is too small returns `CTEX_RESULT_BUFFER_TOO_SMALL`, reports
the required size, and leaves the complete buffer untouched. A right-sized
second call fills it completely. `ctex_document_get_texture_set_ids` applies
this contract to the ordered stable texture-set identities: each UTF-8 identity
is NUL-terminated and entries are packed consecutively; `out_count` reports how
many entries are present.

Input descriptors begin with a `uint32_t size`. Callers set it to the descriptor
size they compiled against; the library rejects values below the required prefix
or above its own current structure before reading any later field. Fields are
read only when `size` covers the complete field. The first
`ctex_texture_set_descriptor` prefix ends before `default_bit_depth`, so an older
caller receives the documented 8-bit default even if bytes beyond its declared
prefix contain another value. Current callers use
`CTEX_TEXTURE_SET_DESCRIPTOR_CURRENT_SIZE`; the stable older prefix is exposed
as `CTEX_TEXTURE_SET_DESCRIPTOR_V1_SIZE`. Descriptor strings are borrowed for
the duration of `ctex_document_create_texture_set` and copied into the document.

After a failed call, `ctex_get_last_result` returns the same result and
`ctex_get_last_diagnostic` returns an English message naming the operation and
the offending value. Diagnostic state belongs to the calling thread. The
message is owned by CyberTexel, requires no caller allocation, and remains valid
until the next fallible C entry point on that thread. A successful fallible call
clears the diagnostic.

The Linux export surface is constrained by `cmake/exports/cybertexel.map`, macOS
uses `cybertexel.exports`, and Windows uses `cybertexel.def`. The
`c-abi-export-surface` test inspects the produced Linux shared object and refuses
any visible symbol outside `ctex_*`.
