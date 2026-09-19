# Colour management

CyberTexel's working colour space is **linear Rec. 709**. Colour-valued inputs
entering through sRGB use the standard piecewise sRGB transfer function and are
stored in the linear working space. Outputs requested as sRGB apply the inverse
transfer function. Conversion does not clamp, so negative and high-dynamic-range
values remain representable.

The currently supported declarations are:

| Name | Primaries | Transfer function |
|---|---|---|
| `Linear Rec. 709` | Rec. 709 | Linear |
| `sRGB (Rec. 709 primaries)` | Rec. 709 | sRGB |

Data-valued channel policy, LUTs, precision warnings and deterministic
quantization are part of the same headless module. Roughness, metallic, height,
normal and other data channels never route through a transfer function.

Normal and height storage recommends at least 16 bits per channel; selecting 8
bits remains permitted but produces a structured warning. Derived height values
accumulate in floating point and quantize only once. Eight-bit quantization uses
a deterministic 4×4 ordered dither by default and can be disabled.

Display transforms may load strict 3D `.cube` LUTs. LUT application is a preview
operation and is never part of texture export.
