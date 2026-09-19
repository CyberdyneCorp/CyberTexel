# Smart material serialization

`SmartMaterialPreset` is the device-independent, filesystem-independent payload
for a reusable smart material. It contains an ordered stack fragment and the
parameters a host should expose. This is the portable definition, not an
instantiated texture-set layer stack.

## Stack fragment

Every `SmartMaterialEntry` has a stable identity, optional parent identity,
display name, kind, enabled state, opacity and optional `GraphDocument`. The
five entry kinds are layer, group, mask, filter and generator. Their vector
order is retained because it is part of compositing semantics. A parent must
already occur in that order, which makes the fragment a deterministic acyclic
hierarchy and prevents ambiguous forward ownership.

Graphs use the same canonical graph serializer as ordinary materials. Each
entry explicitly declares `derived` or `model_specific` content:

- derived entries retain generator and graph definitions but cannot contain
  rasterized output, so a target model can evaluate them from its own maps;
- model-specific entries are painted layers or masks and contain one or more
  named, tightly packed pixel payloads with dimensions and pixel format.

Model-specific content cannot be attached to a group, filter or generator, and
its byte count must exactly match its declared dimensions and format. This
prevents cached generator output from being mistaken for portable authored
pixels. Parameter bindings and application are added by tasks 13.3 and 13.9.

## Content report

`report_smart_material_content()` returns one ordered record per stack entry,
including its content kind, pixel-payload count and stored byte count. It also
reports aggregate derived and model-specific entry counts and the total stored
model-specific bytes. A host can therefore warn that painted content was made
for another model before applying it. The application report added in task
13.9 will carry this information into the one-step application operation.

## Exposed parameters

An `ExposedSmartMaterialParameter` declares:

- a stable identifier and display name;
- a non-empty display group;
- one of the graph socket types: scalar, vector, colour, string, image or
  boolean;
- a default whose value exactly matches that type; and
- an optional finite inclusive numeric range.

Ranges require both endpoints, must be ordered, and apply to every component of
a scalar, vector or colour default. Non-numeric parameter types omit ranges.
Duplicate entry or parameter identities, invalid parents, non-finite values,
out-of-range defaults and opacity outside `[0, 1]` are refused before encoding.

## Canonical format

`serialize_smart_material()` writes `CTEX_SMART_MATERIAL` schema 2. Text and
embedded graph and pixel bytes are hexadecimal, so tabs, line breaks, arbitrary
UTF-8 and binary pixels cannot alter record boundaries. Floating-point values
use their exact IEEE bit patterns. Re-serializing a successfully decoded preset
therefore returns the same bytes.

`deserialize_smart_material()` rejects malformed envelopes, records appearing
out of canonical order, invalid embedded graphs and unsupported schema
versions. Schema migration and documented defaults for older versions belong
to task 13.8; schema 2 is the only accepted version at this stage. Schema 1 was
the definition-only precursor and is intentionally refused until that migration
is implemented rather than being partially interpreted.
