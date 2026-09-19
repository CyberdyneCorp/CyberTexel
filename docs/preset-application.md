# Applying smart presets

`TextureSet::apply_smart_material()` and `TextureSet::apply_smart_mask()` turn a
validated preset into a texture-set-owned application. The caller supplies a
stable application identity. Every fragment entry is deep-copied and receives
the identity `<application>/<preset-entry>`; parent links, parameter bindings,
anchors, and anchor references are remapped to those instantiated identities.
Applying the same source more than once therefore creates independent graphs,
pixels, parameter state, and entry identities.

Smart-material parameters are reset to their declared defaults before the
fragment is published. `set_applied_preset_parameter_value()` changes every
bound entry in one copied application and publishes it atomically. A smart mask
uses the same independent parameter state and records the layer or group entry
to which it is attached. Missing targets, ineligible targets, duplicate
application identities, and instantiated-entry collisions are refused before
the texture set changes.

## Inspectable entries and origin

`applied_entry()` returns an ordinary `SmartMaterialEntry`. A host can copy it,
edit its graph, opacity, display name, or other authoring data, and publish it
through `replace_applied_entry()`. Replacement validates the entire fragment on
a copy and forbids changing the stable entry identity. `applied_entry_origin()`
remains separate from editable entry data and reports the source preset
identifier and schema version after those edits. Smart masks record their outer
smart-mask schema version, not the embedded smart-material implementation
version.

The application report lists every instantiated entry and includes the complete
smart-material content report. Hosts can therefore identify model-specific
painted pixels before or after applying a mixed preset. Portable resource
identities are preserved without neutral substitution.

## One-step undo

One successful application appends exactly one texture-set preset-history
record, regardless of fragment size. `undo_last_preset_application()` removes
that complete record and every entry it owns. A twelve-entry material therefore
requires one undo, and undoing a mask leaves its target material intact. Refused
applications append no history. Calling undo with no application returns a
report with `removed == false`.
