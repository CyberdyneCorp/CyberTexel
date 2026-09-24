# Reference hosts

CyberTexel does not own a GPU device. It emits shader source and an ordered
pass plan, and the host runs them on the device it already has. These are the
programs that prove that contract against real device APIs, as
`build-packaging`'s reference-host requirement and task 18.2 require.

They are **integration hosts, not examples**. The numbered Python examples are
display-free and prove the contract with a software stand-in; these link a real
graphics API. The specification requires them to be distinct.

| Host | Device API | Platforms | Recipe |
| --- | --- | --- | --- |
| [`desktop-wgpu`](desktop-wgpu) | `wgpu` (Metal, Vulkan, DX12) consuming WGSL | macOS, Linux | `just host-desktop` |
| [`ipad-metal`](ipad-metal) | Metal consuming MSL | iPadOS, macOS | `just host-ipad` |

## What a run proves

1. The library emits a shader artifact and a device-independent pass plan and
   never sees a device handle.
2. The host parses the plan strictly — an unknown field fails rather than being
   ignored, so a change to the emitted plan breaks the host and CI.
3. The host creates the declared resource, compiles the emitted source, honours
   the declared vertex layout, render target, load operation and draw command,
   and executes on the device.
4. The host reports completion, and the library publishes the revision without
   the pixels leaving the device.
5. The host drives the explicit transport path: revision cursor, budgeted
   snapshot, tile memory layout, a pending readback that publishes nothing, and
   completion with the exact tile payloads.

## Exit codes

| Code | Meaning |
| --- | --- |
| 0 | The route ran on a device and every assertion held. |
| 2 | The route failed. |
| 3 | No adapter or device was available, so the run is **unmeasured** — never a pass. |

Code 3 exists because `device-gate` forbids substituting an absent measurement.
A CI runner without a usable adapter reports unmeasured; it does not pass.

## Named-device CI gate

`.github/workflows/reference-device-gate.yml` runs on the dedicated
`ctex-m3-pro-ipad-air` self-hosted runner after pushes to `main` and by manual
dispatch. The runner must run in the MacBook Pro M3 Pro's logged-in Aqua
session with its iPad Air M3 connected, unlocked and in Developer Mode. The
workflow never runs pull-request code on that personal machine.

`just gate-reference-devices` checks both hardware identities, runs the visible
desktop WGSL benchmark, then runs the iPad XCTest suite including the full
twenty-minute workload. It extracts the XCTest attachments, compares latency,
memory and zero-byte synchronous readback results with the declared budgets,
and uploads the logs, result bundle and schema-1 measurements. Missing devices,
hidden windows, skipped tests and missing attachments fail the job. Use
`just gate-reference-desktop` or `just gate-reference-ipad` to rerun one half
locally; the complete recipe is the release gate.

## Dependency audit

These hosts are not shipped artifacts, so their graphics dependencies are
outside `just gate-licence`, whose subject is what is compiled into a shipped
binary. That exclusion is deliberate and recorded here rather than implied.

## Devices

`benchmarks/device_gate.json` names the reference devices. Both hosts have been
run on theirs: the desktop host on the MacBook Pro M3 Pro through the Metal
backend, and the iPad probe on the iPad Air 13-inch (M3). Between them they
record input-to-visible, the synchronous-readback budgets and a twenty-minute
sustained tablet workload, closing tasks 17.12–17.14. The remaining numeric
budgets are tasks 17.5 and 17.10, and building or running these hosts does not
by itself record a performance claim — an absent adapter exits 3 and reports
unmeasured.
