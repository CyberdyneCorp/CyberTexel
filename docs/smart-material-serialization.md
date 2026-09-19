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
pixels. [Preset application](preset-application.md) deep-copies both forms into
an independently editable texture-set fragment.

## Content report

`report_smart_material_content()` returns one ordered record per stack entry,
including its content kind, pixel-payload count and stored byte count. It also
reports aggregate derived and model-specific entry counts and the total stored
model-specific bytes. A host can therefore warn that painted content was made
for another model before applying it. The one-step application report carries
the same ordered content evidence into the instantiated fragment.

## Exposed parameters

An `ExposedSmartMaterialParameter` declares:

- a stable identifier and display name;
- a non-empty display group;
- one of the graph socket types: scalar, vector, colour, string, image or
  boolean;
- a default whose value exactly matches that type; and
- an optional finite inclusive numeric range; and
- a binding state; and
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

Schema 1 and 2 parameters predate stored binding targets. Migration preserves
their identity, display metadata, type, default and range with the
`legacy_unbound` binding state and no invented target. Such declarations are
read-only: an attempted update returns `read_only_parameter` and leaves the
preset unchanged. Current authored parameters use `bound` and require at least
one live binding, preserving the no-inert-parameter rule.

## Anchor points

A layer or mask can be listed in `anchor_entries`, making its composited output
available to later stack entries. Each `SmartMaterialAnchorReference` binds one
anchor to a concrete image input in a consuming layer's graph. The input must
exist, remain unlinked, and cannot also be controlled by an exposed parameter;
these checks prevent an accepted reference from being inert.

Stack order runs from lower to upper entries. A consumer must therefore occur
after its anchor. References that point downward are refused with
`anchor_ordering_violation`. References form a directed dependency graph, and a
reference that closes a cycle is refused atomically with `anchor_cycle` and the
complete cycle path in its diagnostic.

`plan_smart_material_anchor_evaluation()` accepts the identities of anchors
whose composited output changed. It walks transitive dependencies and returns
only affected consumers, in stack order. Because valid dependencies always
point upward, this is also dependency order. Unrelated layers are excluded, so
an anchor edit does not force whole-material evaluation.

## Portable resources

`resource_references` declares every external image, font, or other resource by
a stable identifier and kind. Identifiers are portable names, not filesystem
locations: absolute paths, drive-qualified names, empty components, and `.` or
`..` traversal are refused. Every non-empty image value in an embedded graph or
image parameter default must name a declared resource of kind `image`.

The IO-layer [smart-material package API](standalone-assets.md#smart-material-packages)
maps those identities to relative shelf paths. Resolution checks ordered search
roots without changing the serialized identity. Each image input is reported as
packed, resolved from a search path, or missing. A missing input retains its
identifier and is never filled with a neutral substitute.

## Canonical format

`serialize_smart_material()` writes `CTEX_SMART_MATERIAL` schema 6. Text and
embedded graph and pixel bytes are hexadecimal, so tabs, line breaks, arbitrary
UTF-8 and binary pixels cannot alter record boundaries. Floating-point values
use their exact IEEE bit patterns. Re-serializing a successfully decoded preset
therefore returns the same bytes.

`deserialize_smart_material()` rejects malformed envelopes, records appearing
out of canonical order and invalid embedded graphs. It migrates schemas 1–5 to
schema 6 before validation:

- schema 1 entries default to `derived` content with no pixel payloads;
- schema 1–2 parameters become read-only `legacy_unbound` declarations;
- schemas before 4 default to no anchors or anchor references;
- schemas before 5 infer sorted `image` resource declarations from non-empty
  graph image values and image parameter defaults; and
- schemas 3–5 parameters default to the `bound` state.

Version zero and versions newer than 6 are refused with
`unsupported_version`, and the diagnostic names the declared number. Parsing
and migration occur in a temporary value, so a caller assigning the result
cannot observe a partially applied future or malformed preset.
