# bootstrap-v1-cybertexel

Greenfield v1: a portable C++20 3D texture-painting and PBR material-authoring
engine — layer stack, texture-space paint rasterization, node-graph materials
compiled to shader source, smart materials and channel-packing export — with a
headless CLI, Python-driven examples that double as the project's end-to-end
check, and Python, Swift and Rust bindings. Consumes UVs and baked maps from
CyberRemesherAndUV and owns no GPU device in the host-executed path. Logical
resource versions stay on the host GPU, with explicit completion and recovery
contracts. Desktop/mobile painting and resource budgets are validated before
catalogue breadth; replayable strokes and persistent editable entries preserve
future editing options. See `tasks.md` for the four delivery slices.
