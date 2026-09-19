# C ABI capability coverage

[`abi/capi-capabilities.json`](../abi/capi-capabilities.json) is the coverage
inventory for the public C boundary. It contains every capability directory in
the active OpenSpec change and classifies it as:

- `runtime`, whose every OpenSpec requirement must name one or more declared
  `ctex_*` entry points;
- `boundary`, which follows the same rule but may use repository evidence for a
  requirement that describes the boundary itself; or
- `non-runtime`, which must state why it has no library operation and name the
  repository gate or delivery task that proves it.

`just gate-c-api-coverage` compares the manifest to the live OpenSpec requirement
titles and `include/ctex/capi.h`. It rejects missing or invented capabilities,
missing or invented requirements, unknown symbols, unmapped public symbols,
duplicate mappings, and unexplained non-runtime exclusions. A single token entry
cannot make a broad capability pass: coverage is checked requirement by
requirement.

The gate is intentionally red while task 14.8 is in progress. The current C ABI
fully maps its boundary requirements and records the implemented document and
texture-set, colour-management, read-only mesh-ingest, bounded in-memory image
I/O and canonical stroke reconstruction operations, but 201 runtime requirements
still lack C entry points. Mesh handles own allocator-routed
copies of host buffers, expose named UV sets and revision changes, and enforce
the declared 100,000,000-vertex and 100,000,000-triangle ceilings before reading
array contents.
The gate is not part of the aggregate `just check` until that count reaches zero;
its unit tests run in the normal tooling suite throughout the migration.
