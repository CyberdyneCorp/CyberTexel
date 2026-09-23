# Paint-tool scenario coverage

`just test-paint-tools-scenarios` runs the headless CTests carrying the
`paint-tools-scenario` label. The matrix test compares this document with the
OpenSpec capability and refuses missing, duplicate, unknown, unregistered or
unlabelled evidence.

Symmetry is expanded once by the canonical stroke resolver before applicable
tools consume coverage, rejection and masking. Tools without a stroke gesture
(Picker, Colour ID and Selection) consume the resulting surface or selection
domain instead of independently reinterpreting symmetry.

| OpenSpec scenario | Executable evidence | Covered behavior |
| --- | --- | --- |
| Symmetry applies to every tool | `stroke-reconstruction`, `paint-brush-eraser`, `paint-fill`, `paint-clone`, `paint-blur-smear` | Mirror/radial expansion produces distinct canonical instances before every stroke-driven tool consumes shared coverage; region tools consume the already-resolved region. |
| Painting a stroke | `paint-brush-eraser` | Brush shades every enabled channel from the selected material along canonical deposited coverage. |
| Erasing on a layer | `paint-brush-eraser` | Eraser proportionally reduces layer opacity and mask values while preserving unrelated texels. |
| Face fill | `paint-fill` | Triangle scope selects exactly the picked triangle identity and shades only its texels. |
| Island fill | `paint-fill` | UV-island scope floods only the cached island containing the picked texel. |
| Aligned clone | `paint-clone` | Aligned mode holds the stroke-start UV offset while sampling along the destination stroke. |
| Cloning across texture sets is refused | `paint-clone` | Cross-set source/destination identities are refused and both sets appear in the diagnostic. |
| Blur does not feed back | `paint-blur-smear` | Separable blur samples the immutable stroke-start snapshot even where the stroke crosses itself. |
| Placing a sticker | `paint-decal-stencil`, `editable-authoring` | A retained decal rotates and re-rasterizes without another surface pick; commit remains explicit. |
| Painting through a stencil | `paint-decal-stencil` | Screen-anchored position, rotation, scale and inversion constrain canonical paint strength. |
| Photo projection | `paint-projection` | Camera projection shades only visible surface samples inside the projection frame. |
| Applying a label | `paint-text` | UTF-8 glyphs from a supplied font are rasterized with requested size, tracking and alignment, then applied through a decal frame. |
| Deterministic spray | `paint-particle` | Identical seeds replay byte-identical particle state and deposition; a different seed changes the result. |
| Reading a surface | `paint-picker` | Picker returns every enabled semantic channel and optionally the producing material identity. |
| Selecting a region | `paint-colour-id` | Tolerance-matched colour-ID selection is reused as paint restriction, mask source and visibility filter. |
| Tolerance of zero | `paint-colour-id` | A filtered map with no exact match reports an empty selection without widening tolerance. |
| Restricting to a region | `paint-selection` | Lasso coverage rejects paint outside the active selection and the selection can be stored as a mask. |
| Out-of-range radius | `stroke-reconstruction`, `c-abi-paint-coverage` | The shared descriptor clamps radius and reports supplied/resolved values; the C ABI exposes the same catalogue and validator used by generated bindings. Binding runtime scenarios remain assigned to task 14.14. |
| Parameter audit | `paint-parameter-audit` | All 69 documented numeric controls match implementation descriptors and named behavioral assertions showing that each resolved value changes output. |
| Editing a painted seam line | `editable-authoring`, `c-abi-editable-authoring` | Moving an attached control point re-evaluates the surface path through stroke-model, invalidates old/new tiles, and undo restores the original path. |
