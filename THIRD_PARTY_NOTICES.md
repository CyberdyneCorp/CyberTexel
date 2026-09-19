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
| [Kongruent minikong](https://github.com/armory3d/armorpaint/tree/c5ccdf27818a36e67decb691009a3db457d59ab3/base/sources/kong) | Zlib | `c5ccdf27818a36e67decb691009a3db457d59ab3` | Context-isolated multi-target shader compiler backend |

The upstream licence texts are copied at `thirdparty/licenses/lodepng.txt` and
`thirdparty/licenses/kongruent.txt`. ArmorPaint's minikong tree is derived from
Kongruent revision `1b0f3b70673122e3b20701cdb434781a065555d7`; CyberTexel's
local changes are recorded in `thirdparty/kong/CYBERTEXEL_CHANGES.md`.

## Planned

| Component | Licence | Role | Task |
|---|---|---|---|
| Image decoders and encoders — to be selected | permissive only | PNG, JPEG, TGA, BMP, TIFF, OpenEXR, Radiance HDR, PSD | 2.1 |

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
