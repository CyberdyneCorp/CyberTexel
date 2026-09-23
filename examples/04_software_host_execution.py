#!/usr/bin/env python3
"""Execute an emitted host pass plan with a display-free software stand-in.

Capabilities: execution-backends, host-transport, material-graph,
resource-residency, shader-emission.
The stand-in validates the complete render pass, produces its constant material
output in host-owned memory, and publishes the resident generation with recovery.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path

import cybertexel
import numpy as np


CAPABILITIES = (
    "execution-backends",
    "host-transport",
    "material-graph",
    "resource-residency",
    "shader-emission",
)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--executor", default="cpu")
    arguments = parser.parse_args()

    width = height = 64
    program = cybertexel.emit_default_host_material(
        stable_identity="example/software-host",
        output_identity="material-output",
        width=width,
        height=height,
    )
    plan = json.loads(program.pass_plan)
    assert plan["stable_identity"] == "example/software-host"
    assert len(plan["resources"]) == 1 and len(plan["passes"]) == 1
    render_pass = plan["passes"][0]
    target = render_pass["render_targets"][0]
    assert (render_pass["kind"], render_pass["command"]["kind"]) == ("render", "draw")
    assert render_pass["command"]["vertex_count"] == 3
    assert (target["width"], target["height"], target["format"]) == (
        width,
        height,
        "rgba8_unorm",
    )
    assert b"@vertex" in program.vertex_artifact
    assert b"vec4<f32>(5.000000000e-01" in program.fragment_artifact

    pixels = np.full((height, width, 4), [128, 128, 128, 255], dtype=np.uint8)
    output = cybertexel.HostResource(
        "material-output",
        1,
        "material output",
        2,
        width,
        height,
        True,
        externally_initialized=False,
        required_state=cybertexel.ResourceState.RENDER_TARGET,
    )
    with cybertexel.HostExecutionSession() as session:
        token = session.submit("material-graph", 0, [output])
        completion = session.complete(
            token,
            [cybertexel.CompletedResource("material-output", 1, 2, width, height)],
            cybertexel.Recovery("material-graph-v1", 0, pixels.nbytes),
        )
        assert completion.disposition == cybertexel.CompletionDisposition.PUBLISHED
        assert completion.published_revision == 1
        assert session.committed_generation("material-output") == 1
        assert session.resource_is_held("material-output", 1)

    arguments.output.mkdir(parents=True, exist_ok=True)
    (arguments.output / "material_preview.png").write_bytes(cybertexel.encode_image(pixels))
    (arguments.output / "pass_plan.json").write_text(
        json.dumps(plan, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    summary = {
        "capabilities": list(CAPABILITIES),
        "fragment_artifact_bytes": len(program.fragment_artifact),
        "host_resident_bytes": pixels.nbytes,
        "pass_count": len(plan["passes"]),
        "published_revision": completion.published_revision,
        "readback_bytes": 0,
        "vertex_artifact_bytes": len(program.vertex_artifact),
    }
    (arguments.output / "summary.json").write_text(
        json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )


if __name__ == "__main__":
    main()
