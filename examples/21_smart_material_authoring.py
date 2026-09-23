#!/usr/bin/env python3
"""Author a parameterised, anchored smart material and apply it to a texture set.

Capabilities: smart-materials.
One exposed parameter drives the same graph input in two differently authored
stack entries, an anchor publishes the base layer into the coat, and the
finished material is applied, masked, edited and undone on a texture set.
"""

from __future__ import annotations

import argparse
import ctypes
import json
import struct
from pathlib import Path

import cybertexel


CAPABILITIES = ("smart-materials",)
MATERIAL_ID = "materials/atelier-copper"
MASK_ID = "masks/atelier-grime"
RESOURCE_ID = "images/noise"
SIZE = 32


def check(result: int, expected: int = 0) -> None:
    """Fail with the library's own diagnostic rather than a bare result code."""
    diagnostic = cybertexel.capi.ctex_get_last_diagnostic().decode("utf-8")
    assert result == expected, diagnostic


def hex_text(value: str) -> str:
    return value.encode("utf-8").hex()


def hex_double(value: float) -> str:
    return struct.pack(">d", value).hex()


def graph_info(capi: object) -> object:
    info = capi.ctex_material_graph_info()
    info.size = capi.CTEX_MATERIAL_GRAPH_INFO_CURRENT_SIZE
    return info


def material_info(capi: object) -> object:
    info = capi.ctex_smart_material_info()
    info.size = capi.CTEX_SMART_MATERIAL_INFO_CURRENT_SIZE
    return info


def value_of(capi: object, kind: int, scalar: float = 0.0, text: str = "") -> object:
    value = capi.ctex_smart_material_value_descriptor()
    value.size = capi.CTEX_SMART_MATERIAL_VALUE_DESCRIPTOR_CURRENT_SIZE
    value.type, value.scalar, value.text = kind, scalar, capi.String(text.encode())
    return value


def set_property(capi: object, graph: bytes, node: int, name: bytes, value: object) -> bytes:
    """Rewrite one node property of an authored graph under the two-call contract."""
    info = graph_info(capi)
    head = (capi.create_string_buffer(graph), len(graph), node, capi.String(name),
            capi.byref(value), capi.byref(info))
    assert capi.ctex_material_graph_set_property_value(*head, None, 0) == capi.CTEX_RESULT_SUCCESS
    output = capi.create_string_buffer(info.canonical_size)
    check(capi.ctex_material_graph_set_property_value(*head, output, len(output)))
    assert info.node_count == 3
    return output.raw[: info.canonical_size]


def add_builtin_node(capi: object, graph: bytes, type_id: bytes) -> tuple[bytes, int]:
    info, node = graph_info(capi), capi.uint64_t()
    head = (
        capi.create_string_buffer(graph),
        len(graph),
        capi.String(type_id),
        capi.ctex_vec2f(0.0, 0.0),
        capi.byref(info),
        capi.byref(node),
    )
    assert capi.ctex_material_graph_add_builtin_node(*head, None, 0) == capi.CTEX_RESULT_SUCCESS
    output = capi.create_string_buffer(info.canonical_size)
    check(capi.ctex_material_graph_add_builtin_node(*head, output, len(output)))
    return output.raw[: info.canonical_size], node.value


def authored_graph(capi: object) -> tuple[bytes, bytes, int]:
    """Build two entry graphs that differ: the coat blurs the image with another kernel."""
    info = graph_info(capi)
    assert capi.ctex_material_graph_create_default(capi.byref(info), None, 0, None, 0) == 0
    default = capi.create_string_buffer(info.canonical_size)
    check(
        capi.ctex_material_graph_create_default(capi.byref(info), default, len(default), None, 0)
    )
    assert (info.output_node_id, info.node_count, info.link_count) == (1, 1, 0)
    canonical = default.raw[: info.canonical_size]
    graph, image_node = add_builtin_node(capi, canonical, b"ctex.texture.image")
    graph, blur_node = add_builtin_node(capi, graph, b"ctex.filter.blur")
    assert (image_node, blur_node) == (2, 3)

    image = value_of(capi, capi.CTEX_SMART_MATERIAL_VALUE_IMAGE, text=RESOURCE_ID)
    base = set_property(capi, graph, image_node, b"image", image)
    kernel = value_of(capi, capi.CTEX_SMART_MATERIAL_VALUE_STRING, text="box")
    return base, set_property(capi, base, blur_node, b"kernel", kernel), blur_node


def serialize_material(base: bytes, coat: bytes, blur_node: int) -> bytes:
    """Emit canonical schema-6 smart-material records in their declared order."""
    records = [
        "CTEX_SMART_MATERIAL\t6",
        f"PRESET\t{hex_text(MATERIAL_ID)}\t{hex_text('Atelier copper')}",
    ]
    entries = (("base", "Base metal", base), ("coat", "Patina coat", coat))
    for identifier, display, graph in entries:
        records.append(
            f"ENTRY\t0\t{hex_text(identifier)}\t\t{hex_text(display)}"
            f"\t1\t{hex_double(1.0)}\t{graph.hex()}\t0"
        )
    records.append(f"ANCHOR\t{hex_text('base')}")
    records.append(
        f"ANCHOR_REF\t{hex_text('base')}\t{hex_text('coat')}\t{blur_node}\t{hex_text('image')}"
    )
    records.append(f"RESOURCE\t{hex_text(RESOURCE_ID)}\t{hex_text('image')}")
    records.append(
        f"PARAM\t0\t{hex_text('wear')}\t{hex_text('Wear')}\t{hex_text('Surface')}"
        f"\tscalar\t{hex_double(0.25)}\t{hex_double(0.0)}\t{hex_double(1.0)}\t0"
    )
    for identifier in ("base", "coat"):
        records.append(
            f"BIND\t{hex_text('wear')}\t{hex_text(identifier)}\t1\t0\t{hex_text('pbr.roughness')}"
        )
    return ("\n".join(records + ["END"]) + "\n").encode("utf-8")


def serialize_mask(graph: bytes) -> bytes:
    definition = "\n".join(
        [
            "CTEX_SMART_MATERIAL\t6",
            f"PRESET\t{hex_text(MASK_ID)}\t{hex_text('Atelier grime')}",
            f"ENTRY\t2\t{hex_text('root')}\t\t{hex_text('Grime')}\t1"
            f"\t{hex_double(1.0)}\t{graph.hex()}\t0",
            f"RESOURCE\t{hex_text(RESOURCE_ID)}\t{hex_text('image')}",
            "END\n",
        ]
    ).encode("utf-8")
    return f"CTEX_SMART_MASK\t1\nDEFINITION\t{definition.hex()}\nEND\n".encode("utf-8")


def inspect_material(capi: object, material: bytes) -> tuple[object, bytes, dict[str, object]]:
    """Inspect under the atomic two-call contract; a short report must write nothing."""
    info = material_info(capi)
    head = (capi.create_string_buffer(material), len(material), capi.byref(info))
    assert capi.ctex_smart_material_inspect(*head, None, 0, None, 0) == capi.CTEX_RESULT_SUCCESS
    canonical = capi.create_string_buffer(b"\xa5" * info.canonical_size)
    check(capi.ctex_smart_material_inspect(
        *head, canonical, info.canonical_size, capi.create_string_buffer(1), 1),
        capi.CTEX_RESULT_BUFFER_TOO_SMALL)
    assert canonical.raw[: info.canonical_size] == b"\xa5" * info.canonical_size
    report = capi.create_string_buffer(info.report_size)
    check(capi.ctex_smart_material_inspect(
        *head, canonical, info.canonical_size, report, len(report)))
    return info, canonical.raw[: info.canonical_size], json.loads(report.value)


def entry_graphs(material: bytes) -> list[bytes]:
    """Split the canonical record stream back into the graph each entry carries."""
    records = material.decode("utf-8").splitlines()
    return [bytes.fromhex(record.split("\t")[7]) for record in records if record[:6] == "ENTRY\t"]


def graph_roughness(capi: object, material: bytes) -> list[float]:
    """Read back the pbr.roughness default the parameter binds to, entry by entry."""
    values: list[float] = []
    for graph in entry_graphs(material):
        info = graph_info(capi)
        head = (capi.create_string_buffer(graph), len(graph), capi.byref(info))
        assert capi.ctex_material_graph_inspect(*head, None, 0, None, 0) == 0
        report = capi.create_string_buffer(info.report_size)
        check(capi.ctex_material_graph_inspect(*head, None, 0, report, len(report)))
        node = json.loads(report.value)["nodes"][0]
        defaults = {socket["id"]: socket["default"] for socket in node["inputs"]}
        values.append(defaults["pbr.roughness"])
    return values


def set_parameter(capi: object, material: bytes, scalar: float) -> bytes:
    value = value_of(capi, capi.CTEX_SMART_MATERIAL_VALUE_SCALAR, scalar=scalar)
    info = material_info(capi)
    head = (capi.create_string_buffer(material), len(material), capi.String(b"wear"),
            capi.byref(value), capi.byref(info))
    check(capi.ctex_smart_material_set_parameter(*head, None, 0, None, 0))
    updated = capi.create_string_buffer(info.canonical_size)
    report = capi.create_string_buffer(info.report_size)
    check(capi.ctex_smart_material_set_parameter(
        *head, updated, info.canonical_size, report, len(report)))
    assert info.exposed_parameter_count == 1 and info.anchor_reference_count == 1
    return updated.raw[: info.canonical_size]


def mark_anchor(capi: object, material: bytes, entry: bytes) -> bytes:
    info = material_info(capi)
    head = (
        capi.create_string_buffer(material),
        len(material),
        capi.String(entry),
        1,
        capi.byref(info),
    )
    assert capi.ctex_smart_material_set_anchor(*head, None, 0, None, 0) == capi.CTEX_RESULT_SUCCESS
    updated = capi.create_string_buffer(info.canonical_size)
    check(capi.ctex_smart_material_set_anchor(*head, updated, info.canonical_size, None, 0))
    assert info.anchor_count == 2
    return updated.raw[: info.canonical_size]


def refuse_anchor_cycle(capi: object, material: bytes, blur_node: int) -> str:
    """An anchor reference back into its own producer must be refused atomically."""
    info = material_info(capi)
    untouched = capi.create_string_buffer(b"\xa5" * len(material))
    check(capi.ctex_smart_material_add_anchor_reference(
        capi.create_string_buffer(material),
        len(material),
        capi.String(b"coat"),
        capi.String(b"base"),
        blur_node,
        capi.String(b"image"),
        capi.byref(info),
        untouched,
        len(material),
        None,
        0,), capi.CTEX_RESULT_INVALID_ARGUMENT)
    assert capi.ctex_get_last_diagnostic_code() == capi.CTEX_DIAGNOSTIC_INVALID_SMART_MATERIAL
    assert untouched.raw[: len(material)] == b"\xa5" * len(material)
    return capi.ctex_get_last_diagnostic().decode("utf-8")


def plan_evaluation(capi: object, material: bytes, changed: bytes) -> str:
    entry = capi.create_string_buffer(changed)
    identifiers = (ctypes.POINTER(ctypes.c_char) * 1)(
        ctypes.cast(entry, ctypes.POINTER(ctypes.c_char))
    )
    required = capi.c_size_t()
    head = (capi.create_string_buffer(material), len(material), identifiers, 1)
    check(capi.ctex_smart_material_plan_anchor_evaluation(*head, None, 0, capi.byref(required)))
    plan = capi.create_string_buffer(required.value)
    check(capi.ctex_smart_material_plan_anchor_evaluation(
        *head, plan, len(plan), capi.byref(required)))
    return plan.value.decode("utf-8")


def applications(capi: object, document: object, texture_set: bytes) -> dict[str, object]:
    required = capi.c_size_t()
    head = (document, capi.String(texture_set))
    check(capi.ctex_texture_set_get_preset_applications(*head, None, 0, capi.byref(required)))
    report = capi.create_string_buffer(required.value)
    check(capi.ctex_texture_set_get_preset_applications(
        *head, report, len(report), capi.byref(required)))
    return json.loads(report.value)


def application_info(capi: object) -> object:
    info = capi.ctex_preset_application_info()
    info.size = capi.CTEX_PRESET_APPLICATION_INFO_CURRENT_SIZE
    return info


def apply_and_undo(
    capi: object, document: object, texture_set: bytes, material: bytes, mask: bytes
) -> dict[str, object]:
    info = application_info(capi)
    check(capi.ctex_texture_set_apply_smart_material(
        document,
        capi.String(texture_set),
        capi.create_string_buffer(material),
        len(material),
        capi.String(b"copper"),
        capi.byref(info),))
    assert info.kind == capi.CTEX_APPLIED_PRESET_SMART_MATERIAL
    assert (info.schema_version, info.entry_count) == (6, 2)
    assert (info.application_count, info.undo_step_count) == (1, 1)

    check(capi.ctex_texture_set_set_applied_entry_state(
        document, capi.String(texture_set), capi.String(b"copper/base"), 0, 0.25))
    entries = applications(capi, document, texture_set)["applications"][0]["entries"]
    assert [item["id"] for item in entries] == ["copper/base", "copper/coat"]
    assert [(item["enabled"], item["opacity"]) for item in entries] == [
        (False, 0.25), (True, 1.0)]

    mask_info, mask_buffer = application_info(capi), capi.create_string_buffer(mask)
    mask_call = (document, capi.String(texture_set), mask_buffer, len(mask))
    check(capi.ctex_texture_set_apply_smart_mask(
        *mask_call, capi.String(b"grime"), capi.String(b"copper/base"), capi.byref(mask_info)))
    assert mask_info.kind == capi.CTEX_APPLIED_PRESET_SMART_MASK
    assert (mask_info.entry_count, mask_info.application_count) == (1, 2)

    refused = application_info(capi)
    check(capi.ctex_texture_set_apply_smart_mask(
        *mask_call, capi.String(b"orphan"), capi.String(b"missing"), capi.byref(refused)),
        capi.CTEX_RESULT_INVALID_ARGUMENT)
    applied = applications(capi, document, texture_set)
    assert [item["id"] for item in applied["applications"]] == ["copper", "grime"]

    removed = []
    for _ in range(3):
        undo = capi.ctex_preset_undo_info()
        undo.size = capi.CTEX_PRESET_UNDO_INFO_CURRENT_SIZE
        check(capi.ctex_texture_set_undo_last_preset_application(
            document, capi.String(texture_set), capi.byref(undo)))
        removed.append((undo.removed, undo.removed_entry_count, undo.application_count))
    assert removed == [(1, 1, 1), (1, 2, 0), (0, 0, 0)]
    assert applications(capi, document, texture_set)["applications"] == []
    return applied


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--executor", default="cpu")
    arguments = parser.parse_args()

    capi = cybertexel.capi
    base, coat, blur_node = authored_graph(capi)
    material = serialize_material(base, coat, blur_node)
    info, canonical, report = inspect_material(capi, material)
    assert canonical == material
    assert (info.source_schema_version, info.canonical_schema_version) == (6, 6)
    assert (info.entry_count, info.derived_entry_count) == (2, 2)
    assert (info.model_specific_entry_count, info.model_specific_pixel_bytes) == (0, 0)
    assert info.exposed_parameter_count == 1 and info.resource_reference_count == 1
    assert (info.anchor_count, info.anchor_reference_count) == (1, 1)
    assert report["parameters"][0]["binding_count"] == 2
    assert [entry["content"] for entry in report["entries"]] == ["derived", "derived"]
    assert base != coat and entry_graphs(material) == [base, coat]

    assert graph_roughness(capi, material) == [0.5, 0.5]
    worn = set_parameter(capi, material, 0.75)
    assert graph_roughness(capi, worn) == [0.75, 0.75]
    worn_graphs = entry_graphs(worn)
    assert worn_graphs != [base, coat] and worn_graphs[0] != worn_graphs[1]
    _, worn_canonical, worn_report = inspect_material(capi, worn)
    assert worn_canonical == worn and worn_report["parameters"][0]["binding_count"] == 2

    two_anchors = mark_anchor(capi, worn, b"coat")
    assert inspect_material(capi, two_anchors)[2]["anchors"] == ["base", "coat"]
    diagnostic = refuse_anchor_cycle(capi, two_anchors, blur_node)
    assert "cycle" in diagnostic.lower()
    plan = plan_evaluation(capi, two_anchors, b"base")
    assert plan == '["coat"]' and plan_evaluation(capi, two_anchors, b"coat") == "[]"

    mask = serialize_mask(base)
    with cybertexel.Document() as document:
        texture_set = document.create_texture_set(
            "Body", partition_key="body", width=SIZE, height=SIZE
        )
        pointer = ctypes.cast(document._require_open(), ctypes.POINTER(capi.ctex_document))
        applied = apply_and_undo(capi, pointer, texture_set.identifier.encode(), worn, mask)

    arguments.output.mkdir(parents=True, exist_ok=True)
    (arguments.output / "atelier_copper.smart-material").write_bytes(worn)
    (arguments.output / "atelier_grime.smart-mask").write_bytes(mask)
    (arguments.output / "applications.json").write_text(
        json.dumps(applied, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    summary = {
        "anchor_evaluation_plan": plan,
        "anchor_reference_count": info.anchor_reference_count,
        "applied_presets": [item["id"] for item in applied["applications"]],
        "capabilities": list(CAPABILITIES),
        "cycle_diagnostic": diagnostic,
        "entry_count": info.entry_count,
        "material_bytes": len(worn),
        "parameter_bindings": report["parameters"][0]["binding_count"],
        "roughness_after_parameter": graph_roughness(capi, worn),
        "roughness_before_parameter": graph_roughness(capi, material),
    }
    (arguments.output / "summary.json").write_text(
        json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )


if __name__ == "__main__":
    main()
