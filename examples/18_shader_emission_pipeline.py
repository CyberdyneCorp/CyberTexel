#!/usr/bin/env python3
"""Register a host node type and emit every shader a viewport needs from it.

Capabilities: material-graph, shader-emission.
The example teaches the graph a studio node type, holds it to the reference
contract that proves its CPU and emission paths agree, places it in a material,
then emits that material, a layer-stack composite and the lit preview for the
declared targets, reusing one emission cache and reading the pass plan back.
"""

from __future__ import annotations

import argparse
import ctypes
import json
from pathlib import Path

import cybertexel

CAPABILITIES = ("material-graph", "shader-emission")
CAPI = cybertexel.capi
OK, REFUSED = CAPI.CTEX_RESULT_SUCCESS, CAPI.CTEX_RESULT_INVALID_ARGUMENT
SCALAR = CAPI.CTEX_SMART_MATERIAL_VALUE_SCALAR
WGSL, SPIRV = CAPI.CTEX_MATERIAL_GRAPH_TARGET_WGSL, CAPI.CTEX_MATERIAL_GRAPH_TARGET_SPIRV
TARGETS = {"WGSL": WGSL, "MSL": CAPI.CTEX_MATERIAL_GRAPH_TARGET_MSL, "SPIR-V": SPIRV,
           "HLSL": CAPI.CTEX_MATERIAL_GRAPH_TARGET_HLSL}
HOST_TYPE, HOST_TEMPLATE = b"studio.scan.scale", b"(%s * 3.0)"
HOST_IMAGE, OUTPUT_ID = b"library/scan/copper_scan.png", b"gallery/material-output"
RGBA8, FLOAT16 = CAPI.CTEX_EXECUTOR_TEXTURE_RGBA8_UNORM, CAPI.CTEX_EXECUTOR_TEXTURE_RGBA16_FLOAT
RG16 = CAPI.CTEX_EXECUTOR_TEXTURE_RG16_FLOAT
FORMATS = (CAPI.uint32_t * 3)(RGBA8, FLOAT16, RG16)
CALLS = {"cpu": 0, "emit": 0}
EMITS: list[tuple[int, int]] = []
KEEPALIVE: list[object] = []

def ok(result: int) -> None:
    assert result == OK, CAPI.ctex_get_last_diagnostic().decode()

def refused(result: int) -> str:
    assert result == REFUSED, "a request the contract forbids was accepted"
    return CAPI.ctex_get_last_diagnostic().decode()

def buffers(*sizes: int) -> list[object]:
    return [CAPI.create_string_buffer(max(size, 1)) for size in sizes]

def value_of(kind: int, **fields: object) -> object:
    descriptor = CAPI.ctex_smart_material_value_descriptor()
    descriptor.size, descriptor.type = CAPI.CTEX_SMART_MATERIAL_VALUE_DESCRIPTOR_CURRENT_SIZE, kind
    for name, item in fields.items():
        setattr(descriptor, name, item)
    KEEPALIVE.append(descriptor)
    return descriptor

def text_pointers(items: tuple[bytes, ...]) -> object:
    held = [CAPI.create_string_buffer(item) for item in items]
    array = (ctypes.POINTER(ctypes.c_char) * len(held))(
        *(ctypes.cast(item, ctypes.POINTER(ctypes.c_char)) for item in held))
    KEEPALIVE.extend(held)
    return array

def socket(identifier: bytes, default: object = None) -> object:
    descriptor = CAPI.ctex_material_graph_socket_descriptor()
    descriptor.size = CAPI.CTEX_MATERIAL_GRAPH_SOCKET_DESCRIPTOR_CURRENT_SIZE
    descriptor.identifier, descriptor.type = CAPI.String(identifier), SCALAR
    descriptor.display_name = CAPI.String(identifier)
    descriptor.default_value = CAPI.pointer(default) if default is not None else None
    return descriptor

def texture(logical_id: bytes, image_format: int, extent: int, initialized: int,
            layers: int = 1, mip_levels: int = 1) -> object:
    descriptor = CAPI.ctex_shader_texture_descriptor()
    descriptor.size = CAPI.CTEX_SHADER_TEXTURE_DESCRIPTOR_CURRENT_SIZE
    descriptor.logical_id, descriptor.role = CAPI.String(logical_id), CAPI.String(b"fixture")
    descriptor.generation, descriptor.format = 7, image_format
    descriptor.width, descriptor.height = extent, extent
    descriptor.layers, descriptor.mip_levels = layers, mip_levels
    descriptor.tile_width, descriptor.tile_height = 32, 32
    descriptor.externally_initialized = initialized
    return descriptor

def features(binding_budget: int) -> object:
    descriptor = CAPI.ctex_shader_device_features_descriptor()
    descriptor.size = CAPI.CTEX_SHADER_DEVICE_FEATURES_DESCRIPTOR_CURRENT_SIZE
    descriptor.binding_budget, descriptor.maximum_texture_dimension = binding_budget, 4096
    descriptor.supported_texture_formats, descriptor.supported_texture_format_count = FORMATS, 3
    descriptor.floating_point_filtering, descriptor.compute_available = 1, 0
    return descriptor

def register_host_node() -> object:
    """Install a studio node type with both required paths, CPU and emission."""
    registry = CAPI.POINTER(CAPI.ctex_material_graph_node_registry)()
    ok(CAPI.ctex_material_graph_node_registry_create(CAPI.byref(registry)))
    @CAPI.ctex_material_graph_cpu_evaluate_callback
    def evaluate(request: object, outputs: object, _count: int, _user: object) -> int:
        CALLS["cpu"] += 1
        incoming = request.contents
        factor = next(incoming.properties[index].value.scalar
                      for index in range(incoming.property_count)
                      if incoming.properties[index].identifier.data == b"factor")
        outputs[0].type, outputs[0].scalar = SCALAR, incoming.inputs[0].scalar * factor
        return OK
    resources = text_pointers((HOST_IMAGE,))
    @CAPI.ctex_material_graph_emit_callback
    def emit(request: object, out_result: object, _user: object) -> int:
        CALLS["emit"] += 1
        incoming = request.contents
        EMITS.append((incoming.target, incoming.input_expression_count))
        source = ctypes.cast(incoming.input_expressions[0], ctypes.c_char_p).value
        composed = text_pointers((HOST_TEMPLATE % source,))
        out_result.contents.output_expressions = composed
        out_result.contents.output_expression_count = 1
        out_result.contents.resource_identifiers = resources
        out_result.contents.resource_identifier_count = 1
        KEEPALIVE.append(composed)
        return OK
    KEEPALIVE.extend((evaluate, emit, resources))
    properties = (CAPI.ctex_material_graph_property_descriptor * 2)()
    for index, (identifier, default) in enumerate((
            (b"factor", value_of(SCALAR, scalar=3.0)),
            (b"source_image", value_of(CAPI.CTEX_SMART_MATERIAL_VALUE_IMAGE,
                                       text=CAPI.String(HOST_IMAGE))))):
        properties[index].size = CAPI.CTEX_MATERIAL_GRAPH_PROPERTY_DESCRIPTOR_CURRENT_SIZE
        properties[index].identifier = CAPI.String(identifier)
        properties[index].display_name, properties[index].default_value = (
            CAPI.String(identifier), CAPI.pointer(default))
    fixtures = (CAPI.ctex_material_graph_parity_fixture_descriptor * 1)()
    fixtures[0].size = CAPI.CTEX_MATERIAL_GRAPH_PARITY_FIXTURE_DESCRIPTOR_CURRENT_SIZE
    fixtures[0].identifier, fixtures[0].tolerance = CAPI.String(b"positive-scale"), 0.0
    values = CAPI.ctex_smart_material_value_descriptor * 1
    fixtures[0].inputs = values(value_of(SCALAR, scalar=2.0))
    fixtures[0].expected_outputs = values(value_of(SCALAR, scalar=6.0))
    fixtures[0].input_count, fixtures[0].expected_output_count = 1, 1
    registration = CAPI.ctex_material_graph_host_node_registration_descriptor()
    registration.size = CAPI.CTEX_MATERIAL_GRAPH_HOST_NODE_REGISTRATION_DESCRIPTOR_CURRENT_SIZE
    registration.type_id, registration.type_version = CAPI.String(HOST_TYPE), 1
    registration.display_name, registration.deterministic = CAPI.String(b"Scan Scale"), 1
    sockets = CAPI.ctex_material_graph_socket_descriptor * 1
    registration.inputs = sockets(socket(b"value", value_of(SCALAR, scalar=1.0)))
    registration.outputs = sockets(socket(b"result"))
    registration.input_count, registration.output_count = 1, 1
    registration.properties, registration.property_count = properties, 2
    registration.cpu_evaluate, registration.emit = evaluate, emit
    registration.resource_dependencies = text_pointers((b"source_image",))
    registration.resource_dependency_count = 1
    registration.supported_targets = (CAPI.uint32_t * len(TARGETS))(*TARGETS.values())
    registration.supported_target_count = len(TARGETS)
    registration.parity_fixtures, registration.parity_fixture_count = fixtures, 1
    ok(CAPI.ctex_material_graph_node_registry_register(registry, CAPI.byref(registration)))
    registration.type_version = 2
    registration.cpu_evaluate = CAPI.ctex_material_graph_cpu_evaluate_callback()
    message = refused(CAPI.ctex_material_graph_node_registry_register(
        registry, CAPI.byref(registration)))
    assert "CPU evaluation" in message, "an emission-only node type was not refused by name"
    KEEPALIVE.extend((registration, properties, fixtures))
    return registry

def verify_contract(registry: object) -> dict[str, object]:
    """The reference contract runs the parity fixture through both node paths."""
    pinned = text_pointers((b"source_image",))
    info = CAPI.ctex_material_graph_host_contract_info()
    info.size = CAPI.CTEX_MATERIAL_GRAPH_HOST_CONTRACT_INFO_CURRENT_SIZE
    arguments = (registry, CAPI.String(HOST_TYPE), 1, pinned, 1, CAPI.byref(info))
    ok(CAPI.ctex_material_graph_node_registry_verify_contract(*arguments, None, 0))
    report, = buffers(info.report_size)
    ok(CAPI.ctex_material_graph_node_registry_verify_contract(
        *arguments, report, info.report_size))
    contract = json.loads(report.value)
    assert (info.parity_passed, info.parity_failure_count) == (1, 0) and contract["parity_passed"]
    assert (info.replay_eligible, info.unpinned_dependency_count) == (1, 0)
    assert CALLS == {"cpu": 2, "emit": 2 * len(TARGETS)}, "a declared target went unemitted"
    assert sorted(EMITS) == sorted([(target, 1) for target in TARGETS.values()] * 2), (
        "the host node saw an undeclared target or lost its input expression")
    inventory = CAPI.ctex_material_graph_node_registry_info()
    inventory.size = CAPI.CTEX_MATERIAL_GRAPH_NODE_REGISTRY_INFO_CURRENT_SIZE
    ok(CAPI.ctex_material_graph_node_registry_get_info(registry, CAPI.byref(inventory)))
    assert inventory.registration_count == 1, "the refused registration entered the registry"
    return contract

def build_graph(registry: object) -> tuple[bytes, int]:
    """Place the host node in a default graph and let it drive one channel."""
    info = CAPI.ctex_material_graph_info()
    info.size = CAPI.CTEX_MATERIAL_GRAPH_INFO_CURRENT_SIZE
    ok(CAPI.ctex_material_graph_create_default(CAPI.byref(info), None, 0, None, 0))
    graph, = buffers(info.canonical_size)
    ok(CAPI.ctex_material_graph_create_default(
        CAPI.byref(info), graph, info.canonical_size, None, 0))
    output, node = info.output_node_id, CAPI.c_uint64()
    placement = (registry, graph.raw, len(graph.raw), CAPI.String(HOST_TYPE), 1,
                 CAPI.ctex_vec2f(-220.0, 60.0), CAPI.byref(info), CAPI.byref(node))
    ok(CAPI.ctex_material_graph_add_registered_node(*placement, None, 0))
    placed, = buffers(info.canonical_size)
    ok(CAPI.ctex_material_graph_add_registered_node(*placement, placed, info.canonical_size))
    assert info.node_count == 2, "the registered node did not reach the document"
    descriptor = CAPI.ctex_material_graph_link_descriptor(
        CAPI.CTEX_MATERIAL_GRAPH_LINK_DESCRIPTOR_CURRENT_SIZE, node.value, CAPI.String(b"result"),
        output, CAPI.String(b"pbr.roughness"))
    link = CAPI.ctex_material_graph_link_info()
    link.size, blob = CAPI.CTEX_MATERIAL_GRAPH_LINK_INFO_CURRENT_SIZE, placed.raw
    joined = (blob, len(blob), CAPI.byref(descriptor), CAPI.byref(link))
    ok(CAPI.ctex_material_graph_add_link(*joined, None, 0, None, 0))
    linked, = buffers(link.graph.canonical_size)
    ok(CAPI.ctex_material_graph_add_link(*joined, linked, link.graph.canonical_size, None, 0))
    assert link.graph.link_count == 1, "the host node did not reach the material output"
    return linked.raw, node.value

def material_request(target: int) -> object:
    request = CAPI.ctex_shader_material_request()
    request.size = CAPI.CTEX_SHADER_MATERIAL_REQUEST_CURRENT_SIZE
    request.stable_identity = CAPI.String(b"gallery/scan-material")
    request.target, request.features = target, features(16)
    resources = (CAPI.ctex_shader_material_resource_descriptor * 1)()
    resources[0].size = CAPI.CTEX_SHADER_MATERIAL_RESOURCE_DESCRIPTOR_CURRENT_SIZE
    resources[0].identifier, resources[0].texture = (
        CAPI.String(HOST_IMAGE), texture(HOST_IMAGE, RGBA8, 512, 1))
    request.resources, request.resource_count = resources, 1
    request.output = texture(OUTPUT_ID, RGBA8, 64, 0)
    request.requested_filter, request.vertex_count = CAPI.CTEX_SHADER_FILTER_LINEAR, 3
    KEEPALIVE.append(resources)
    return request

def emit_material(registry: object, graph: bytes, target: int) -> tuple:
    info = CAPI.ctex_shader_material_info()
    info.size = CAPI.CTEX_SHADER_MATERIAL_INFO_CURRENT_SIZE
    request = material_request(target)
    arguments = (registry, graph, len(graph), CAPI.byref(request), CAPI.byref(info))
    ok(CAPI.ctex_shader_emit_material(*arguments, None, 0, None, 0, None, 0, None, 0))
    vertex, fragment, plan, notes = buffers(
        info.vertex_artifact_size, info.fragment_artifact_size, info.pass_plan_size,
        info.workaround_report_size)
    ok(CAPI.ctex_shader_emit_material(
        *arguments, vertex, info.vertex_artifact_size, fragment, info.fragment_artifact_size,
        plan, info.pass_plan_size, notes, info.workaround_report_size))
    return (info, vertex.raw[:info.vertex_artifact_size],
            fragment.raw[:info.fragment_artifact_size], json.loads(plan.value))

def emit_every_target(registry: object, graph: bytes) -> dict[str, object]:
    """Emission returns source or bytecode plus the pass plan a host must run."""
    sizes = {}
    for name, target in TARGETS.items():
        info, vertex, fragment, plan = emit_material(registry, graph, target)
        assert (info.target, info.pass_count, info.logical_resource_count) == (target, 1, 2)
        assert (info.fragment_artifact_size > 0) == (name != "MSL"), "only MSL packs one module"
        assert plan["stable_identity"] == "gallery/scan-material"
        stage = plan["passes"][0]
        assert (stage["identifier"], stage["kind"]) == ("material-graph", "render")
        assert (stage["command"]["topology"], stage["command"]["vertex_count"]) == (
            "triangle_list", 3)
        access = {item["resource"]["logical_id"]: item for item in stage["accesses"]}
        assert (access[HOST_IMAGE.decode()]["mode"], access[OUTPUT_ID.decode()]["mode"],
                access[OUTPUT_ID.decode()]["load"]) == ("read", "write", "clear")
        assert len(plan["lifetimes"]) == 2, "a resource lost its declared lifetime"
        if target == SPIRV:
            assert {int.from_bytes(item[:4], "little") for item in (vertex, fragment)} == {
                0x07230203}, "a SPIR-V stage module lost its magic number"
        else:
            assert vertex[-1:] == b"\0", "a text shader artifact was not NUL terminated"
        if target == WGSL:
            assert b" * 3.0)" in fragment, "the host node contributed no emitted source"
        sizes[name] = info.vertex_artifact_size + info.fragment_artifact_size
    unsupported = CAPI.ctex_shader_material_info()
    unsupported.size = CAPI.CTEX_SHADER_MATERIAL_INFO_CURRENT_SIZE
    message = refused(CAPI.ctex_shader_emit_material(
        registry, graph, len(graph), CAPI.byref(material_request(99)),
        CAPI.byref(unsupported), None, 0, None, 0, None, 0, None, 0))
    assert "unknown(99)" in message and "WGSL, MSL, SPIR-V, HLSL" in message
    return {"artifact_bytes": sizes, "target_refusal": message}

def emit_layer_stack(cache: object, budget: int, count: int = 6) -> tuple:
    layers = (CAPI.ctex_shader_layer_descriptor * count)()
    for index in range(count):
        layers[index].size = CAPI.CTEX_SHADER_LAYER_DESCRIPTOR_CURRENT_SIZE
        layers[index].identifier = CAPI.String(f"layer-{index}".encode())
        layers[index].texture = texture(f"source-{index}".encode(), RGBA8, 256, 1)
    request = CAPI.ctex_shader_layer_stack_request()
    request.size = CAPI.CTEX_SHADER_LAYER_STACK_REQUEST_CURRENT_SIZE
    request.stable_identity = CAPI.String(b"gallery/layer-stack")
    request.target, request.features, request.layer_count = WGSL, features(budget), count
    request.layers = layers
    request.output, request.requested_filter = (
        texture(b"gallery/composite", RGBA8, 256, 0), CAPI.CTEX_SHADER_FILTER_LINEAR)
    info = CAPI.ctex_shader_layer_stack_info()
    info.size = CAPI.CTEX_SHADER_LAYER_STACK_INFO_CURRENT_SIZE
    arguments = (cache, CAPI.byref(request), CAPI.byref(info))
    ok(CAPI.ctex_shader_emit_layer_stack(*arguments, None, 0, None, 0, None, 0, None, 0))
    sizing_hit = info.cache_hit
    blob, listing, plan, notes = buffers(
        info.artifact_blob_size, info.artifact_report_size, info.pass_plan_size,
        info.workaround_report_size)
    ok(CAPI.ctex_shader_emit_layer_stack(
        *arguments, blob, info.artifact_blob_size, listing, info.artifact_report_size,
        plan, info.pass_plan_size, notes, info.workaround_report_size))
    return info, sizing_hit, blob.raw, json.loads(listing.value), plan.value.decode()

def composite_layers(cache: object) -> dict[str, object]:
    """A fitting stack is one pass; a tight binding budget carries a composite."""
    info, sizing_hit, blob, listing, plan = emit_layer_stack(cache, 9)
    assert (info.layer_count, info.pass_count, info.workaround_count) == (6, 1, 0)
    assert (sizing_hit, info.cache_hit) == (0, 1), "the first emission came from the cache"
    artifact = listing[0]
    assert (artifact["pass_identifier"], artifact["target"], artifact["encoding"]) == (
        "layer-stack-0", "WGSL", "text")
    assert artifact["vertex_size"] + artifact["fragment_size"] == len(blob)
    assert '"texture_bindings":[' in plan and '"sampler_bindings":[' in plan
    repeat, repeat_sizing, repeat_blob, _, repeat_plan = emit_layer_stack(cache, 9)
    assert (repeat_sizing, repeat.cache_hit) == (1, 1) and (repeat_blob, repeat_plan) == (
        blob, plan), "an unchanged stack missed the cache or changed its artifacts"
    tight, _, _, tight_listing, tight_plan = emit_layer_stack(cache, 4)
    assert (tight.pass_count, len(tight_listing)) == (3, 3), "a tight budget carried no composite"
    assert '"dependencies":["layer-stack-0"]' in tight_plan and "carried composite" in tight_plan
    return {"bytes": len(blob), "layers": info.layer_count, "passes": info.pass_count,
            "split_passes": tight.pass_count}

def preview_request() -> object:
    channels = (CAPI.ctex_shader_preview_channel_descriptor * 4)()
    for index, (semantic, components) in enumerate((
            (b"pbr.base_color", 3), (b"pbr.roughness", 1), (b"pbr.metallic", 1),
            (b"pbr.normal", 3))):
        channels[index].size = CAPI.CTEX_SHADER_PREVIEW_CHANNEL_DESCRIPTOR_CURRENT_SIZE
        channels[index].semantic_id = CAPI.String(semantic)
        channels[index].component_count, channels[index].texture = (
            components, texture(semantic, RGBA8, 256, 1))
    environment = CAPI.ctex_shader_preview_environment_descriptor()
    environment.size = CAPI.CTEX_SHADER_PREVIEW_ENVIRONMENT_DESCRIPTOR_CURRENT_SIZE
    environment.radiance = texture(b"lighting/radiance", FLOAT16, 256, 1, 6, 8)
    environment.diffuse_irradiance = texture(b"lighting/diffuse", FLOAT16, 32, 1, 6, 1)
    environment.specular_brdf_lookup = texture(b"lighting/brdf", RG16, 256, 1)
    request = CAPI.ctex_shader_preview_request()
    request.size = CAPI.CTEX_SHADER_PREVIEW_REQUEST_CURRENT_SIZE
    request.stable_identity = CAPI.String(b"gallery/material-preview")
    request.target, request.features = WGSL, features(16)
    request.channels, request.channel_count = channels, 4
    request.output = texture(b"gallery/preview", RGBA8, 512, 0)
    request.environment = CAPI.pointer(environment)
    request.analytic_light_count, request.vertex_count = 2, 36
    KEEPALIVE.extend((channels, environment))
    return request

def shade_preview(cache: object) -> dict[str, object]:
    """A declared environment adds lighting bindings; dropping it falls back."""
    request = preview_request()
    info = CAPI.ctex_shader_preview_info()
    info.size = CAPI.CTEX_SHADER_PREVIEW_INFO_CURRENT_SIZE
    arguments = (cache, CAPI.byref(request), CAPI.byref(info))
    ok(CAPI.ctex_shader_emit_lit_preview(*arguments, None, 0, None, 0, None, 0, None, 0))
    vertex, fragment, plan, notes = buffers(
        info.vertex_artifact_size, info.fragment_artifact_size, info.pass_plan_size,
        info.workaround_report_size)
    ok(CAPI.ctex_shader_emit_lit_preview(
        *arguments, vertex, info.vertex_artifact_size, fragment, info.fragment_artifact_size,
        plan, info.pass_plan_size, notes, info.workaround_report_size))
    text = plan.value.decode()
    assert (info.kind, info.fallback_lighting, info.pass_count) == (
        CAPI.CTEX_SHADER_PREVIEW_LIT, 0, 1) and info.binding_count > 4
    assert all(token in text for token in (
        "prefiltered environment radiance", "GGX roughness", '"view_dimension":"cube"',
        "split-sum specular BRDF lookup", '"name":"camera_position","offset":0',
        '"name":"light_1_color"')), "the preview plan omitted a declared lighting input"
    lit_bindings = info.binding_count
    request.environment = None
    info.size = CAPI.CTEX_SHADER_PREVIEW_INFO_CURRENT_SIZE
    ok(CAPI.ctex_shader_emit_lit_preview(*arguments, None, 0, None, 0, None, 0, None, 0))
    assert info.fallback_lighting == 1, "a preview without an environment claimed image lighting"
    assert info.binding_count < lit_bindings, "fallback lighting kept the environment bindings"
    return {"bindings": [lit_bindings, info.binding_count], "bytes": len(vertex.raw)}

def cache_counts(cache: object) -> dict[str, int]:
    info = CAPI.ctex_shader_emission_cache_info()
    info.size = CAPI.CTEX_SHADER_EMISSION_CACHE_INFO_CURRENT_SIZE
    ok(CAPI.ctex_shader_emission_cache_get_info(cache, CAPI.byref(info)))
    return {"entries": info.entry_count, "hits": info.hit_count, "misses": info.miss_count}

def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--executor", default="cpu")
    arguments = parser.parse_args()
    registry = register_host_node()
    contract = verify_contract(registry)
    graph, host_node = build_graph(registry)
    emission = emit_every_target(registry, graph)
    cache = CAPI.POINTER(CAPI.ctex_shader_emission_cache)()
    ok(CAPI.ctex_shader_emission_cache_create(CAPI.byref(cache)))
    layers = composite_layers(cache)
    preview = shade_preview(cache)
    cache_report = cache_counts(cache)
    assert min(cache_report.values()) > 0, "the emission cache recorded no traffic"
    ok(CAPI.ctex_shader_emission_cache_clear(cache))
    assert set(cache_counts(cache).values()) == {0}, "clearing left cache state behind"
    CAPI.ctex_shader_emission_cache_destroy(cache)
    CAPI.ctex_material_graph_node_registry_destroy(registry)
    arguments.output.mkdir(parents=True, exist_ok=True)
    summary = {"cache": cache_report, "capabilities": list(CAPABILITIES),
               "emission": emission, "host_contract": contract, "host_node_calls": dict(CALLS),
               "host_node_id": host_node, "layer_stack": layers, "preview": preview}
    (arguments.output / "summary.json").write_text(
        json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8")

if __name__ == "__main__":
    main()
