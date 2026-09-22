# Python binding

The `cybertexel` wheel is a typed Python layer over the stable C ABI. It bundles
the platform shared library, checks ABI major version zero during import, and
depends only on NumPy at runtime. Native failures raise `CyberTexelError`
subclasses carrying the stable result, diagnostic code, and diagnostic text.

`decode_image()` returns a C-contiguous NumPy array in the decoded native dtype
and shape `(height, width, channels)`. The C decoder writes directly into that
caller-owned array after its sizing pass. `Document.read_channel()` similarly
returns an independent caller-owned snapshot populated directly by the C ABI;
it is not a view and stays valid after edits or document destruction. This
initial idiomatic layer also accepts C-contiguous NumPy mesh and mesh-map
buffers. Exact `float32`/`uint32` mesh arrays cross without an intermediate
binding copy before the native mesh takes ownership of its own copy; map imports
behave the same for `uint8`, `uint16`, and `float32`. Other dtypes or strided
inputs are explicitly normalized to a contiguous supported array. Tasks
14.12–14.13 extend the host-executed route and mechanically enforce complete C
surface parity.

`MeshMapSet.generate_mask()` returns a caller-owned `float32` NumPy array and
uses generator defaults. Missing required maps fail before allocation and raise
`MissingResourceError` with the native diagnostic intact.

Documents own their native handle. A context manager or `close()` releases it
deterministically; finalization is a fallback, and repeated `close()` calls are
safe.

Build and test the platform wheel with:

```sh
just test-python-binding
```

Set `CTEX_PYTHON_VERSION` to select a supported interpreter for the isolated
wheel smoke test. Set `CYBERTEXEL_LIBRARY` only when building against a shared
library outside `build/`; installed wheels normally load their bundled copy.
