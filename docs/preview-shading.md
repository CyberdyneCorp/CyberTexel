# Preview shading and channel inspection

`ctex/emit/preview_emission.hpp` emits a complete shader and validated pass
plan for a lit material preview or an unlit view of one channel. Both routes
support WGSL, MSL, SPIR-V, and HLSL. `PreviewEmissionCache` retains either
result by complete request content, target, feature set, and display mode.

## Geometry and material inputs

The host supplies non-indexed triangle vertices using one 64-byte stream:

| Location | Semantic | Format | Offset |
| --- | --- | --- | ---: |
| 0 | clip position | `float4` | 0 |
| 1 | world position | `float3` | 16 |
| 2 | world normal | `float3` | 28 |
| 3 | world tangent and handedness | `float4` | 40 |
| 4 | UV | `float2` | 56 |

The lit shader recognizes base colour, opacity, roughness, metallic, tangent
normal, occlusion, emission, and subsurface channels by their `pbr.*` semantic
IDs. Missing channels use the metallic/roughness preset defaults. Height has no
screen-space derivative or displacement interpretation in this preview and is
therefore available through channel inspection only. Custom semantic channels
are also inspectable without becoming implicit shading parameters.

Every inspection shader binds exactly the selected texture and a sampler; it
has no camera, environment, or analytic-light uniform. One-component values are
shown as grayscale, two components as red/green, three as RGB, and four as
RGBA. Values are displayed directly as linear channel data.

## Lighting resource contract

When environment lighting is present, the pass plan declares these bindings
with their view dimension, encoding, and mip convention:

- Environment radiance is a six-face `RGBA16_FLOAT` cube in linear Rec. 709
  radiance. Mip 0 is sharp; successive mips are GGX-prefiltered for increasing
  perceptual roughness. The shader selects
  `roughness * (mip_count - 1)`.
- Diffuse irradiance is a single-mip six-face `RGBA16_FLOAT` cube in linear
  Rec. 709. It stores the cosine-weighted irradiance integral without division
  by pi; the shader applies the Lambertian `1/pi` factor.
- The split-sum specular BRDF lookup is a single-mip `RG16_FLOAT` 2D texture.
  Coordinates are `(NdotV, perceptual_roughness)`; red is the Fresnel scale and
  green the bias.

The uniform block uses 16-byte fields. `camera_position.xyz` is in world space.
`environment_rotation_intensity.x` is a right-handed rotation in radians about
world +Y and `.y` is a non-negative radiance multiplier. Each analytic
directional light consumes two fields: `light_N_direction_intensity.xyz` points
from the surface toward the light and `.w` is intensity;
`light_N_color.xyz` is linear Rec. 709 radiance colour. Up to four lights are
emitted, in declared order.

When the declared device cannot linearly filter a sampled floating-point
texture, emission selects nearest filtering and reports the
`nearest_float_sampling` workaround. Unorm-only inspection remains linearly
filtered because it does not bind unused environment resources.

If the request has no environment resources, no environment texture binding is
required. The same intensity uniform scales a deterministic fallback: ground
RGB `(0.025, 0.025, 0.03)` and sky RGB `(0.12, 0.16, 0.24)` are interpolated by
`clamp(normal.y * 0.5 + 0.5, 0, 1)`. The sky colour also supplies the fallback
specular term. This makes missing environment resources a defined rendering
mode rather than an unbound-resource condition.

## Shading model

All calculations and output RGB are linear Rec. 709. Output alpha is straight
opacity. The metallic/roughness model uses:

- isotropic GGX/Trowbridge-Reitz normal distribution with
  `alpha = perceptual_roughness^2` and a minimum perceptual roughness of 0.045;
- Schlick-GGX direct-light masking-shadowing with
  `k = (perceptual_roughness + 1)^2 / 8`;
- Schlick Fresnel with dielectric F0 `(0.04, 0.04, 0.04)`, linearly blended to
  base colour by metallic;
- energy partition `kD = (1 - F) * (1 - metallic)` and Lambertian diffuse
  `kD * base_color / pi`;
- split-sum image-based specular and the irradiance diffuse convention above;
- ambient occlusion applied to environment lighting, with emission added after
  lighting;
- a documented diffuse-wrap preview for subsurface:
  `clamp((NdotL + subsurface) / (1 + subsurface), 0, 1)`. It does not alter the
  GGX specular lobe.

Normal textures are decoded from `[0,1]` to `[-1,1]` and transformed by the
normalized world normal, tangent, and handedness-derived bitangent. These
equations, constants, resource encodings, and uniform meanings are the complete
contract a host needs to reproduce the preview. Numeric cross-executor
tolerances are established by roadmap task 7.5.
