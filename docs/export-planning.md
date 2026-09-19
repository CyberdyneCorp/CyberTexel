# Texture export planning

`plan_texture_export()` produces the complete output manifest before encoding
or filesystem work begins. Its source catalogue describes texture sets, their
working dimensions and occupied UDIM tiles, atlas membership, and the ordered
layer tree. Planning is deterministic and performs no I/O, so an invalid scope,
pattern, or collision cannot leave a partial file.

## Texture-set and spatial scopes

Texture-set selection and spatial scope are orthogonal:

- `all` includes every texture set; `selected` requires an explicit, unique
  list of stable texture-set identifiers.
- `texture_set` emits one unit per included texture set.
- `udim_tile` emits one unit per occupied tile of each included set. Tiles are
  ordered numerically and their standard number is carried in both the plan and
  the `{udim}` filename token.
- `atlas` emits one unit per atlas containing included sets. Every included set
  must have exactly one atlas assignment; an omitted or duplicate assignment is
  refused rather than silently dropping content.

The planner records all contributing stable texture-set identifiers. Atlas
outputs use the atlas dimensions and identity; texture-set and UDIM outputs use
the source set's dimensions.

## Layer scopes

Layer scope composes with every texture and spatial scope:

- `flatten_visible` includes layers whose own visibility and every ancestor's
  visibility are enabled.
- `flatten_selected` combines the selected roots into one output. Selecting a
  group includes the group and every descendant, including explicitly hidden
  children because the selection is explicit.
- `each_selected` emits one output per selected root. A selected child beneath
  an already selected group is not emitted a second time; the group output
  already contains it.

Layer selections are keyed by stable texture-set and layer identifiers. Empty,
duplicate, missing, or out-of-scope selections and malformed or cyclic layer
trees produce typed errors.

## Filename patterns

The default pattern is:

```text
{project}_{texture_set}{suffix}_{resolution}_{bit_depth}_{udim}_{layer}.{extension}
```

Patterns can be overridden and support these named tokens:

| Token | Expansion |
| --- | --- |
| `{project}` | Project display name |
| `{texture_set}` | Texture-set display name, or atlas display name for atlas scope |
| `{udim}` | Numeric occupied tile, `single`, or `atlas` |
| `{suffix}` | Preset texture suffix |
| `{resolution}` | A square edge such as `2048`, or `2048x1024` |
| `{bit_depth}` | `8`, `16`, or `32` |
| `{layer}` | `visible`, `selected`, or the separately exported layer name |
| `{extension}` | `png`, `jpg`, `tga`, `tiff`, or `exr` |

Token values are converted to portable ASCII filename atoms: letters, digits,
period, underscore, and hyphen are retained and other bytes become underscores.
Patterns may introduce relative subdirectories with `/`; absolute paths,
parent traversal, empty components, backslashes, drive separators, unmatched
braces, unknown tokens, non-ASCII/control bytes, and platform-reserved filename
characters are refused. Components may not end in a period or space.

Every expanded path is compared using an ASCII case-insensitive key so a plan
that would collide on Windows or a default macOS volume is also refused on a
case-sensitive host. The error names both colliding paths, and no partial plan
is returned.
