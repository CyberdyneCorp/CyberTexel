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
lifetime changes.

## Budget admission

`ResourceLedger::admit()` and `ctex_resource_ledger_admit()` enforce separate
CPU, GPU, backing-store and temporary ceilings. A request declares fixed
requirements—such as history and recovery storage—and one temporary
requirement per bounded work item. Successful admission returns a move-only
reservation that prevents concurrent operations from overcommitting the same
budgets.

The admission order is deterministic:

1. admit the complete operation if it fits;
2. otherwise schedule the largest work-item batch that fits;
3. if even one item does not fit and a cache-eviction callback is registered,
   consider unpinned, non-in-flight `cache` allocations in identity order and
   stop evicting as soon as work fits; and
4. refuse atomically if fixed requirements plus one item still cannot fit.

Only reconstructible storage belongs in the `cache` category. Document,
history, recovery, pinned and in-flight allocations are never selected for
eviction. Before removing an eligible ledger record, admission invokes the
registered callback so the producer releases the physical allocation; without
a callback, no cache record is considered evictable. The callback must not call
back into the same ledger. Refusal leaves every ledger allocation unchanged.
Destroying or releasing the reservation immediately returns its reserved
capacity; admitted work then repeats the reported batch size until all work
items are complete.
Sparse authored-tile backing and reload policy builds on this contract in task
19.3.

## Sparse tiles and lossless backing

Clear tiles remain logical constants and allocate neither pixels nor backing
storage. A host can attach a `TileBackingStore` to an authored image, or the
equivalent callback descriptor to a document channel through C. Backing keys
combine a process-local image namespace, tile coordinate and exact generation;
they do not depend on canvas resolution or UDIM number.

Eviction follows a lossless sequence: write the complete physical tile, accept
the write only when the host confirms success, then release the resident
allocation. A tile retained by history or a snapshot is reported as pinned and
is not evicted. A failed backing write leaves the resident allocation and
pixels untouched. Reads and writes reload an evicted generation into the
image's configured memory resource before access, and a failed reload leaves
the backed state intact. Editing a restored tile discards its stale backing key;
the next eviction writes the new generation.

The C operations accept `udim_tile_number == 0` for the texture set's primary
channel image or a concrete UDIM number for sparse tiled sets. The backing
callbacks own persistence and receive explicit store, load, per-key discard and
namespace-release notifications. They must remain valid until the document is
destroyed and must not call back into that document from inside a callback.

## Host preview-quality policy

`ResourceLedger::admit_preview_quality()` and
`ctex_resource_ledger_admit_preview_quality()` accept host-ordered preview
choices. Each choice declares a resolution, one or more exact resource
requirements, and whether derived work is deferred. Choices cannot exceed the
declared full-quality dimensions. The first choice that fits without changing
committed allocations wins. If none fits and a cache-release callback exists,
the same order is retried while considering eligible caches in allocation
identity order. If no allowed choice fits, admission returns over-budget and
changes neither allocations nor reservations.

The report distinguishes full quality, reduced resolution, deferred derived
work, and a combined reduction/defer outcome. It also names the selected option,
dimensions, projected resource totals, and released cache count. The returned
reservation protects that preview choice against concurrent overcommit.

This policy is deliberately separate from image and export state: it can reserve
preview composites and temporary work, but it cannot mutate authored dimensions,
pixel formats, stored precision, or export inputs. Hosts must build preview
resources from the selected option and continue to use authored resources for
full-quality export.
