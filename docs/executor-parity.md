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
deviation, allowed bound and index. Task 7.6 builds the committed cross-executor
fixture corpus and CI gate on this policy; it will add case and semantic-channel
names around these value-level diagnostics.
