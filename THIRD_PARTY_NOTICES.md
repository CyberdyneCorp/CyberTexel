# Third-party notices

CyberTexel is MIT licensed. Every component compiled into a shipped binary must
be permissively licensed (MIT, BSD, Apache-2.0, MPL-2.0, zlib or equivalent);
GPL and LGPL code must not be linked. This is enforced by the dependency audit
gate described in `build-packaging`, whose subject is **what is compiled into a
shipped binary** — including trees vendored outside the manifest and
dependencies fetched at configure time.

No dependency is vendored yet. The table below records what the founding
specification commits to, so the audit has something to check against from the
first commit rather than from the first release.

## Planned

| Component | Licence | Role | Task |
|---|---|---|---|
| [Kong](https://github.com/Kode/Kongruent) (minikong, via [ArmorPaint](https://github.com/armory3d/armorpaint)) | zlib | Shader emission backend: one IR to HLSL, SPIR-V, MSL and WGSL | 6.8 |
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
