# CPU layer compositing

`TextureSet::composite_cpu()` is the device-free reference implementation of
document compositing. The texture set remains authoritative for enabled channel
descriptors, stack order, groups, instances, filters, blend modes, opacity and
masks. A `LayerCompositeRequest` supplies dense normalized rasters for content
that is authored or resolved elsewhere; this deliberately does not invent the
tile ownership and undo model scheduled for task 3.9.

Each content raster is keyed by stable entry and channel identities. Paint,
fill, editable decal/text/path, and filter entries accept content. Fill and
editable rasters are the CPU-resolved result of their procedural definition. A
filter raster is its graph's resolved output after reading the target's isolated
input; the compositor applies the filter's own channel state, opacity and blend
mode. Instances never accept copied input: they resolve the original entry's
content dynamically and retain only instance-owned modulation.

Coverage is independent from channel components. Empty coverage means one at
every texel. This allows one-to-four-component extensible channels, including a
real fourth data component, without overloading it as layer coverage. Inputs
must match the texture-set extent and contain finite normalized pixels.

## Order and groups

Root entries and each group's children retain canonical bottom-to-top stack
order. An ordinary group starts with a transparent accumulator, composites its
children, then blends the isolated straight-colour result once into its parent.
A Pass Through group with no enabled filter applies children directly to the
parent accumulator while propagating the group's opacity and masks. An enabled
group filter requires an isolated input; a filtered Pass Through group therefore
publishes its filtered isolated result with Normal group composition.

Masks attached to a layer affect that layer. Masks attached to a group affect
the group result, or the inherited child factor for Pass Through. Disabled masks
are identity. Every active mask needed by an evaluated entry must have one exact
identity-addressed raster.

## Channel policy and coverage

Colour channels use the layer's selected blend formula. Scalar components use
that formula independently. Additive channels use clamped addition. Normal
vectors use the selected RGB formula, decode from `[0,1]` to `[-1,1]`, normalize
with a positive-Z fallback for a zero vector, and encode back to `[0,1]`.

For base coverage `ab`, source coverage after opacity and masks `as`, base value
`Cb`, source value `Cs`, and unweighted blend result `B`, the reference uses:

```text
ao = as + ab * (1 - as)
Co = ((1-as)*ab*Cb + (1-ab)*as*Cs + ab*as*B) / ao
```

The final texture-set accumulator begins at the channel's documented default
with coverage one, so this reduces to the specification's opacity interpolation
for ordinary layers. Isolated group accumulators begin at zero coverage.

The compositor validates the complete request before returning a result and
reports typed errors for duplicate, unknown, missing, malformed, or unevaluable
inputs. Enabled output channels are emitted in stable semantic-identity order.
The determinism gate runs the same nested composite twice and byte-compares the
serialized dimensions, identities, component counts and float pixels.
