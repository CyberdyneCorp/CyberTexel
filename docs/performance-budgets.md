# Reference-device performance budgets

This file is generated from `benchmarks/device_gate.json` by
`python3 tools/device_gate.py document --write`. Targets are not measurements.
A row remains **unmeasured** until a dated, commit-pinned run from the exact named
device is recorded; developer-machine timings are informational only.

## Reference devices

| ID | Kind | Model | CPU | GPU | Memory | Operating system | Host |
| --- | --- | --- | --- | --- | --- | ---: | --- |
| `macbook-pro-m3-pro-18gpu-36gb` | desktop | MacBook Pro 14-inch (Mac15,7, MRW23LL/A) | Apple M3 Pro, 12 cores (6 performance, 6 efficiency) | Apple M3 Pro, 18 cores, Metal 4 | 38654705664 bytes | macOS 27.0 (26A428) | Swift/Metal reference host |
| `ipad-air-13-m3-8gb` | tablet | iPad Air 13-inch (M3) (iPad15,5) | Apple M3, 8 cores | Apple M3, 9 cores, Metal 4 | 8589934592 bytes | iPadOS 27.0 (24A437) | Swift/Metal reference host |

## Configurations

| ID | Description |
| --- | --- |
| `interactive-4k` | 4096x4096 texture set, 250k-triangle mesh, 8 visible layers, 4 enabled channels, resident authored tiles, 120 Hz input, 120 Hz desktop or 120 Hz tablet presentation |
| `batch-4k` | 4096x4096 texture set, 250k-triangle mesh, 8 visible layers, 4 enabled channels, resident inputs, one output texture set |
| `stamp-scaling` | Identical 64-pixel-radius stamp and touched tiles on 2048x2048 and 16384x16384 texture sets |
| `sustained-mobile-4k` | 20-minute 120 Hz input workload at 4096x4096 with 8 layers and 4 channels; budgets apply to the final five minutes |

## Budgets

| ID | Operation | Metric | Ceiling | Device | Configuration | Latest |
| --- | --- | --- | ---: | --- | --- | --- |
| `desktop-stamp-median` | stamp | median_ms | 4 ms | `macbook-pro-m3-pro-18gpu-36gb` | `interactive-4k` | **unmeasured** — n/a; 2026-09-23; `febc2a329d74a92adb30f6f34ef786a4c77a1e8f` |
| `desktop-stamp-p95` | stamp | p95_ms | 8 ms | `macbook-pro-m3-pro-18gpu-36gb` | `interactive-4k` | **unmeasured** — n/a; 2026-09-23; `febc2a329d74a92adb30f6f34ef786a4c77a1e8f` |
| `desktop-stroke` | stroke | p95_ms | 25 ms | `macbook-pro-m3-pro-18gpu-36gb` | `interactive-4k` | **unmeasured** — n/a; 2026-09-23; `febc2a329d74a92adb30f6f34ef786a4c77a1e8f` |
| `desktop-composite` | composite | p95_ms | 20 ms | `macbook-pro-m3-pro-18gpu-36gb` | `batch-4k` | **unmeasured** — n/a; 2026-09-23; `febc2a329d74a92adb30f6f34ef786a4c77a1e8f` |
| `desktop-emission` | emission | p95_ms | 50 ms | `macbook-pro-m3-pro-18gpu-36gb` | `batch-4k` | **unmeasured** — n/a; 2026-09-23; `febc2a329d74a92adb30f6f34ef786a4c77a1e8f` |
| `desktop-generator` | generator | p95_ms | 100 ms | `macbook-pro-m3-pro-18gpu-36gb` | `batch-4k` | **unmeasured** — n/a; 2026-09-23; `febc2a329d74a92adb30f6f34ef786a4c77a1e8f` |
| `desktop-delta-query` | delta-query | p95_ms | 1 ms | `macbook-pro-m3-pro-18gpu-36gb` | `interactive-4k` | **unmeasured** — n/a; 2026-09-23; `febc2a329d74a92adb30f6f34ef786a4c77a1e8f` |
| `desktop-tile-readback` | tile-readback | p95_ms | 4 ms | `macbook-pro-m3-pro-18gpu-36gb` | `interactive-4k` | **unmeasured** — n/a; 2026-09-23; `febc2a329d74a92adb30f6f34ef786a4c77a1e8f` |
| `desktop-smart-material` | smart-material | p95_ms | 50 ms | `macbook-pro-m3-pro-18gpu-36gb` | `batch-4k` | **unmeasured** — n/a; 2026-09-23; `febc2a329d74a92adb30f6f34ef786a4c77a1e8f` |
| `desktop-export` | export | p95_ms | 250 ms | `macbook-pro-m3-pro-18gpu-36gb` | `batch-4k` | **unmeasured** — n/a; 2026-09-23; `febc2a329d74a92adb30f6f34ef786a4c77a1e8f` |
| `desktop-stroke-memory` | stroke | peak_working_bytes | 67108864 bytes | `macbook-pro-m3-pro-18gpu-36gb` | `interactive-4k` | **unmeasured** — n/a; 2026-09-23; `febc2a329d74a92adb30f6f34ef786a4c77a1e8f` |
| `desktop-composite-memory` | composite | peak_working_bytes | 268435456 bytes | `macbook-pro-m3-pro-18gpu-36gb` | `batch-4k` | **unmeasured** — n/a; 2026-09-23; `febc2a329d74a92adb30f6f34ef786a4c77a1e8f` |
| `desktop-smart-material-memory` | smart-material | peak_working_bytes | 268435456 bytes | `macbook-pro-m3-pro-18gpu-36gb` | `batch-4k` | **unmeasured** — n/a; 2026-09-23; `febc2a329d74a92adb30f6f34ef786a4c77a1e8f` |
| `desktop-export-memory` | export | peak_working_bytes | 536870912 bytes | `macbook-pro-m3-pro-18gpu-36gb` | `batch-4k` | **unmeasured** — n/a; 2026-09-23; `febc2a329d74a92adb30f6f34ef786a4c77a1e8f` |
| `tablet-stroke-memory` | stroke | peak_working_bytes | 25165824 bytes | `ipad-air-13-m3-8gb` | `interactive-4k` | **unmeasured** |
| `tablet-composite-memory` | composite | peak_working_bytes | 100663296 bytes | `ipad-air-13-m3-8gb` | `batch-4k` | **unmeasured** |
| `tablet-smart-material-memory` | smart-material | peak_working_bytes | 100663296 bytes | `ipad-air-13-m3-8gb` | `batch-4k` | **unmeasured** |
| `tablet-export-memory` | export | peak_working_bytes | 201326592 bytes | `ipad-air-13-m3-8gb` | `batch-4k` | **unmeasured** |
| `desktop-visible-median` | input-to-visible | median_ms | 16 ms | `macbook-pro-m3-pro-18gpu-36gb` | `interactive-4k` | **unmeasured** — n/a; 2026-09-23; `febc2a329d74a92adb30f6f34ef786a4c77a1e8f` |
| `desktop-visible-p95` | input-to-visible | p95_ms | 25 ms | `macbook-pro-m3-pro-18gpu-36gb` | `interactive-4k` | **unmeasured** — n/a; 2026-09-23; `febc2a329d74a92adb30f6f34ef786a4c77a1e8f` |
| `desktop-visible-p99` | input-to-visible | p99_ms | 33 ms | `macbook-pro-m3-pro-18gpu-36gb` | `interactive-4k` | **unmeasured** — n/a; 2026-09-23; `febc2a329d74a92adb30f6f34ef786a4c77a1e8f` |
| `tablet-visible-median` | input-to-visible | median_ms | 20 ms | `ipad-air-13-m3-8gb` | `interactive-4k` | **unmeasured** |
| `tablet-visible-p95` | input-to-visible | p95_ms | 33 ms | `ipad-air-13-m3-8gb` | `interactive-4k` | **unmeasured** |
| `tablet-visible-p99` | input-to-visible | p99_ms | 50 ms | `ipad-air-13-m3-8gb` | `interactive-4k` | **unmeasured** |
| `tablet-sustained-final-p95` | sustained-mobile | final_five_minute_p95_ms | 33 ms | `ipad-air-13-m3-8gb` | `sustained-mobile-4k` | **unmeasured** |
| `tablet-sustained-memory` | sustained-mobile | peak_physical_bytes | 536870912 bytes | `ipad-air-13-m3-8gb` | `sustained-mobile-4k` | **unmeasured** |
| `desktop-paint-sync-readback` | resident-paint | synchronous_readback_bytes | 0 bytes | `macbook-pro-m3-pro-18gpu-36gb` | `interactive-4k` | **passed** — 0 bytes; 2026-09-23; `febc2a329d74a92adb30f6f34ef786a4c77a1e8f` |
| `desktop-undo-sync-readback` | resident-undo | synchronous_readback_bytes | 0 bytes | `macbook-pro-m3-pro-18gpu-36gb` | `interactive-4k` | **passed** — 0 bytes; 2026-09-23; `febc2a329d74a92adb30f6f34ef786a4c77a1e8f` |
| `tablet-paint-sync-readback` | resident-paint | synchronous_readback_bytes | 0 bytes | `ipad-air-13-m3-8gb` | `interactive-4k` | **unmeasured** |
| `tablet-undo-sync-readback` | resident-undo | synchronous_readback_bytes | 0 bytes | `ipad-air-13-m3-8gb` | `interactive-4k` | **unmeasured** |

## Recording a run

A reference host writes schema-1 JSON containing `device_id`, `date`, `commit`,
`command` and one measurement per `budget_id`. Reproduce and decide it with:

```sh
CTEX_DEVICE_GATE_RESULTS=benchmarks/results/<run>.json just gate-budgets
python3 tools/device_gate.py document --results benchmarks/results/<run>.json --write
```

Each measurement records its unit, configuration, batch size and whether fixed batch
cost was excluded. The gate refuses incomplete attribution and never substitutes one
device for another.

## Gate rules

- Stamp scaling from 2048 to 16384 may increase by at most 1.5× for identical touched area.
- A result beyond 15% of its recorded baseline is a regression.
- Batch measurements must remove fixed batch cost before reporting a per-operation value.
- Coverage counts only passed or failed decisions; informational, unmeasured and unreachable rows are uncovered.
- Resident paint and undo have zero-byte synchronous-readback ceilings; save and export traffic is attributed separately.
- The mobile workload lasts at least twenty minutes and enforces sustained budgets over the final five minutes.
