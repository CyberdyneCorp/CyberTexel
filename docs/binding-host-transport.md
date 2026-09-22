# Host execution from language bindings

Python, Swift and Rust expose the same device-free host workflow. A host can:

1. emit a real shader artifact and JSON pass plan with
   `emit_default_host_material` / `emitDefaultHostMaterial`;
2. submit the pass plan's logical input and output resources to a
   `HostExecutionSession`;
3. execute the returned shader and plan on its own device, without passing a
   device handle to CyberTexel;
4. report a typed completion and recovery record; and
5. keep the completed logical generation host-resident without transferring
   pixels back to the library.

The emission helper owns its returned shader bytes and strings. Submission
descriptors borrow caller strings and arrays only for the synchronous call.
Completion results copy messages and released-resource identities into native
language values before releasing the result handle.

`SnapshotPool` pins exact channel generations under its byte ceiling. A
snapshot produces a `HostReadback` with one caller-owned output allocation per
changed tile and a declared row pitch, pixel stride and byte count internally.
The output is inaccessible while the request is pending. `complete` requires
one exact-size payload per requested tile; the native layer validates the whole
batch before atomically making all outputs readable. Destroying a readback
releases its retained snapshot pin. Python reference-counted storage, Swift
reference types and Rust ownership keep every native dependency alive for at
least that interval.

No revision or delta query initiates a transfer. Hosts request readback
explicitly only when CPU pixels are actually needed, such as for save or
export.
