# Resource accounting

CyberTexel exposes one device-independent resource ledger through
`ctex/xport/resource_accounting.hpp` and the `ctex_resource_ledger_*` C API.
The ledger combines library and host observations without accepting graphics
API handles.

## Allocation identity

Each physical allocation has a non-zero `uint64_t` identity chosen by the
producer. The identity must remain unique and stable while that allocation is
reported. Re-submitting the identity updates its roles; it cannot change its
byte size, category, or device descriptor. Remove the identity before reusing
it for different storage.

A shared or unified allocation is submitted once with both CPU- and
GPU-resident role bits. Its bytes contribute to both role totals, because both
forms of access are available, but contribute only once to `physical_bytes` and
`allocation_count`. The same rule applies when pinned or in-flight roles
overlap residency roles.

## Categories and roles

Every allocation has one owning category:

- document storage;
- history;
- recovery record;
- mesh map;
- composite;
- cache; or
- temporary.

Categories identify why storage exists. Roles independently identify where it
resides and whether work retains it: CPU resident, GPU resident, backing store,
pinned, and in flight. At least one residency role is required. Pinned and
in-flight are additional states and are not residency locations.

GPU-resident allocations require three copied, API-independent strings:
`backend`, `device_identifier`, and `heap_identifier`. For example, a Metal
host can report `metal`, its registry identifier, and a logical heap name. The
library never stores a `MTLResource`, Vulkan handle, or Direct3D object.

## Reports and lifecycle

`ResourceLedger::report()` and `ctex_resource_ledger_get_report()` return
physical and per-role totals plus a fixed entry for every category. Category
entries therefore remain indexable even when their totals are zero. The C API
uses the normal sizing contract: passing no category array queries the required
count, and an undersized array returns `CTEX_RESULT_BUFFER_TOO_SMALL` without
changing either output.

The ledger is thread-safe. It does not own the reported allocations and does
not poll graphics APIs. Producers update it when allocation residency or
lifetime changes. Budget reservation, eviction, and tiled admission build on
this accounting contract in roadmap tasks 19.2–19.3.
