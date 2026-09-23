#!/usr/bin/env python3
"""Carry a fixture mesh through maps, layers, material and texture export.

Capabilities: smart-materials, texture-export.
The example creates a material graph, embeds and applies it as a smart material,
uses a generated mesh-map mask to shade the layer, and exports the final texture.
"""

from __future__ import annotations

import argparse
import ctypes
import json
from pathlib import Path

import cybertexel
import numpy as np


ROOT = Path(__file__).resolve().parent
CAPABILITIES = ("smart-materials", "texture-export")
SIZE = 32


def fixture_mesh() -> tuple[np.ndarray, np.ndarray, np.ndarray, np.ndarray]:
    positions: list[list[float]] = []
    uv: list[list[float]] = []
    triangles: list[list[int]] = []
    path = ROOT / "fixtures" / "meshes" / "quad_uv.obj"
    for line in path.read_text(encoding="utf-8").splitlines():
        fields = line.split()
        if fields[:1] == ["v"]:
            positions.append([float(value) for value in fields[1:4]])
        elif fields[:1] == ["vt"]:
            uv.append([float(value) for value in fields[1:3]])
        elif fields[:1] == ["f"]:
            triangles.append([int(value.split("/")[0]) - 1 for value in fields[1:4]])
    return (
        np.asarray(positions, dtype=np.float32),
        np.asarray(triangles, dtype=np.uint32),
        np.asarray([[0.0, 0.0, 1.0]] * len(positions), dtype=np.float32),
        np.asarray(uv, dtype=np.float32),
    )


def make_smart_material(capi: object) -> bytes:
    graph_info = capi.ctex_material_graph_info()
    graph_info.size = capi.CTEX_MATERIAL_GRAPH_INFO_CURRENT_SIZE
    assert (
        capi.ctex_material_graph_create_default(
            capi.byref(graph_info), None, 0, None, 0
        )
        == capi.CTEX_RESULT_SUCCESS
    )
    graph = capi.create_string_buffer(graph_info.canonical_size)
    assert (
        capi.ctex_material_graph_create_default(
            capi.byref(graph_info), graph, len(graph), None, 0
        )
        == capi.CTEX_RESULT_SUCCESS
    )
    material = (
        "CTEX_SMART_MATERIAL\t6\n"
        f"PRESET\t{'examples/oxidized-copper'.encode().hex()}\t{'Oxidized copper'.encode().hex()}\n"
        f"ENTRY\t0\t{'surface'.encode().hex()}\t\t{'Surface'.encode().hex()}\t1\t"
        f"3ff0000000000000\t{graph.raw.hex()}\t0\n"
        "END\n"
    ).encode()
    info = capi.ctex_smart_material_info()
    info.size = capi.CTEX_SMART_MATERIAL_INFO_CURRENT_SIZE
    source = capi.create_string_buffer(material)
    assert (
        capi.ctex_smart_material_inspect(
            source, len(material), capi.byref(info), None, 0, None, 0
        )
        == capi.CTEX_RESULT_SUCCESS
    )
    assert (info.entry_count, info.derived_entry_count, info.model_specific_entry_count) == (
        1,
        1,
        0,
    )
    return material


def apply_smart_material(
    capi: object, document: cybertexel.Document, texture_set: cybertexel.TextureSet, material: bytes
) -> dict[str, object]:
    source = capi.create_string_buffer(material)
    info = capi.ctex_preset_application_info()
    info.size = capi.CTEX_PRESET_APPLICATION_INFO_CURRENT_SIZE
    document_pointer = ctypes.cast(
        document._require_open(), ctypes.POINTER(capi.ctex_document)
    )
    assert (
        capi.ctex_texture_set_apply_smart_material(
            document_pointer,
            capi.String(texture_set.identifier.encode()),
            source,
            len(material),
            capi.String(b"example-application"),
            capi.byref(info),
        )
        == capi.CTEX_RESULT_SUCCESS
    )
    required = capi.c_size_t()
    assert (
        capi.ctex_texture_set_get_preset_applications(
            document_pointer,
            capi.String(texture_set.identifier.encode()),
            None,
            0,
            capi.byref(required),
        )
        == capi.CTEX_RESULT_SUCCESS
    )
    report = capi.create_string_buffer(required.value)
    assert (
        capi.ctex_texture_set_get_preset_applications(
            document_pointer,
            capi.String(texture_set.identifier.encode()),
            report,
            len(report),
            capi.byref(required),
        )
        == capi.CTEX_RESULT_SUCCESS
    )
    application_report = json.loads(report.value)
    assert info.entry_count == 1 and info.application_count == 1
    application = application_report["applications"][0]
    assert application["id"] == "example-application"
    return application


def export_texture(capi: object, pixels: np.ndarray) -> tuple[bytes, dict[str, object], str]:
    encoded_outputs: list[bytes] = []
    reports: list[dict[str, object]] = []
    paths: list[str] = []
    coverage = (capi.uint8_t * (SIZE * SIZE))(*([1] * (SIZE * SIZE)))

    @capi.ctex_texture_export_sample_callback
    def sample(x: int, y: int, output: object, _user_data: object) -> int:
        value = output.contents
        rgb = pixels[y, x]
        value.base_color[:] = tuple(float(component) / 255.0 for component in rgb)
        value.opacity = 1.0
        value.roughness = 0.55
        value.metallic = 0.8
        value.normal[:] = (0.5, 0.5, 1.0)
        value.occlusion = float(rgb[1]) / 255.0
        return capi.CTEX_RESULT_SUCCESS

    @capi.ctex_texture_export_source_callback
    def source(planned: object, output: object, _user_data: object) -> int:
        assert (planned.contents.width, planned.contents.height) == (SIZE, SIZE)
        output.contents.size = capi.CTEX_TEXTURE_EXPORT_PIXEL_SOURCE_DESCRIPTOR_CURRENT_SIZE
        output.contents.width = SIZE
        output.contents.height = SIZE
        output.contents.sample = sample
        output.contents.coverage = coverage
        output.contents.coverage_count = len(coverage)
        return capi.CTEX_RESULT_SUCCESS

    @capi.ctex_texture_export_output_callback
    def receive_output(output: object, _user_data: object) -> int:
        item = output.contents
        encoded_outputs.append(ctypes.string_at(item.bytes, item.byte_count))
        paths.append(item.relative_path.data.decode())
        return capi.CTEX_RESULT_SUCCESS

    @capi.ctex_texture_export_report_callback
    def receive_report(payload: object, size: int, _user_data: object) -> int:
        reports.append(json.loads(ctypes.string_at(payload.data, size)))
        return capi.CTEX_RESULT_SUCCESS

    @capi.ctex_texture_export_progress_callback
    def progress(_completed: int, _total: int, _path: object, _user_data: object) -> None:
        return None

    @capi.ctex_texture_export_cancel_callback
    def cancel(_user_data: object) -> int:
        return 0

    layer = capi.ctex_texture_export_layer_source_descriptor(
        capi.CTEX_TEXTURE_EXPORT_LAYER_SOURCE_DESCRIPTOR_CURRENT_SIZE,
        capi.String(b"surface"),
        capi.String(b"Surface"),
        capi.String(b""),
        capi.CTEX_TEXTURE_EXPORT_LAYER_CONTENT,
        1,
    )
    layers = (capi.ctex_texture_export_layer_source_descriptor * 1)(layer)
    texture_set = capi.ctex_texture_export_texture_set_source_descriptor(
        capi.CTEX_TEXTURE_EXPORT_TEXTURE_SET_SOURCE_DESCRIPTOR_CURRENT_SIZE,
        capi.String(b"body"),
        capi.String(b"Body"),
        SIZE,
        SIZE,
        None,
        0,
        layers,
        1,
    )
    texture_sets = (capi.ctex_texture_export_texture_set_source_descriptor * 1)(texture_set)
    catalogue = capi.ctex_texture_export_catalogue_descriptor(
        capi.CTEX_TEXTURE_EXPORT_CATALOGUE_DESCRIPTOR_CURRENT_SIZE,
        capi.String(b"fixture"),
        texture_sets,
        1,
        None,
        0,
    )
    texture = capi.ctex_texture_export_texture_descriptor()
    texture.size = capi.CTEX_TEXTURE_EXPORT_TEXTURE_DESCRIPTOR_CURRENT_SIZE
    texture.suffix = capi.String(b"_BaseColor")
    token_buffers = [
        capi.create_string_buffer(token)
        for token in (b"base_color.r", b"base_color.g", b"base_color.b", b"opacity")
    ]
    for index, token in enumerate(token_buffers):
        texture.channel_tokens[index] = ctypes.cast(token, ctypes.POINTER(ctypes.c_char))
    texture.color_space = capi.CTEX_COLOR_SPACE_LINEAR_REC709
    texture.bit_depth = 8
    texture.format = capi.CTEX_IMAGE_FILE_FORMAT_PNG
    textures = (capi.ctex_texture_export_texture_descriptor * 1)(texture)
    preset = capi.ctex_texture_export_preset_descriptor(
        capi.CTEX_TEXTURE_EXPORT_PRESET_DESCRIPTOR_CURRENT_SIZE,
        capi.String(b"example-base-color"),
        capi.String(b"Example base color"),
        textures,
        1,
    )
    plan = capi.ctex_texture_export_plan_descriptor(
        capi.CTEX_TEXTURE_EXPORT_PLAN_DESCRIPTOR_CURRENT_SIZE,
        capi.CTEX_TEXTURE_EXPORT_TEXTURE_SET_ALL,
        None,
        0,
        capi.CTEX_TEXTURE_EXPORT_SCOPE_TEXTURE_SET,
        capi.CTEX_TEXTURE_EXPORT_LAYER_FLATTEN_VISIBLE,
        None,
        0,
        SIZE,
        SIZE,
        capi.String(b"{project}_{texture_set}{suffix}.{extension}"),
    )
    options = capi.ctex_texture_export_options_descriptor(
        capi.CTEX_TEXTURE_EXPORT_OPTIONS_DESCRIPTOR_CURRENT_SIZE,
        capi.pointer(plan),
        0,
        90,
        0,
    )
    callbacks = capi.ctex_texture_export_callbacks_descriptor(
        capi.CTEX_TEXTURE_EXPORT_CALLBACKS_DESCRIPTOR_CURRENT_SIZE,
        source,
        receive_output,
        receive_report,
        progress,
        cancel,
        None,
    )
    info = capi.ctex_texture_export_info()
    info.size = capi.CTEX_TEXTURE_EXPORT_INFO_CURRENT_SIZE
    assert (
        capi.ctex_texture_export_run(
            capi.byref(catalogue),
            capi.byref(preset),
            capi.byref(options),
            capi.byref(callbacks),
            capi.byref(info),
        )
        == capi.CTEX_RESULT_SUCCESS
    )
    assert (info.planned_output_count, info.encoded_output_count) == (1, 1)
    assert len(encoded_outputs) == len(reports) == len(paths) == 1
    return encoded_outputs[0], reports[0], paths[0]


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--executor", default="cpu")
    arguments = parser.parse_args()

    capi = cybertexel.capi
    positions, triangles, normals, uv = fixture_mesh()
    ao_path = ROOT / "fixtures" / "maps" / "ambient_occlusion.png"
    ao = cybertexel.decode_image(
        ao_path.read_bytes(),
        source_name=ao_path.name,
        intended_channel=cybertexel.ChannelSemantic.OCCLUSION,
    ).pixels[:, :, 0]
    material = make_smart_material(capi)
    with (
        cybertexel.Document() as document,
        cybertexel.Mesh(positions, triangles, normals=normals, uv=uv) as mesh,
    ):
        texture_set = document.create_texture_set(
            "Fixture Quad", partition_key="body", width=SIZE, height=SIZE
        )
        document.set_channel_enabled(texture_set, "pbr.base_color")
        with cybertexel.MeshMapSet(document, texture_set, mesh) as maps:
            maps.import_map(cybertexel.MeshMapKind.AMBIENT_OCCLUSION, ao)
            mask = maps.generate_mask(
                cybertexel.MeshMapGeneratorKind.AMBIENT_OCCLUSION, SIZE, SIZE
            )
        application = apply_smart_material(capi, document, texture_set, material)

    shaded = np.empty((SIZE, SIZE, 3), dtype=np.uint8)
    shaded[:, :, 0] = np.rint(35 + mask * 150).astype(np.uint8)
    shaded[:, :, 1] = np.rint(45 + mask * 80).astype(np.uint8)
    shaded[:, :, 2] = np.rint(50 + mask * 25).astype(np.uint8)
    exported, report, relative_path = export_texture(capi, shaded)
    decoded = cybertexel.decode_image(exported, source_name=relative_path)
    np.testing.assert_array_equal(decoded.pixels[:, :, :3], shaded)
    assert report["dry_run"] is False and len(report["outputs"]) == 1

    arguments.output.mkdir(parents=True, exist_ok=True)
    (arguments.output / "fixture_body_BaseColor.png").write_bytes(exported)
    summary = {
        "application_id": application["id"],
        "capabilities": list(CAPABILITIES),
        "export_path": relative_path,
        "material_bytes": len(material),
        "mesh_triangles": int(triangles.shape[0]),
        "mesh_vertices": int(positions.shape[0]),
        "output_shape": list(decoded.pixels.shape),
        "smart_material_entry_count": 1,
    }
    (arguments.output / "summary.json").write_text(
        json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )


if __name__ == "__main__":
    main()
