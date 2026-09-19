# Picker tool

`pick_enabled_channels` turns an existing geometric `HitRecord` into texture
values. The caller supplies one or more texture views, each naming a stable
texture set, a UDIM tile origin, dimensions and the complete ordered set of
enabled channel rasters. The picker resolves exactly one view from the hit's
texture-set identity and UV, applies the engine's top-down pixel layout, and
returns every enabled semantic, component count and value in declared order.

Views are validated before sampling. Empty channel sets, duplicate or empty
semantic IDs, invalid component counts, non-finite pixels, inconsistent raster
sizes, a missing matching view and overlapping matching views are refused.
This makes a missing texture set distinct from a valid pick and prevents an
arbitrary view from winning at a UDIM boundary.

Material selection is explicit rather than inferred from colour values. A view
may provide one non-empty material provenance identity per texel through
`MaterialProvenanceView`; if present, the matching identity is returned with the
channel values. If provenance is omitted, `material_identity` remains empty.
Thus callers that only need sampled values do not pay for or accidentally use
material-selection metadata.
