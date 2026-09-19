# Stroke presets

`ctex/paint/stroke_preset.hpp` defines a named, device-independent wrapper for
the complete `StrokeSettings` contract. `serialize_stroke_preset` writes the
current schema; `deserialize_stroke_preset` validates and migrates supported
older schemas before returning a current `StrokePreset`.

## Canonical format

The canonical representation starts with `CTEX_STROKE_PRESET` and a decimal
schema version, contains one record for every settings group, and ends with
`END`. Record order is fixed: name, base brush properties, stabilizer, the seven
pressure/tilt mappings, jitter, taper, constraint, and symmetry. Arbitrary names
and tip resource identities are hexadecimal byte strings, so UTF-8, tabs,
newlines, and embedded zero bytes are unambiguous. Every `double` is its exact
64-bit IEEE representation in lowercase hexadecimal. Curves retain point order
and exact inputs and outputs.

Serialization validates through `StrokeResolver` and only writes
`current_stroke_preset_schema_version`. Because the serialized preset API has
no clamp-report return channel, settings that would require normalization are
refused rather than silently changed. Empty names, invalid or out-of-range
settings, and attempts to write an older or future in-memory schema are
refused. Re-serializing a successfully loaded current preset produces identical
bytes.

## Versions and migration

The current schema is version 2:

- Version 1 contains every current record but predates flow jitter. Its
  `JITTER` record ends after opacity jitter.
- Version 2 adds flow jitter as the last `JITTER` field.

Loading version 1 starts from documented `StrokeSettings` defaults, parses all
fields present in version 1, leaves flow jitter at its default of zero, validates
the resulting settings, and returns a version-2 preset. No other present field
is changed.

The header is parsed before the version-specific record envelope. A version
greater than this build understands is therefore refused with
`StrokePresetError` naming that numeric version, even if the future schema adds
records this build does not recognize. Parsing constructs a separate result and
returns only after complete validation, so assigning the result cannot partially
modify an existing preset when any header, record, enum, curve, numeric value,
or setting is invalid.

The stroke reconstruction algorithm version remains a field inside
`StrokeSettings`; it is distinct from the preset schema version. The former
selects canonical sample reconstruction semantics, while the latter controls
how settings are encoded and migrated.
