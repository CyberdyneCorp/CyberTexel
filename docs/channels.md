# Channel descriptors

Document channels are registered by stable semantic identifiers rather than
fixed ABI slots. A descriptor records component count, normalized or floating
representation, preferred precision, default value, colour/data classification,
blending policy, export mapping and whether this build can evaluate it.

The built-in metallic/roughness preset contains base colour, opacity, roughness,
metallic, normal, height, occlusion, emission and subsurface descriptors. It is
a preset, not a limit: custom identifiers such as `openpbr.coat_weight` are
stored and enumerated even when their semantics are not yet evaluable.

A texture set supplies an 8-, 16- or 32-bit default. Enabling a channel may
override that precision independently. Disabled channels have no `TiledImage`
and therefore no pixel storage. Enabling one creates sparse storage initialized
from the descriptor's constant default, without touching existing channels.
