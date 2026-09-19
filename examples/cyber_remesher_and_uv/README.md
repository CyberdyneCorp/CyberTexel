# CyberRemesherAndUV bake provider

This opt-in C++ example adapts CyberRemesherAndUV's stable C ABI to
`ctex::maps::BakeProvider`. It advertises and binds the maps the baker can
produce directly:

- tangent-space normal (OpenGL/+Y convention)
- ambient occlusion (openness)
- curvature
- world-space position
- height (CyberRemesherAndUV displacement)
- vertex colour

The adapter copies the returned float image into CyberTexel's map set, so the
`CyberImage` may be released as soon as the provider callback returns.
Unsupported maps are not advertised and CyberTexel never substitutes a neutral
map.

The dependency is example-only. A normal CyberTexel configure never searches
for or links CyberRemesherAndUV. To build the example against an installed
package:

```sh
cmake -S . -B build/with-cyber-remesher \
  -DCTEX_BUILD_CYBER_REMESHER_EXAMPLE=ON \
  -DCMAKE_PREFIX_PATH=/path/to/cyber-remesher/install
cmake --build build/with-cyber-remesher --target ctex_cyber_remesher_provider_example
```

Run it with an unwrapped low-poly mesh and its high-poly source:

```sh
build/with-cyber-remesher/ctex_cyber_remesher_provider_example low.obj high.obj 1024
```

CyberRemesherAndUV's current `cyber_bake` C entry point is synchronous and has
no progress or cancellation parameters. The adapter therefore checks
cancellation immediately before and after that call and reports progress at
the boundary; it cannot interrupt an in-flight bake. A host requiring bounded
mid-bake cancellation should bind the sibling's C++ baking API or wait for a
cancellable C entry point.
