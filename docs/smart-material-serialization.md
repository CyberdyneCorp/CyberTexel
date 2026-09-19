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
pixels. Application is added by task 13.9.

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
- an optional finite inclusive numeric range; and
- one or more ordered bindings to graph input sockets or node properties.

Ranges require both endpoints, must be ordered, and apply to every component of
a scalar, vector or colour default. Non-numeric parameter types omit ranges.
Duplicate entry or parameter identities, invalid parents, non-finite values,
out-of-range defaults and opacity outside `[0, 1]` are refused before encoding.

Each binding names a stack entry, graph node and input or property by stable
identity. Its target must exist and exactly match the exposed parameter type. A
target can belong to only one exposed parameter, and linked graph inputs cannot
be bound because changing their stored fallback would be inert.

`set_smart_material_parameter_value()` validates the new type and range, applies
it to every bound target on a copy, revalidates the result, and publishes the
copy atomically. Its report lists every updated target. Refused, unknown,
wrong-type and out-of-range updates leave the preset byte-identical. The
declared default remains the reset value; per-instance parameter state arrives
through independent [smart-mask instances](smart-masks.md).

## Canonical format

`serialize_smart_material()` writes `CTEX_SMART_MATERIAL` schema 3. Text and
embedded graph and pixel bytes are hexadecimal, so tabs, line breaks, arbitrary
UTF-8 and binary pixels cannot alter record boundaries. Floating-point values
use their exact IEEE bit patterns. Re-serializing a successfully decoded preset
therefore returns the same bytes.

`deserialize_smart_material()` rejects malformed envelopes, records appearing
out of canonical order, invalid embedded graphs and unsupported schema
versions. Schema migration and documented defaults for older versions belong
to task 13.8; schema 3 is the only accepted version at this stage. Schema 1 was
the definition-only precursor and schema 2 added content classification and
pixels. Both are intentionally refused until that migration is implemented
rather than being partially interpreted.
