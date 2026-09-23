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
- `compare` additionally requires an exact match with `examples/outputs/`;
- `update` replaces committed CPU-reference outputs after an intentional change.

Updates are refused for non-CPU executors. The runner fixes `PYTHONHASHSEED` and
publishes `CTEX_EXAMPLE_SEED=1729`; stochastic examples must consume that seed.
Output tolerances and cross-executor comparison are added with the remaining
example-output and parity tasks.

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

The current first example, `01_version_and_project_container.py`, exercises the
raw Python C ABI, version synchronization and canonical project-container
creation. It commits both the binary container and a JSON summary. More numbered
examples and visual gallery outputs remain roadmap work.
