# Resource residency scenario coverage

`just test-resource-residency-scenarios` runs every CTest carrying the
`resource-residency-scenario` label. The checked matrix maps every OpenSpec
resource-residency scenario to executable C++ and strict-C evidence.

| OpenSpec scenario | CTest | Covered behavior |
|---|---|---|
| Unified memory accounting | `resource-accounting`, `c-abi-resource-accounting` | One physical identity exposes CPU and GPU access roles while contributing its bytes once to the physical total |
| Large export under a small budget | `resource-accounting`, `c-abi-resource-accounting` | Whole-image work is refused, the largest fitting tile batch is reserved, peak projected usage is reported, and refusal leaves committed storage unchanged |
| Evicting authored content | `tiled-image`, `c-abi-tile-backing` | Flat and UDIM authored tiles require successful lossless backing, release resident bytes, reload the exact generation bit-for-bit, and retain logical occupancy |
| Lower quality preview | `resource-accounting`, `c-abi-resource-accounting` | Ordered host policy defers derived work or selects a smaller preview while authored allocation identity, size and precision remain unchanged; no fitting policy reports over-budget |
| Mobile suspension interrupts readback | `c-abi-host-transport-snapshot`, `project-autosave`, `c-abi-project-autosave` | A pending host readback is cancellable without publishing bytes; quiesce stops admission, requests cancellation, reports the unsaved range, and resume opens only the atomically published durable revision |

The suite is host-neutral and deterministic. Reference-host memory pressure,
lifecycle timing and device traffic remain numeric device gates under tasks
17.12–17.14; those measurements extend this evidence rather than replacing the
contract tests here.
