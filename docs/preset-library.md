# Preset shelves and library

`PresetLibrary` is the unified host-facing catalogue for materials, smart
materials, smart masks, brushes, stroke presets, generators and export presets.
It organises standalone assets into named `PresetShelf` values without parsing
their domain payloads during enumeration.

Each shelf has a stable identifier, display name, a validated project-container
payload and exactly one `PresetShelfEntry` metadata record per asset. Metadata
contains the asset identity, display name, tags and the identity of an embedded
tiled thumbnail. A thumbnail is required and returned as `StoredTiledImage`, so
a host can display the listing without opening the preset payload or consulting
an external path.

`enumerate_preset_shelves()` returns shelves ordered by stable identity with
their display names and preset counts. `enumerate_presets()` returns the entire
catalogue ordered by shelf and preset identity, while
`enumerate_preset_shelf()` limits the result to one named shelf. Each
`PresetListing` reports:

- shelf and preset identifiers;
- preset kind and format version;
- display name and canonically ordered tags; and
- the complete embedded thumbnail.

Preset identities are unique across the entire library. `resolve_preset()` can
therefore resolve one identity without a shelf hint and returns its listing plus
a standalone package containing the selected asset and its dependencies.

Validation refuses duplicate shelf or preset identities, missing or duplicate
metadata, empty or duplicate tags, absent thumbnails, malformed shelf
containers and kinds outside the seven supported shelf categories. Node groups
remain valid standalone assets but are not shelf preset kinds in the current
OpenSpec contract.

## Format versions

Every shelf asset carries a positive `format_version`.
`current_preset_format_version()` exposes the version accepted for each of the
seven kinds: material, smart mask, generator and export preset use version 1;
brushes and stroke presets use the stroke-preset schema version 2; and smart
materials use schema version 6. Older positive versions remain eligible for
their domain loader's migration rules.

Library validation refuses a version newer than the current version before a
preset can be resolved or its destination replaced. The typed
`unsupported_version` diagnostic names the preset kind, declared version and
current version. This policy applies uniformly during enumeration and
resolution, so an unsupported shelf cannot be partially exposed.
