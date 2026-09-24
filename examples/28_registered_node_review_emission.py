#!/usr/bin/env python3
"""Take a studio host node through review and emit the shaders review needs.

Capabilities: material-graph, shader-emission.
A studio wraps its edge-wear node in a reusable group, tunes that copy to its own
strength, and hands the material to review. Review is registry aware: it holds
the node to its shipped parity fixture, names the emission target the node never
claimed, and, opened without the studio plug-in, names the type it cannot
resolve. What clears that gate is emitted three ways -- with attribution, from
the cache, and as the unlit channel viewer -- beside the pinned backend note.
"""

from __future__ import annotations

import argparse
import ctypes
import json
from pathlib import Path

import cybertexel

CAPABILITIES = ("material-graph", "shader-emission")
CAPI = cybertexel.capi
byref, pointer, text, buffer = CAPI.byref, CAPI.pointer, CAPI.String, CAPI.create_string_buffer
OK, REFUSED = CAPI.CTEX_RESULT_SUCCESS, CAPI.CTEX_RESULT_INVALID_ARGUMENT
SCALAR, IMAGE = CAPI.CTEX_SMART_MATERIAL_VALUE_SCALAR, CAPI.CTEX_SMART_MATERIAL_VALUE_IMAGE
WGSL, HLSL = CAPI.CTEX_MATERIAL_GRAPH_TARGET_WGSL, CAPI.CTEX_MATERIAL_GRAPH_TARGET_HLSL
GROUP, MATERIAL = CAPI.CTEX_MATERIAL_GRAPH_OWNER_GROUP, CAPI.CTEX_MATERIAL_GRAPH_OWNER_MATERIAL
RGBA8, LINEAR = CAPI.CTEX_EXECUTOR_TEXTURE_RGBA8_UNORM, CAPI.CTEX_SHADER_FILTER_LINEAR
HOST_TYPE, HOST_IMAGE = b"studio.wear.tint", b"library/wear/edge_mask.png"
GROUP_ID, MATERIAL_ID, GROUP_OUTPUT = b"wear-tint", b"hero-armour", b"amount"
SHIPPED_FACTOR, STUDIO_FACTOR = 3.0, 5.0
CHANNELS = ((b"pbr.base_color", 3), (b"pbr.roughness", 1), (b"pbr.metallic", 1))
KEEPALIVE: list[object] = []
EMITTED: list[tuple[int, float]] = []

def ok(result: int) -> None:
    assert result == OK, CAPI.ctex_get_last_diagnostic().decode()

def refused(result: int) -> str:
    assert result == REFUSED, "a request the contract forbids was accepted"
    return CAPI.ctex_get_last_diagnostic().decode()

def fill(name: str, **fields: object) -> object:
    value = getattr(CAPI, f"ctex_{name}")()
    value.size = getattr(CAPI, f"CTEX_{name.upper()}_CURRENT_SIZE")  # the layout we compiled
    for key, item in fields.items():
        setattr(value, key, item)
    KEEPALIVE.append(value)
    return value

def buffers(*sizes: int) -> list[object]:
    return [buffer(max(size, 1)) for size in sizes]

def value_of(kind: int, **fields: object) -> object:
    return fill("smart_material_value_descriptor", type=kind, **fields)

def text_pointers(items: tuple[bytes, ...]) -> object:
    held = [buffer(item) for item in items]
    array = (ctypes.POINTER(ctypes.c_char) * len(held))(
        *(ctypes.cast(item, ctypes.POINTER(ctypes.c_char)) for item in held))
    KEEPALIVE.extend([*held, array])
    return array

def socket(identifier: bytes, default: object = None) -> object:
    return fill("material_graph_socket_descriptor", identifier=text(identifier), type=SCALAR,
                display_name=text(identifier),
                default_value=pointer(default) if default is not None else None)

def texture(logical_id: bytes, extent: int, initialized: int) -> object:
    return fill("shader_texture_descriptor", logical_id=text(logical_id), role=text(b"review"),
                generation=3, format=RGBA8, width=extent, height=extent, layers=1, mip_levels=1,
                tile_width=32, tile_height=32, externally_initialized=initialized)

def features() -> object:
    formats = (CAPI.uint32_t * 1)(RGBA8)
    KEEPALIVE.append(formats)
    return fill("shader_device_features_descriptor", binding_budget=16, compute_available=0,
                maximum_texture_dimension=4096, supported_texture_formats=formats,
                supported_texture_format_count=1, floating_point_filtering=1)

def host_callbacks() -> tuple[object, object]:
    """The two paths every registered node type owes: CPU evaluation and emission."""
    def factor_of(request: object) -> float:
        return next(request.properties[index].value.scalar
                    for index in range(request.property_count)
                    if request.properties[index].identifier.data == b"factor")
    @CAPI.ctex_material_graph_cpu_evaluate_callback
    def evaluate(request: object, outputs: object, _count: int, _user: object) -> int:
        outputs[0].type = SCALAR
        outputs[0].scalar = request.contents.inputs[0].scalar * factor_of(request.contents)
        return OK
    @CAPI.ctex_material_graph_emit_callback
    def emit(request: object, out_result: object, _user: object) -> int:
        incoming, result = request.contents, out_result.contents
        factor = factor_of(incoming)
        EMITTED.append((incoming.node_id, factor))  # never assert inside a callback: record
        source = ctypes.cast(incoming.input_expressions[0], ctypes.c_char_p).value
        result.output_expressions = text_pointers((b"(%s * %.1f)" % (source, factor),))
        result.resource_identifiers = text_pointers((HOST_IMAGE,))
        result.output_expression_count, result.resource_identifier_count = 1, 1
        return OK
    KEEPALIVE.extend((evaluate, emit))
    return evaluate, emit

def register_host_node() -> object:
    """Install the studio type: one scalar input, a strength factor, a mask image."""
    registry = CAPI.POINTER(CAPI.ctex_material_graph_node_registry)()
    ok(CAPI.ctex_material_graph_node_registry_create(byref(registry)))
    properties = (CAPI.ctex_material_graph_property_descriptor * 2)(*(
        fill("material_graph_property_descriptor", identifier=text(name), display_name=text(name),
             default_value=pointer(default))
        for name, default in ((b"factor", value_of(SCALAR, scalar=SHIPPED_FACTOR)),
                              (b"mask_image", value_of(IMAGE, text=text(HOST_IMAGE))))))
    values, sockets = (CAPI.ctex_smart_material_value_descriptor * 1,
                       CAPI.ctex_material_graph_socket_descriptor * 1)
    fixtures = (CAPI.ctex_material_graph_parity_fixture_descriptor * 1)(fill(
        "material_graph_parity_fixture_descriptor", identifier=text(b"shipped-strength"),
        tolerance=0.0, inputs=values(value_of(SCALAR, scalar=2.0)), input_count=1,
        expected_outputs=values(value_of(SCALAR, scalar=2.0 * SHIPPED_FACTOR)),
        expected_output_count=1))
    # The studio ships a WGSL implementation only, and review has to see that.
    entry = fill("material_graph_host_node_registration_descriptor", type_version=1,
                 type_id=text(HOST_TYPE), display_name=text(b"Edge Wear Tint"), deterministic=1,
                 inputs=sockets(socket(b"wear", value_of(SCALAR, scalar=1.0))), input_count=1,
                 outputs=sockets(socket(b"tint")), output_count=1, properties=properties,
                 property_count=2, parity_fixtures=fixtures, parity_fixture_count=1,
                 resource_dependencies=text_pointers((b"mask_image",)), resource_dependency_count=1,
                 supported_targets=(CAPI.uint32_t * 1)(WGSL), supported_target_count=1)
    entry.cpu_evaluate, entry.emit = host_callbacks()
    ok(CAPI.ctex_material_graph_node_registry_register(registry, byref(entry)))
    KEEPALIVE.extend((properties, fixtures))
    return registry

def default_graph() -> tuple[bytes, int]:
    info = fill("material_graph_info")
    ok(CAPI.ctex_material_graph_create_default(byref(info), None, 0, None, 0))
    canonical, = buffers(info.canonical_size)
    ok(CAPI.ctex_material_graph_create_default(byref(info), canonical, info.canonical_size, None, 0))
    return canonical.raw[:info.canonical_size], info.output_node_id

def link(workspace: object, owner: int, identifier: bytes, source: int, source_socket: bytes,
         target: int, target_socket: bytes) -> object:
    descriptor = fill("material_graph_link_descriptor", source_node=source, target_node=target,
                      source_socket=text(source_socket), target_socket=text(target_socket))
    info = fill("material_graph_link_info")
    ok(CAPI.ctex_material_graph_workspace_add_link(
        workspace, owner, text(identifier), byref(descriptor), byref(info), None, 0))
    return info

def author_in_workspace(registry: object) -> tuple[object, dict[str, int]]:
    """Wrap the host node in a reusable group and tune that copy to the studio."""
    workspace = CAPI.POINTER(CAPI.ctex_material_graph_workspace)()
    ok(CAPI.ctex_material_graph_workspace_create(byref(workspace)))
    graph, output_node = default_graph()
    ok(CAPI.ctex_material_graph_workspace_add_material(
        workspace, text(MATERIAL_ID), graph, len(graph)))
    group = fill("material_graph_group_descriptor", identifier=text(GROUP_ID), output_count=1,
                 display_name=text(b"Edge Wear"),
                 outputs=(CAPI.ctex_material_graph_socket_descriptor * 1)(socket(GROUP_OUTPUT)))
    ok(CAPI.ctex_material_graph_workspace_create_group(workspace, byref(group)))
    node, instance = CAPI.c_uint64(), CAPI.c_uint64()
    ok(CAPI.ctex_material_graph_workspace_add_registered_node(
        workspace, registry, GROUP, text(GROUP_ID), text(HOST_TYPE), 1,
        CAPI.ctex_vec2f(-180.0, 40.0), byref(node)))
    inside = fill("material_graph_info")
    ok(CAPI.ctex_material_graph_workspace_get_graph(
        workspace, GROUP, text(GROUP_ID), byref(inside), None, 0, None, 0))
    assert inside.node_count == 3, "the group holds two boundary nodes and the host node"
    assert node.value not in (0, inside.output_node_id), "the placed node has its own identity"
    # The studio copy runs stronger than the default the shipped type declares.
    ok(CAPI.ctex_material_graph_workspace_set_property_value(
        workspace, GROUP, text(GROUP_ID), node.value, text(b"factor"),
        byref(value_of(SCALAR, scalar=STUDIO_FACTOR))))
    message = refused(CAPI.ctex_material_graph_workspace_set_property_value(
        workspace, GROUP, text(GROUP_ID), node.value + 1000, text(b"factor"),
        byref(value_of(SCALAR, scalar=9.0))))
    link(workspace, GROUP, GROUP_ID, node.value, b"tint", inside.output_node_id, GROUP_OUTPUT)
    ok(CAPI.ctex_material_graph_workspace_instantiate_group(
        workspace, text(GROUP_ID), MATERIAL, text(MATERIAL_ID), CAPI.ctex_vec2f(-60.0, 0.0),
        byref(instance)))
    joined = link(workspace, MATERIAL, MATERIAL_ID, instance.value, GROUP_OUTPUT, output_node,
                  b"pbr.roughness")
    assert joined.graph.link_count == 1, "the group instance never reached the material output"
    return workspace, {"host_node": node.value, "instance": instance.value,
                       "output_node": output_node, "unknown_node_refusal": message}

def shipping_graph(registry: object) -> bytes:
    """The document review opens: the same node, left at its shipped default."""
    graph, output_node = default_graph()
    info, node = fill("material_graph_info"), CAPI.c_uint64()
    placement = (registry, graph, len(graph), text(HOST_TYPE), 1, CAPI.ctex_vec2f(-180.0, 40.0),
                 byref(info), byref(node))
    ok(CAPI.ctex_material_graph_add_registered_node(*placement, None, 0))
    placed, = buffers(info.canonical_size)
    ok(CAPI.ctex_material_graph_add_registered_node(*placement, placed, info.canonical_size))
    descriptor = fill("material_graph_link_descriptor", source_node=node.value,
                      source_socket=text(b"tint"), target_node=output_node,
                      target_socket=text(b"pbr.roughness"))
    joint, blob = fill("material_graph_link_info"), placed.raw[:info.canonical_size]
    arguments = (blob, len(blob), byref(descriptor), byref(joint))
    ok(CAPI.ctex_material_graph_add_link(*arguments, None, 0, None, 0))
    linked, = buffers(joint.graph.canonical_size)
    ok(CAPI.ctex_material_graph_add_link(*arguments, linked, joint.graph.canonical_size, None, 0))
    assert (joint.graph.node_count, joint.graph.link_count) == (2, 1)
    return linked.raw[:joint.graph.canonical_size]

def validate(registry: object, graph: bytes, target: int, resources: object) -> tuple[object, dict]:
    info = fill("material_graph_validation_info")
    arguments = (registry, graph, len(graph), target, resources, byref(info))
    ok(CAPI.ctex_material_graph_validate_registered(*arguments, None, 0))
    # The first call publishes the exact report size; the buffer is sized from it.
    report, = buffers(info.report_size)
    ok(CAPI.ctex_material_graph_validate_registered(*arguments, report, info.report_size))
    assert len(report.value) + 1 == info.report_size, "the published report size was not exact"
    return info, json.loads(report.value)

def review_the_material(registry: object, graph: bytes) -> dict[str, object]:
    """Registry-aware validation names the missing target and the opaque type."""
    stocked = byref(fill("material_graph_validation_resources_descriptor", image_resource_count=1,
                         image_resources=text_pointers((HOST_IMAGE,))))
    accepted, report = validate(registry, graph, WGSL, stocked)
    assert (accepted.valid, accepted.error_count, report["valid"]) == (1, 0, True), "unclean"
    rejected, hlsl = validate(registry, graph, HLSL, stocked)
    assert (rejected.valid, rejected.error_count) == (0, 1), "an unclaimed target was accepted"
    unsupported = hlsl["diagnostics"][0]
    assert unsupported["code"] == "unsupported_emission_target"
    assert unsupported["node_type"] == HOST_TYPE.decode() and unsupported["severity"] == "error"
    # The registry runs the shipped fixture through both node paths; without it
    # the fixture, the CPU callback and the determinism claim ride along unread.
    info = CAPI.ctex_material_graph_host_contract_info()
    info.size = CAPI.CTEX_MATERIAL_GRAPH_HOST_CONTRACT_INFO_CURRENT_SIZE
    contract = (registry, CAPI.String(HOST_TYPE), 1, text_pointers((b"mask_image",)), 1, byref(info))
    ok(CAPI.ctex_material_graph_node_registry_verify_contract(*contract, None, 0))
    proof, = buffers(info.report_size)
    ok(CAPI.ctex_material_graph_node_registry_verify_contract(*contract, proof, info.report_size))
    assert (info.parity_passed, info.parity_failure_count) == (1, 0), "the fixture failed"
    assert json.loads(proof.value)["parity_passed"] is True, "the report disagrees"
    assert (info.replay_eligible, info.unpinned_dependency_count) == (1, 0), "not replayable"
    reviewer = CAPI.POINTER(CAPI.ctex_material_graph_node_registry)()
    ok(CAPI.ctex_material_graph_node_registry_create(byref(reviewer)))
    opaque, unknown = validate(reviewer, graph, WGSL, stocked)
    assert (opaque.valid, opaque.error_count) == (0, 1), "an unresolvable node type emitted anyway"
    entry = unknown["diagnostics"][0]
    assert entry["code"] == "missing_node_type" and entry["node_type"] == HOST_TYPE.decode()
    assert entry["subject"] == f"{HOST_TYPE.decode()}@1", "the opaque type lost its version"
    CAPI.ctex_material_graph_node_registry_destroy(reviewer)
    return {"unsupported_target": unsupported["message"], "opaque_type": entry["subject"]}

def material_request(identity: bytes) -> object:
    resources = (CAPI.ctex_shader_material_resource_descriptor * 1)(fill(
        "shader_material_resource_descriptor", identifier=text(HOST_IMAGE),
        texture=texture(HOST_IMAGE, 512, 1)))
    KEEPALIVE.append(resources)
    return fill("shader_material_request", stable_identity=text(identity), target=WGSL,
                features=features(), resources=resources, resource_count=1, vertex_count=3,
                output=texture(b"review/output", 64, 0), requested_filter=LINEAR)

def emit_inspectable(cache: object, registry: object, workspace: object,
                     ids: dict[str, int]) -> tuple[dict[str, object], bytes, bytes]:
    """The workspace material emits attribution that names its whole group path."""
    source = fill("shader_material_source_descriptor", workspace=workspace,
                  kind=CAPI.CTEX_SHADER_MATERIAL_SOURCE_WORKSPACE,
                  material_identifier=text(MATERIAL_ID))
    request, info = material_request(b"review/hero-armour"), fill("shader_material_info")
    debug, hit = fill("shader_material_debug_info"), CAPI.uint32_t()
    emit = CAPI.ctex_shader_emit_material_inspectable
    head = (cache, registry, byref(source), byref(request), byref(info))
    EMITTED.clear()
    ok(emit(*head, None, 0, None, 0, None, 0, None, 0, byref(debug), None, 0, byref(hit)))
    vertex, fragment, plan, notes, metadata = buffers(
        info.vertex_artifact_size, info.fragment_artifact_size, info.pass_plan_size,
        info.workaround_report_size, debug.metadata_size)
    body = (vertex, info.vertex_artifact_size, fragment, info.fragment_artifact_size, plan,
            info.pass_plan_size, notes, info.workaround_report_size, byref(debug))
    ok(emit(*head, *body, metadata, debug.metadata_size, byref(hit)))
    assert len(metadata.value) + 1 == debug.metadata_size, "metadata sizing was not exact"
    assert len(fragment.value) + 1 == info.fragment_artifact_size, "WGSL is NUL-terminated text"
    attribution = json.loads(metadata.value)
    assert (debug.binary_companion, attribution["binary_companion"]) == (0, False)
    named = {item["type_id"]: item for item in attribution["node_attributions"]}
    assert debug.node_attribution_count == len(named) == 2, "the group boundary lost attribution"
    entry, boundary = named[HOST_TYPE.decode()], named["ctex.group-instance"]
    assert (entry["output_socket"], entry["node_id"]) == ("tint", ids["host_node"]), (
        "attribution lost the authored node identity")
    assert (boundary["output_socket"], boundary["node_id"]) == (
        GROUP_OUTPUT.decode(), ids["instance"]), "the group instance was attributed elsewhere"
    assert entry["node_path"].startswith(f"{GROUP_ID.decode()}[{ids['instance']}]/") and (
        entry["node_path"].endswith(f"{HOST_TYPE.decode()}[{ids['host_node']}]")), (
        "a grouped node was attributed without its enclosing group path")
    assert entry["variable_name"].encode() in fragment.value, "the named variable is not emitted"
    # The property edit lives in the workspace: the library carried 5.0 into emission.
    assert set(EMITTED) == {(ids["host_node"], STUDIO_FACTOR)}, "the tuned copy never emitted"
    assert b"* 5.0)" in fragment.value, "the tuned strength never reached the shader"
    assert emit(*head, *body, metadata, debug.metadata_size - 1, byref(hit)) == (
        CAPI.CTEX_RESULT_BUFFER_TOO_SMALL), "a short metadata buffer published a shader anyway"
    return ({"attribution": entry, "bindings": info.binding_count, "passes": info.pass_count},
            metadata.raw[:debug.metadata_size], fragment.raw[:info.fragment_artifact_size])

def emit_cached(cache: object, registry: object, graph: bytes) -> tuple[dict[str, object], bytes]:
    """The shipped document caches by content: two reads, one emission, same bytes."""
    request, info = material_request(b"review/shipped-armour"), fill("shader_material_info")
    hit, emit = CAPI.uint32_t(), CAPI.ctex_shader_emit_material_cached
    def read() -> tuple[int, int, bytes, str]:
        head = (cache, registry, graph, len(graph), byref(request), byref(info))
        ok(emit(*head, None, 0, None, 0, None, 0, None, 0, byref(hit)))
        sizing_hit = hit.value
        vertex, fragment, plan, notes = buffers(
            info.vertex_artifact_size, info.fragment_artifact_size, info.pass_plan_size,
            info.workaround_report_size)
        ok(emit(*head, vertex, info.vertex_artifact_size, fragment, info.fragment_artifact_size,
                plan, info.pass_plan_size, notes, info.workaround_report_size, byref(hit)))
        assert len(plan.value) + 1 == info.pass_plan_size, "the pass plan size was not exact"
        return sizing_hit, hit.value, fragment.raw[:info.fragment_artifact_size], plan.value.decode()
    EMITTED.clear()
    first_sizing, first_hit, fragment, plan = read()
    assert (first_sizing, first_hit) == (0, 1), "a first emission was served from the cache"
    second_sizing, second_hit, repeated, repeat_plan = read()
    assert (second_sizing, second_hit) == (1, 1), "an unchanged document missed the cache"
    assert (fragment, plan) == (repeated, repeat_plan), "a cache hit changed the shader"
    assert len(EMITTED) == 1, "the cached route ran the host node callback twice"
    # The shipped document keeps the declared default; only the workspace copy was tuned.
    assert EMITTED[0][1] == SHIPPED_FACTOR and b"* 3.0)" in fragment
    stage = json.loads(plan)["passes"][0]
    assert (stage["kind"], stage["command"]["vertex_count"]) == ("render", 3)
    return {"cache_hits": [first_sizing, first_hit, second_sizing, second_hit],
            "resources": info.logical_resource_count}, fragment

def inspect_channel(cache: object) -> dict[str, object]:
    """The channel viewer binds one declared channel and no lighting at all."""
    channels = (CAPI.ctex_shader_preview_channel_descriptor * len(CHANNELS))(*(
        fill("shader_preview_channel_descriptor", semantic_id=text(semantic),
             component_count=components, texture=texture(semantic, 256, 1))
        for semantic, components in CHANNELS))
    request = fill("shader_preview_request", stable_identity=text(b"review/channel-viewer"),
                   target=WGSL, features=features(), channels=channels, environment=None,
                   channel_count=len(CHANNELS), analytic_light_count=0, vertex_count=6,
                   output=texture(b"review/viewer", 512, 0))
    info, emit = fill("shader_preview_info"), CAPI.ctex_shader_emit_channel_inspection
    head = (cache, byref(request), text(b"pbr.roughness"), byref(info))
    ok(emit(*head, None, 0, None, 0, None, 0, None, 0))
    vertex, fragment, plan, notes = buffers(
        info.vertex_artifact_size, info.fragment_artifact_size, info.pass_plan_size,
        info.workaround_report_size)
    ok(emit(*head, vertex, info.vertex_artifact_size, fragment, info.fragment_artifact_size,
            plan, info.pass_plan_size, notes, info.workaround_report_size))
    assert len(plan.value) + 1 == info.pass_plan_size, "the pass plan size was not exact"
    assert info.kind == CAPI.CTEX_SHADER_PREVIEW_CHANNEL_INSPECTION
    assert (info.binding_count, info.fallback_lighting) == (2, 0), (
        "the viewer bound more than the inspected channel and its sampler")
    document = json.loads(plan.value)["passes"][0]
    accesses = sorted(item["resource"]["logical_id"] for item in document["accesses"])
    assert accesses == ["pbr.roughness", "review/viewer"], "the viewer read an uninspected channel"
    assert document["uniform_blocks"] == [], "an unlit viewer declared a lighting uniform"
    message = refused(emit(cache, byref(request), text(b"pbr.emission"), byref(info),
                           None, 0, None, 0, None, 0, None, 0))
    assert "pbr.emission" in message, "an undeclared channel was inspected anyway"
    KEEPALIVE.append(channels)
    return {"bindings": info.binding_count, "accesses": accesses, "undeclared_refusal": message}

def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--executor", default="cpu")
    arguments = parser.parse_args()
    registry = register_host_node()
    workspace, ids = author_in_workspace(registry)
    graph = shipping_graph(registry)
    review = review_the_material(registry, graph)
    cache = CAPI.POINTER(CAPI.ctex_shader_emission_cache)()
    ok(CAPI.ctex_shader_emission_cache_create(byref(cache)))
    inspectable, metadata, tuned = emit_inspectable(cache, registry, workspace, ids)
    cached, shipped = emit_cached(cache, registry, graph)
    assert tuned != shipped, "a grouped workspace emitted the same text as a flat graph"
    channel = inspect_channel(cache)
    CAPI.ctex_shader_emission_cache_destroy(cache)
    CAPI.ctex_material_graph_workspace_destroy(workspace)
    CAPI.ctex_material_graph_node_registry_destroy(registry)
    arguments.output.mkdir(parents=True, exist_ok=True)
    (arguments.output / "hero_armour_attribution.json").write_bytes(metadata[:-1] + b"\n")
    (arguments.output / "hero_armour.fragment.wgsl").write_bytes(tuned[:-1])
    summary = {"capabilities": list(CAPABILITIES), "cached": cached,
               "channel_viewer": channel, "inspectable": inspectable, "ids": ids, "review": review}
    (arguments.output / "review_note.json").write_text(
        json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8")

if __name__ == "__main__":
    main()
