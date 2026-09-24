# Demos

Scripts that drive CyberTexel against assets that are **not** in the repository.

They are deliberately not numbered examples. `examples` requires every example to
run on committed fixtures with recorded provenance, and to need no GPU, display
or network — these take a model path and shell out to Blender, so they live here
and are outside the gated suite. Nothing in `just check` or `just examples`
depends on them.

## Painting a model you own

CyberTexel ingests meshes as arrays, not files: the only format the repository
parses is OBJ, through the headless CLI. Anything else is the host's problem, so
these scripts use Blender to get a mesh in and a picture out, and CyberTexel does
the painting in between.

```
just demo-paint ~/path/to/model.fbx ~/path/to/base_color.png
```

The texture is optional. Without it the model is painted from scratch, which
on a model whose canopy and trunk share one UV layout produces a brown tree —
the heuristic has nothing to tell them apart. With it, the paint lands on top
of the material the asset already ships.

That runs three steps:

| Step | Script | What it does |
| --- | --- | --- |
| 1 | `extract_mesh_blender.py` | Blender reads the model and writes positions, normals, UVs and triangles to an `.npz` |
| 2 | `paint_fbx_model.py` | **CyberTexel** rasterizes the surface map, deposits paint and shades it through `ctex_paint_apply_brush` |
| 3 | `render_blender.py` | Blender maps the painted texture back onto the model and renders it |

Results land in `demos/output/`, which is git-ignored because it is generated
from assets the repository does not own:

- `painted_base_color.png` — the painted texture
- `render_painted.png` — the model wearing it
- `summary.json` — mesh size, surface texels, UV coverage, island count

## What the paint step actually proves

Step 2 touches no pixel itself. It rasterizes the model's UV surface map once
through the revision-keyed cache, decides per-texel coverage, and hands the
coverage plus a stroke-start snapshot to `ctex_paint_apply_brush`. Every value in
the output image came back from the library.

Passing `--base-colour` supplies the material the asset already ships as that
stroke-start snapshot, so the paint lands **on top of** it. That is the ordinary
case for a texture painter and it is what the snapshot argument is for.

## Honest limits

The masks are a blunt heuristic over surface normals and height: moss on
upward-facing surface low down, lichen on downward-facing surface higher up. On a
model whose canopy and trunk share one UV layout the lichen follows island seams
and reads as an artifact rather than weathering. Masking by material ID or
painting by hand both produce better results and the engine supports both; the
heuristic is here to keep the demo short, not because it is good art direction.
