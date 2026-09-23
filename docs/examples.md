# Python examples and fixture assets

Numbered scripts under `examples/` are executable tests written against the
installed `cybertexel` wheel. Each script states the capabilities it covers,
asserts its behavior, and writes deterministic artifacts to the output directory
provided by the runner.

Run the committed-output comparison with:

```sh
just examples
```

`examples/run_all.py` has three modes:

- `assert` runs each script and requires it to succeed and produce output;
- `compare` checks against `examples/outputs/` using the declared tolerance;
- `update` replaces committed CPU-reference outputs after an intentional change.

Updates are refused for non-CPU executors. The runner fixes `PYTHONHASHSEED` and
publishes `CTEX_EXAMPLE_SEED=1729`; stochastic examples must consume that seed.
`examples/output_tolerances.json` declares comparison policy. Reports and
binary artifacts require an exact byte match. PNG previews compare decoded
pixels at their stated maximum absolute channel error, currently zero. See the
[published gallery](gallery.md) for the committed visual results.

## Fixtures

`examples/fixtures/` contains a single-tile UV mesh, a two-tile UDIM mesh, an
8×8 map set, a deterministic brush alpha, a reference image and an OFL-licensed
variable font. `manifest.json` records every asset's SHA-256, origin, category
and licence; `ATTRIBUTION.md` records human-readable provenance. Generated PNGs
can be recreated without third-party packages:

```sh
python3 examples/fixtures/generate_images.py
python3 tools/check_example_fixtures.py
```

The first example, `01_version_and_project_container.py`, exercises the raw
Python C ABI, version synchronization and canonical project-container creation.
`02_image_io.py` demonstrates the public NumPy decode/encode path and produces
the first visual gallery output. `03_mesh_map_generator.py` carries the checked
UV quad and AO fixture through a texture document and deterministic generator.
`04_software_host_execution.py` consumes an emitted WGSL pass plan with a
display-free stand-in and returns a host-resident result without readback.
`05_color_management.py` visualizes the working-space transfer and deterministic
quantization policy.
`06_uv_picking.py` exercises the typed Python UV hit/miss path and publishes the
resolved positions on the fixture topology.
`07_editable_operation_record.py` pins a brush asset in a replayable operation
record and round-trips it through project storage.
`08_brush_stroke.py` resolves timestamped input samples, evaluates geometric
coverage and deposition, and applies the resulting brush through the native
multi-channel paint-tool path.
Full capability example coverage remains roadmap work. Every numbered script
declares a literal `CAPABILITIES` tuple;
`just gate-example-coverage` compares those declarations with
`abi/capi-capabilities.json` and names every missing capability.
