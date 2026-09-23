#!/usr/bin/env python3
"""Author a material graph, publish it, and share a node group across a workspace.

Capabilities: material-graph.
The example browses the built-in node catalogue, grows the default document into
a copper material, validates it against the resources a project owns, stores it
under a stable library identity, and propagates one reusable node group to every
material in a workspace without losing the values each instance already carried.
"""

from __future__ import annotations

import argparse
import ctypes
import json
from pathlib import Path

import cybertexel

CAPABILITIES = ("material-graph",)
CAPI = cybertexel.capi
OK = CAPI.CTEX_RESULT_SUCCESS
REFUSED = CAPI.CTEX_RESULT_INVALID_ARGUMENT
MATERIAL = CAPI.CTEX_MATERIAL_GRAPH_OWNER_MATERIAL
GROUP = CAPI.CTEX_MATERIAL_GRAPH_OWNER_GROUP
SCALAR = CAPI.CTEX_SMART_MATERIAL_VALUE_SCALAR
PLATE_IMAGE = b"library/copper/plate_albedo.png"
MATERIALS = (b"oxidised", b"polished", b"brushed")

def diagnostic() -> str:
    return CAPI.ctex_get_last_diagnostic().decode()

def ok(result: int) -> None:
    assert result == OK, diagnostic()

def refused(result: int) -> str:
    assert result == REFUSED, "an edit the contract forbids was accepted"
    return diagnostic()

def document(invoke: object) -> tuple[bytes, str, object]:
    """Drive one graph call through the two-call sizing contract."""
    info = CAPI.ctex_material_graph_info()
    info.size = CAPI.CTEX_MATERIAL_GRAPH_INFO_CURRENT_SIZE
    ok(invoke(info, None, 0, None, 0))
    canonical = CAPI.create_string_buffer(info.canonical_size)
    report = CAPI.create_string_buffer(max(info.report_size, 1))
    ok(invoke(info, canonical, info.canonical_size, report, info.report_size))
    return canonical.raw, report.value.decode(), info

def value_of(kind: int, **fields: object) -> object:
    descriptor = CAPI.ctex_smart_material_value_descriptor()
    descriptor.size = CAPI.CTEX_SMART_MATERIAL_VALUE_DESCRIPTOR_CURRENT_SIZE
    descriptor.type = kind
    for name, item in fields.items():
        setattr(descriptor, name, item)
    return descriptor

def text_pointers(items: tuple[bytes, ...]) -> tuple[object, list[object]]:
    held = [CAPI.create_string_buffer(item) for item in items]
    array = (ctypes.POINTER(ctypes.c_char) * len(held))(
        *(ctypes.cast(item, ctypes.POINTER(ctypes.c_char)) for item in held))
    return array, held

def socket(identifier: bytes, default: object = None) -> object:
    descriptor = CAPI.ctex_material_graph_socket_descriptor()
    descriptor.size = CAPI.CTEX_MATERIAL_GRAPH_SOCKET_DESCRIPTOR_CURRENT_SIZE
    descriptor.identifier = CAPI.String(identifier)
    descriptor.display_name = CAPI.String(identifier)
    descriptor.type = SCALAR
    descriptor.default_value = CAPI.pointer(default) if default is not None else None
    return descriptor

def inspect(graph: bytes) -> tuple[dict[str, object], bytes, object]:
    canonical, report, info = document(
        lambda i, c, cs, r, rs: CAPI.ctex_material_graph_inspect(
            graph, len(graph), CAPI.byref(i), c, cs, r, rs))
    return json.loads(report), canonical, info

def add_node(graph: bytes, type_id: bytes, position: tuple[float, float]) -> tuple[bytes, int]:
    node = CAPI.c_uint64()
    canonical, _, _ = document(
        lambda i, c, cs, r, rs: CAPI.ctex_material_graph_add_builtin_node(
            graph, len(graph), CAPI.String(type_id), CAPI.ctex_vec2f(*position),
            CAPI.byref(i), CAPI.byref(node), c, cs))
    return canonical, node.value

def set_value(graph: bytes, setter: object, node: int, identifier: bytes, value: object) -> bytes:
    canonical, _, _ = document(
        lambda i, c, cs, r, rs: setter(
            graph, len(graph), node, CAPI.String(identifier), CAPI.byref(value),
            CAPI.byref(i), c, cs))
    return canonical

def link_descriptor(source: tuple[int, bytes], target: tuple[int, bytes]) -> object:
    return CAPI.ctex_material_graph_link_descriptor(
        CAPI.CTEX_MATERIAL_GRAPH_LINK_DESCRIPTOR_CURRENT_SIZE,
        source[0], CAPI.String(source[1]), target[0], CAPI.String(target[1]))

def link(graph: bytes, source: tuple[int, bytes], target: tuple[int, bytes]) -> tuple:
    """Add one link, reporting the coercion and any link it displaced."""
    descriptor = link_descriptor(source, target)
    info = CAPI.ctex_material_graph_link_info()
    info.size = CAPI.CTEX_MATERIAL_GRAPH_LINK_INFO_CURRENT_SIZE
    ok(CAPI.ctex_material_graph_add_link(
        graph, len(graph), CAPI.byref(descriptor), CAPI.byref(info), None, 0, None, 0))
    canonical = CAPI.create_string_buffer(info.graph.canonical_size)
    replaced = CAPI.create_string_buffer(max(info.replaced_source_socket_size, 1))
    ok(CAPI.ctex_material_graph_add_link(
        graph, len(graph), CAPI.byref(descriptor), CAPI.byref(info), canonical,
        info.graph.canonical_size, replaced, info.replaced_source_socket_size))
    name = replaced.value.decode() if info.replaced_source_socket_size else None
    return info, canonical.raw, name

def graphs_equal(left: bytes, right: bytes) -> bool:
    equal = CAPI.c_uint32()
    ok(CAPI.ctex_material_graph_compare(left, len(left), right, len(right), CAPI.byref(equal)))
    return equal.value == 1

def browse_catalogue() -> dict[str, object]:
    """Read the shipped node inventory an author picks node types from."""
    info = CAPI.ctex_material_graph_catalogue_info()
    info.size = CAPI.CTEX_MATERIAL_GRAPH_CATALOGUE_INFO_CURRENT_SIZE
    ok(CAPI.ctex_material_graph_get_builtin_catalogue(CAPI.byref(info), None, 0))
    sentinel = CAPI.create_string_buffer(b"x")
    assert CAPI.ctex_material_graph_get_builtin_catalogue(
        CAPI.byref(info), sentinel, 0) == CAPI.CTEX_RESULT_BUFFER_TOO_SMALL
    assert sentinel.raw[0:1] == b"x", "an undersized catalogue output was not atomic"
    report = CAPI.create_string_buffer(info.report_size)
    ok(CAPI.ctex_material_graph_get_builtin_catalogue(CAPI.byref(info), report, info.report_size))
    catalogue = json.loads(report.value)
    nodes = catalogue["nodes"]
    assert (info.node_count, len(nodes)) == (52, 52)
    assert (info.input_node_count, info.texture_node_count) == (10, 10)
    assert (info.colour_filter_node_count, info.vector_math_node_count) == (18, 14)
    assert (info.math_operation_count, info.vector_math_operation_count) == (40, 27)
    by_type = {node["type_id"]: node for node in nodes}
    assert len(by_type) == 52, "the catalogue repeated a node type"
    modes = by_type["ctex.colour.blend"]["properties"][0]
    assert (modes["id"], modes["default"]) == ("mode", "normal")
    assert modes["allowed_values"][:2] == ["normal", "darken"]
    assert "add" in by_type["ctex.math.scalar"]["properties"][0]["allowed_values"]
    assert catalogue["math_operations"][0]["formula"] == "a + b"
    return {"categories": sorted({node["category"] for node in nodes}),
            "choices": modes["allowed_values"], "type_count": len(by_type)}

def author_copper_graph() -> tuple[bytes, dict[str, int], str]:
    """Grow the default output-only document into a scratched copper material."""
    graph, report, info = document(
        lambda i, c, cs, r, rs: CAPI.ctex_material_graph_create_default(
            CAPI.byref(i), c, cs, r, rs))
    assert (info.node_count, info.link_count) == (1, 0)
    assert (info.output_node_id, info.output_channel_count) == (1, 9)
    default = json.loads(report)["nodes"][0]
    assert default["role"] == "output"
    assert {item["id"] for item in default["inputs"]} >= {"pbr.base_color", "pbr.roughness"}
    output = info.output_node_id
    graph, plate = add_node(graph, b"ctex.texture.image", (-320.0, 40.0))
    graph = set_value(graph, CAPI.ctex_material_graph_set_input_value, plate, b"vector",
                      value_of(CAPI.CTEX_SMART_MATERIAL_VALUE_VECTOR,
                               vector=CAPI.ctex_vec3f(2.0, 2.0, 0.0)))
    graph = set_value(graph, CAPI.ctex_material_graph_set_property_value, plate, b"image",
                      value_of(CAPI.CTEX_SMART_MATERIAL_VALUE_IMAGE,
                               text=CAPI.String(PLATE_IMAGE)))
    colour, graph, replaced = link(graph, (plate, b"colour"), (output, b"pbr.base_color"))
    assert colour.coercion == CAPI.CTEX_MATERIAL_GRAPH_COERCION_IDENTITY
    assert (colour.replaced, replaced, colour.graph.link_count) == (0, None, 1)
    graph, scratch = add_node(graph, b"ctex.input.constant-value", (-320.0, -120.0))
    _, graph, _ = link(graph, (scratch, b"value"), (output, b"pbr.roughness"))
    graph, tint = add_node(graph, b"ctex.input.constant-colour", (-320.0, -260.0))
    again, graph, replaced = link(graph, (tint, b"colour"), (output, b"pbr.roughness"))
    assert again.coercion == CAPI.CTEX_MATERIAL_GRAPH_COERCION_COLOUR_TO_SCALAR
    assert (again.replaced, again.replaced_source_node) == (1, scratch)
    assert replaced == "value", "the displaced link did not name its own source socket"
    assert again.graph.link_count == 2, "a replacement must not grow the link count"
    probe, blur = add_node(graph, b"ctex.filter.blur", (-140.0, -420.0))
    rejected = link_descriptor((blur, b"image"), (output, b"pbr.roughness"))
    link_info = CAPI.ctex_material_graph_link_info()
    link_info.size = CAPI.CTEX_MATERIAL_GRAPH_LINK_INFO_CURRENT_SIZE
    untouched = b"\xff" * len(probe)
    sentinel = CAPI.create_string_buffer(untouched)
    refusal = refused(CAPI.ctex_material_graph_add_link(
        probe, len(probe), CAPI.byref(rejected), CAPI.byref(link_info),
        sentinel, len(probe), None, 0))
    assert sentinel.raw[:len(probe)] == untouched, "a refused link still wrote a document"
    assert CAPI.ctex_get_last_diagnostic_code() == CAPI.CTEX_DIAGNOSTIC_INVALID_MATERIAL_GRAPH
    assert "image" in refusal and "scalar" in refusal, "the refusal named neither socket type"
    graph, blend = add_node(graph, b"ctex.colour.blend", (-140.0, 220.0))
    graph = set_value(graph, CAPI.ctex_material_graph_set_property_value, blend, b"mode",
                      value_of(CAPI.CTEX_SMART_MATERIAL_VALUE_STRING,
                               text=CAPI.String(b"darken")))
    stored, canonical, inspected = inspect(graph)
    assert canonical == graph, "inspection rewrote the canonical document"
    assert (inspected.node_count, inspected.link_count) == (5, 2)
    nodes = {node["id"]: node for node in stored["nodes"]}
    assert nodes[blend]["properties"][0]["value"] == "darken"
    assert nodes[plate]["properties"][0]["value"] == PLATE_IMAGE.decode()
    assert not graphs_equal(graph, probe), "the blur probe was indistinguishable from the material"
    return graph, {"blend": blend, "output": output, "plate": plate, "scratch": scratch}, refusal

def validate(graph: bytes, resources: object) -> tuple[object, list[dict[str, object]]]:
    info = CAPI.ctex_material_graph_validation_info()
    info.size = CAPI.CTEX_MATERIAL_GRAPH_VALIDATION_INFO_CURRENT_SIZE
    ok(CAPI.ctex_material_graph_validate(graph, len(graph), resources, CAPI.byref(info), None, 0))
    report = CAPI.create_string_buffer(info.report_size)
    ok(CAPI.ctex_material_graph_validate(
        graph, len(graph), resources, CAPI.byref(info), report, info.report_size))
    return info, json.loads(report.value)["diagnostics"]

def check_resources(graph: bytes) -> list[dict[str, object]]:
    """Validation names what the project does not own, then clears once it does."""
    missing, diagnostics = validate(graph, None)
    assert (missing.valid, missing.error_count, missing.warning_count) == (0, 1, 2)
    errors = [item for item in diagnostics if item["severity"] == "error"]
    assert errors[0]["code"] == "missing_image_resource"
    assert errors[0]["subject"] == PLATE_IMAGE.decode()
    warnings = [item for item in diagnostics if item["severity"] == "warning"]
    assert {item["code"] for item in warnings} == {"unreachable_node"}
    assert {item["subject"] for item in warnings} == {"Blend", "Constant Value"}
    images, _held = text_pointers((PLATE_IMAGE,))
    descriptor = CAPI.ctex_material_graph_validation_resources_descriptor()
    descriptor.size = CAPI.CTEX_MATERIAL_GRAPH_VALIDATION_RESOURCES_DESCRIPTOR_CURRENT_SIZE
    descriptor.image_resources = images
    descriptor.image_resource_count = 1
    supplied, supplied_diagnostics = validate(graph, CAPI.byref(descriptor))
    assert supplied.error_count == 0, "an owned image resource was still reported missing"
    assert supplied.valid == 1, "a warning must not invalidate a graph"
    assert {item["code"] for item in supplied_diagnostics} == {"unreachable_node"}
    return diagnostics

def publish_library(copper: bytes, blank: bytes) -> dict[str, object]:
    """A stable identity resolves to the graph it was published with."""
    info = CAPI.ctex_material_graph_library_info()
    info.size = CAPI.CTEX_MATERIAL_GRAPH_LIBRARY_INFO_CURRENT_SIZE
    ok(CAPI.ctex_material_graph_library_create_empty(CAPI.byref(info), None, 0, None, 0))
    empty = CAPI.create_string_buffer(info.canonical_size)
    ok(CAPI.ctex_material_graph_library_create_empty(
        CAPI.byref(info), empty, info.canonical_size, None, 0))
    assert info.preset_count == 0, "a new library was not empty"
    blob = empty.raw
    for stable_id, name, thumbnail, graph in (
            (b"studio.material.copper", b"Copper", b"thumbs/copper.png", copper),
            (b"studio.material.blank", b"Blank", b"thumbs/blank.png", blank)):
        held = CAPI.create_string_buffer(graph)
        preset = CAPI.ctex_material_graph_preset_descriptor(
            CAPI.CTEX_MATERIAL_GRAPH_PRESET_DESCRIPTOR_CURRENT_SIZE, CAPI.String(stable_id),
            CAPI.String(name), CAPI.String(thumbnail),
            ctypes.cast(held, ctypes.c_void_p), len(graph))
        info.size = CAPI.CTEX_MATERIAL_GRAPH_LIBRARY_INFO_CURRENT_SIZE
        ok(CAPI.ctex_material_graph_library_add_preset(
            blob, len(blob), CAPI.byref(preset), CAPI.byref(info), None, 0, None, 0))
        output = CAPI.create_string_buffer(info.canonical_size)
        ok(CAPI.ctex_material_graph_library_add_preset(
            blob, len(blob), CAPI.byref(preset), CAPI.byref(info), output,
            info.canonical_size, None, 0))
        blob = output.raw
    info.size = CAPI.CTEX_MATERIAL_GRAPH_LIBRARY_INFO_CURRENT_SIZE
    ok(CAPI.ctex_material_graph_library_inspect(
        blob, len(blob), CAPI.byref(info), None, 0, None, 0))
    assert (info.preset_count, info.canonical_size) == (2, len(blob))
    report = CAPI.create_string_buffer(info.report_size)
    ok(CAPI.ctex_material_graph_library_inspect(
        blob, len(blob), CAPI.byref(info), None, 0, report, info.report_size))
    listed = {item["stable_id"]: item for item in json.loads(report.value)["presets"]}
    assert sorted(listed) == ["studio.material.blank", "studio.material.copper"]
    copper_entry = listed["studio.material.copper"]
    assert copper_entry["thumbnail_resource"] == "thumbs/copper.png"
    assert (copper_entry["name"], copper_entry["node_count"]) == ("Copper", 5)
    assert listed["studio.material.blank"]["node_count"] == 1
    graph_info = CAPI.ctex_material_graph_info()
    graph_info.size = CAPI.CTEX_MATERIAL_GRAPH_INFO_CURRENT_SIZE
    ok(CAPI.ctex_material_graph_library_resolve_preset(
        blob, len(blob), CAPI.String(b"studio.material.copper"), CAPI.byref(graph_info),
        None, 0, None, 0))
    resolved = CAPI.create_string_buffer(graph_info.canonical_size)
    ok(CAPI.ctex_material_graph_library_resolve_preset(
        blob, len(blob), CAPI.String(b"studio.material.copper"), CAPI.byref(graph_info),
        resolved, graph_info.canonical_size, None, 0))
    assert graphs_equal(resolved.raw, copper), "a stable identity resolved to another graph"
    assert not graphs_equal(resolved.raw, blank), "every identity resolved to the same graph"
    graph_info.size = CAPI.CTEX_MATERIAL_GRAPH_INFO_CURRENT_SIZE
    absent = refused(CAPI.ctex_material_graph_library_resolve_preset(
        blob, len(blob), CAPI.String(b"studio.material.ghost"), CAPI.byref(graph_info),
        None, 0, None, 0))
    assert "studio.material.ghost" in absent, "the unknown identity was not named"
    return {"bytes": len(blob), "presets": sorted(listed), "refusal": absent}

def share_group(base: bytes) -> dict[str, object]:
    """One group definition drives three materials and survives an interface edit."""
    workspace = CAPI.POINTER(CAPI.ctex_material_graph_workspace)()
    ok(CAPI.ctex_material_graph_workspace_create(CAPI.byref(workspace)))
    for identifier in MATERIALS:
        ok(CAPI.ctex_material_graph_workspace_add_material(
            workspace, CAPI.String(identifier), base, len(base)))
    inputs = (CAPI.ctex_material_graph_socket_descriptor * 1)(
        socket(b"factor", value_of(SCALAR, scalar=0.25)))
    outputs = (CAPI.ctex_material_graph_socket_descriptor * 1)(socket(b"value"))
    definition = CAPI.ctex_material_graph_group_descriptor()
    definition.size = CAPI.CTEX_MATERIAL_GRAPH_GROUP_DESCRIPTOR_CURRENT_SIZE
    definition.identifier = CAPI.String(b"weathering")
    definition.display_name = CAPI.String(b"Weathering")
    definition.inputs = inputs
    definition.input_count = 1
    definition.outputs = outputs
    definition.output_count = 1
    ok(CAPI.ctex_material_graph_workspace_create_group(workspace, CAPI.byref(definition)))
    instances = {}
    for index, identifier in enumerate(MATERIALS):
        node = CAPI.c_uint64()
        ok(CAPI.ctex_material_graph_workspace_instantiate_group(
            workspace, CAPI.String(b"weathering"), MATERIAL, CAPI.String(identifier),
            CAPI.ctex_vec2f(-80.0, 40.0 * index), CAPI.byref(node)))
        stored = value_of(SCALAR, scalar=0.1 * (index + 1))
        ok(CAPI.ctex_material_graph_workspace_set_input_value(
            workspace, MATERIAL, CAPI.String(identifier), node.value,
            CAPI.String(b"factor"), CAPI.byref(stored)))
        instances[identifier.decode()] = node.value
    constant = CAPI.c_uint64()
    ok(CAPI.ctex_material_graph_workspace_add_builtin_node(
        workspace, GROUP, CAPI.String(b"weathering"), CAPI.String(b"ctex.input.constant-value"),
        CAPI.ctex_vec2f(0.0, 0.0), CAPI.byref(constant)))
    inner = link_descriptor((constant.value, b"value"), (1, b"value"))
    link_info = CAPI.ctex_material_graph_link_info()
    link_info.size = CAPI.CTEX_MATERIAL_GRAPH_LINK_INFO_CURRENT_SIZE
    ok(CAPI.ctex_material_graph_workspace_add_link(
        workspace, GROUP, CAPI.String(b"weathering"), CAPI.byref(inner),
        CAPI.byref(link_info), None, 0))
    assert link_info.graph.link_count == 1, "the group subgraph did not keep its own link"
    changed = (CAPI.ctex_material_graph_socket_descriptor * 2)(
        socket(b"detail", value_of(SCALAR, scalar=0.75)),
        socket(b"factor", value_of(SCALAR, scalar=0.5)))
    interface = CAPI.ctex_material_graph_group_interface_descriptor()
    interface.size = CAPI.CTEX_MATERIAL_GRAPH_GROUP_INTERFACE_DESCRIPTOR_CURRENT_SIZE
    interface.inputs = changed
    interface.input_count = 2
    interface.outputs = outputs
    interface.output_count = 1
    update = CAPI.ctex_material_graph_group_update_info()
    update.size = CAPI.CTEX_MATERIAL_GRAPH_GROUP_UPDATE_INFO_CURRENT_SIZE
    ok(CAPI.ctex_material_graph_workspace_update_group_interface(
        workspace, CAPI.String(b"weathering"), CAPI.byref(interface), CAPI.byref(update)))
    assert (update.group_version, update.instances_updated) == (2, len(MATERIALS))
    for index, identifier in enumerate(MATERIALS):
        _, report, info = document(
            lambda i, c, cs, r, rs, name=identifier: CAPI.ctex_material_graph_workspace_get_graph(
                workspace, MATERIAL, CAPI.String(name), CAPI.byref(i), c, cs, r, rs))
        assert info.node_count == 2, "a material lost its group instance"
        instance = [node for node in json.loads(report)["nodes"] if node["id"] != 1][0]
        assert instance["type_id"] == "ctex.group-instance"
        assert instance["type_version"] == update.group_version
        assert instance["properties"][0]["value"] == "weathering"
        sockets = {item["id"]: item for item in instance["inputs"]}
        assert sockets["detail"]["default"] == 0.75, "the new interface socket lost its default"
        assert sockets["factor"]["default"] == round(0.1 * (index + 1), 6), "a value was lost"
    node = CAPI.c_uint64()
    message = refused(CAPI.ctex_material_graph_workspace_instantiate_group(
        workspace, CAPI.String(b"weathering"), GROUP, CAPI.String(b"weathering"),
        CAPI.ctex_vec2f(0.0, 0.0), CAPI.byref(node)))
    assert "weathering -> weathering" in message, "recursion was not refused with its path"
    info = CAPI.ctex_material_graph_workspace_info()
    info.size = CAPI.CTEX_MATERIAL_GRAPH_WORKSPACE_INFO_CURRENT_SIZE
    ok(CAPI.ctex_material_graph_workspace_get_info(workspace, CAPI.byref(info)))
    assert (info.material_count, info.group_count) == (len(MATERIALS), 1)
    CAPI.ctex_material_graph_workspace_destroy(workspace)
    return {"instances": instances, "recursion_refusal": message, "version": update.group_version}

def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--executor", default="cpu")
    arguments = parser.parse_args()
    catalogue = browse_catalogue()
    copper, identifiers, refusal = author_copper_graph()
    diagnostics = check_resources(copper)
    blank, _, _ = document(
        lambda i, c, cs, r, rs: CAPI.ctex_material_graph_create_default(
            CAPI.byref(i), c, cs, r, rs))
    library = publish_library(copper, blank)
    workspace = share_group(blank)
    final, canonical, info = inspect(copper)
    assert canonical == copper
    arguments.output.mkdir(parents=True, exist_ok=True)
    (arguments.output / "copper_graph.json").write_text(
        json.dumps(final, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    summary = {"canonical_bytes": len(copper), "capabilities": list(CAPABILITIES),
               "catalogue": catalogue, "diagnostics": diagnostics, "library": library,
               "link_count": info.link_count, "link_refusal": refusal,
               "node_count": info.node_count, "node_ids": identifiers, "workspace": workspace}
    (arguments.output / "summary.json").write_text(
        json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8")

if __name__ == "__main__":
    main()
