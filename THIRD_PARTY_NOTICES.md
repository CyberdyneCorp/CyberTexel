# Third-party notices

CyberTexel is MIT licensed. Every component compiled into a shipped binary must
be permissively licensed (MIT, BSD, Apache-2.0, MPL-2.0, zlib or equivalent);
GPL and LGPL code must not be linked. This is enforced by the dependency audit
gate described in `build-packaging`, whose subject is **what is compiled into a
shipped binary** — including trees vendored outside the manifest and
dependencies fetched at configure time.

## Included dependencies

| Component | Licence | Revision | Role |
|---|---|---|---|
| LodePNG | Zlib | `ed6fe5825c6a4fbb7f58ab35a4231c7543cd452a` | Slice-A PNG decoding and encoding |
| [stb](https://github.com/nothings/stb) | MIT | `2c980bb59875b0d32144a71867fbdebb2f77cd20` | JPEG and TGA memory encoding |
| [Kongruent minikong](https://github.com/armory3d/armorpaint/tree/c5ccdf27818a36e67decb691009a3db457d59ab3/base/sources/kong) | Zlib | `c5ccdf27818a36e67decb691009a3db457d59ab3` | Context-isolated multi-target shader compiler backend |
| [Vulkan-Headers](https://github.com/KhronosGroup/Vulkan-Headers) | MIT | `217e93c664ec6704ec2d8c36fa116c1a4a1e2d40` | Optional owned Vulkan executor API declarations |
| [volk](https://github.com/zeux/volk) | MIT | `f2a16e3e19c2349b873343b2dc38a1d4c25af23a` | Optional dynamic Vulkan entry-point loader |
| [CyberRemesherAndUV](https://github.com/CyberdyneCorp/CyberRemesherAndUV) | MIT | `f3f38bdc93269f90d60564c22422cc80dcd5a016` | Optional bake-provider example only |

The upstream licence texts are copied under `thirdparty/licenses/` for all five
dependencies. ArmorPaint's minikong tree is derived from Kongruent revision
`1b0f3b70673122e3b20701cdb434781a065555d7`; CyberTexel's local changes are
recorded in `thirdparty/kong/CYBERTEXEL_CHANGES.md`.

## Planned

| Component | Licence | Role | Task |
|---|---|---|---|
| Remaining image decoders — to be selected | permissive only | JPEG, TGA, BMP, TIFF, OpenEXR, Radiance HDR, PSD decoding | 2.1 |

Each entry gains its pinned revision and its full licence text here when it is
vendored, recorded from that dependency's own licence file rather than from its
reputation.

## Behavioural references

These are not dependencies. No code, asset or format from them is used.

- **Substance Painter** (Adobe) — behavioural reference for texture sets, mesh
  maps, smart materials, smart masks, anchor points and channel-packing export
  presets, from public documentation only.
- **ArmorPaint** (zlib) — architectural reference for texture-space paint
  rasterization, swept-capsule strokes, stroke-start-snapshot blending, the
  per-stroke coverage mask, extrapolating seam dilation and graph-to-shader-source
  emission. Permissively licensed, so code may additionally be vendored; anything
  vendored appears in the table above.
