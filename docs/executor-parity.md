# Executor parity policy

The CPU reference defines the correct result. Other executors compare each
normalized channel value with `ctex/exec/parity.hpp`; “visually close” is not a
passing condition.

| Value class | Unfiltered absolute | Unfiltered relative | Filtered absolute | Filtered relative |
|---|---:|---:|---:|---:|
| 8-bit UNORM | `1 / 255` | `0` | `2 / 255` | `0` |
| 16-bit UNORM | `1 / 65535` | `0` | `2 / 65535` | `0` |
| Floating point | `1e-6` | `1e-5` | `5e-6` | `5e-5` |

Integer channels are compared after normalization to `[0, 1]`. One code value
allows the final quantization boundary to differ without hiding a larger error;
filtered values allow two code values because independently implemented texture
filters may round adjacent samples differently.

Floating-point channels use an absolute-plus-relative bound:

```text
abs(reference - measured) <= absolute + relative * max(abs(reference), abs(measured))
```

The absolute term controls values near zero and the relative term scales for HDR
values. Filtered floats receive a separately declared wider bound for device
interpolation and contraction differences. Non-finite values always fail rather
than allowing NaNs to hide drift.

`compare_parity` refuses different value counts, reports the maximum absolute
deviation, and records the first failing value with its reference, measurement,
deviation, allowed bound and index. The cross-executor gate adds fixture and
semantic-channel names around these value-level diagnostics.

## Committed corpus and gate

`tests/fixtures/executor_parity/corpus.hpp` is the versioned corpus. Its three
cases commit all inputs needed for repeatable execution rather than referring to
mutable application state:

- a document extent, tile and channel precision;
- a stroke centre, radius and opacity;
- an OpenGL clip-space camera and viewport;
- a base/layer material and named blend formula; and
- an indexed UV triangle.

The corpus covers unfiltered 8-bit, filtered 16-bit and filtered floating-point
colour, plus depth and coverage. Its CPU renderer uses the production reference
UV rasterizer, shared material blend formulas, and a fixed quarter-texel bilinear
sample for channels declared filtered. These are conformance inputs, not a claim
that the later paint engine and stroke model are already complete; their tasks
extend the corpus with their operation records as those APIs land.

`run_parity_gate` locates exactly one available CPU reference, renders its result,
then measures every other available executor by named fixture and semantic
channel. It rejects malformed/empty corpus data, malformed renderer output,
duplicate executor identities, and an available executor without a renderer. A
drift failure records the fixture, channel, value index, measured deviation and
allowed deviation.

Compiled executors whose device or host attachment is absent are emitted as
`unmeasured`; they are never rendered or counted as passing. `just gate-parity`
runs on GPU-free CI, prints the CPU reference and every unmeasured route, and
will automatically measure an optional backend once that backend is compiled,
available and bound to the corpus renderer.
