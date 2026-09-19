# Standalone asset packages

CyberTexel exports materials, smart materials, smart masks, brushes, stroke
presets, export presets, and node groups through the same versioned container
used by projects. A `StandaloneAsset` carries the domain serializer's opaque
payload and format version plus explicit external-resource and tiled-image
dependencies. The packaging layer does not reinterpret that payload.
[Smart-material payloads](smart-material-serialization.md) use their canonical
domain serializer before entering this container.
[Smart-mask payloads](smart-masks.md) follow the same rule while retaining their
distinct asset kind and schema.

`package_standalone_asset` selects exactly one asset and only its declared
dependencies from a source container. Unknown opaque sections are retained so
an older build does not discard future package data. The default export keeps
external resources as relative references. With `self_contained` enabled,
every referenced resource is read relative to `source_directory` and embedded;
already packed resources and tiled images remain embedded without another
filesystem dependency. A missing declaration, ambiguous identity, or missing
file refuses the package instead of substituting content.

`save_standalone_asset_atomic` builds the complete package before using the
normal atomic project publication path. A packaging failure therefore cannot
replace a valid existing asset file.

`import_standalone_asset` accepts container bytes and the package's containing
directory. It requires exactly one asset, verifies that every declared
dependency is present in the package, resolves referenced resources, and
returns the parsed package with the normal missing-resource report. Its
`self_contained` flag is true only when every declared external resource has an
embedded payload; such a package opens after all source files are removed.

`install_standalone_asset` atomically adds an imported asset to a container used
as a library. Referenced dependencies are packed from their resolved bytes so
their meaning does not change when the library moves. Equal existing resources
or tiled images are reused; conflicting or duplicate identities and missing
dependencies refuse the whole install without partially changing the library.

## Smart-material packages

`package_smart_material` creates the standalone manifest directly from a
validated `SmartMaterialPreset`. Its declared resource identities and kinds are
the package dependencies, and the canonical schema version and payload must
match the manifest. Callers supply `ProjectResource` entries that map those
stable identities to portable relative shelf paths or already packed bytes.
Missing, ambiguous, wrong-kind, absolute, or escaping resources are refused.

Referenced imports accept an ordered list of search roots. The first root that
contains a resource's relative shelf path supplies it, so moving a shelf does
not change the material. `import_smart_material` returns the unchanged preset,
the normal missing-identifier report, and one status for every graph image
input. Missing inputs retain their resource identifier and are marked
`missing`; no neutral content is substituted.

Self-contained export uses the same `self_contained` option as other standalone
assets and embeds every declared resource. Such a package imports with an empty
search-path list and reports every image input as `packed`.
