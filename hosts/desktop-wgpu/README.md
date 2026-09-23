# Desktop WGSL reference host

Runs an emitted CyberTexel pass plan on a real `wgpu` device (Metal, Vulkan,
DX12), reports completion so the library publishes the revision, and drives the
explicit transport path. See [`../README.md`](../README.md) for what a run
proves and for the exit-code contract shared by both hosts.

```
cargo run --release -- [--report PATH] [--measurements PATH]
                      [--benchmark [--frames N]]
```

| Flag | Effect |
| --- | --- |
| `--report PATH` | write the run's JSON report |
| `--measurements PATH` | write a schema-1 measurement run for `device-gate` |
| `--benchmark` | additionally open a window and measure input-to-visible latency |
| `--frames N` | frames the benchmark records, after warm-up (default 600) |

## `--benchmark`

The benchmark opens a window, configures a surface on it, and drives a
synthetic 120 Hz pointer stream at the `interactive-4k` configuration: a
4096-square texture set derived from a 250k-triangle mesh, four enabled
channels and eight visible layers. Each frame is timed in five contiguous
stages — `input`, `upload`, `queue`, `execution`, `presentation` — which
therefore sum to that frame's input-to-visible latency rather than being
reconciled with it afterwards. The loop is paced by the display: it acquires a
drawable before the sample exists and the next one straight after presenting,
so the wait for the display to free the drawable a frame was queued behind is
measured rather than idled through. `tools/device_gate.py::validate_interaction_trace`
is the contract the emitted trace satisfies.

The window stays above the others for the length of the run and the run ends if
the compositor hides it anyway: a window that is not on screen is throttled, and
a throttled frame measures the compositor rather than the host.

`median`, `p95` and `p99` are nearest-rank order statistics, so every reported
figure is a frame that actually ran. The emitted trace is the median frame; a
stage-wise median would sum to a frame that never happened.

### The authored edit

Latency is only meaningful if the edit inside it is the one painting performs.
`Document::write_channel_pixel` is not: it is a convenience that allocates a
whole-canvas coverage buffer and finalizes the preview by decoding,
seam-dilating and re-encoding every texel on the canvas — about **360 ms** for
a single texel at 4096 square, measured by the residency route in the same
binary. A latency figure built on it would be that helper's cost wearing a
frame's name.

The benchmark uses the bounded paint-work surface instead, through the stable
C ABI (`src/authoring.rs`):

1. `ctex_paint_plan_work` reports exactly the storage tiles a stamp footprint
   reaches, dilation included, without visiting the canvas.
2. `ctex_texture_set_begin_transaction` declares that tile set before the edit,
   so history retains exactly those tiles and a write outside them is refused.
3. `ctex_texture_set_transaction_commit` publishes the step.

A 16-texel-radius stamp — about 800 authored texels across one to four storage
tiles — costs roughly **0.2 ms** on the reference device. That is the figure
the `input` stage is built on.

### What the figure includes, and what it does not

* **Input phase.** A 120 Hz pointer stream against a 120 Hz display sits at one
  arbitrary phase for a whole run, and that phase alone moves the result by up
  to a frame interval. The run steps the input clock through the interval in
  eight segments, so the recorded distribution covers every alignment instead
  of whichever one it started at. A sample the loop could not keep up with is
  coalesced to the newest, and its age counts toward the frame's latency.
* **Serialized stages.** The loop waits for the device between stages so each
  one can be attributed. A shipping application pipelines that work, so the
  figure is an upper bound, not a best case.
* **Presentation boundary.** The stage covers presenting the frame and the
  block until the display frees the drawable it was queued behind — the closest
  observable proxy for the frame going out. `wgpu` exposes no scanout timestamp,
  so the remaining wait is bounded rather than measured: the report carries
  `scanout_bound_ms`, the measured figure plus one refresh interval, which is
  the longest that remainder can be. An application that idled between frames
  would always find a free drawable and never see the queue its frame was
  sitting in, which is why this loop does not idle.
* **Upload.** Tiles are published with `write_texture` and the copy is flushed
  and waited on inside the stage. Nothing is mapped and nothing is read back,
  which is why the residency figure beside it is zero bytes of synchronous
  readback rather than a small one.

### When it cannot measure

No display, no window, no adapter that can present to it, a device that cannot
hold a 4096-square texture, a window the compositor hid once recording had
started — a hidden window is a throttled window, and throttled frames are not
an interaction measurement — or a run that ends before it has recorded the
frames it asked for: the host reports the reason, emits `desktop-visible-*` as
`not_measured` rather than approximating it, and exits **3** (unmeasured, never
a pass).
