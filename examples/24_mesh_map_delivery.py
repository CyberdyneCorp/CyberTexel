#!/usr/bin/env python3
"""Turn a delivered layered PSD into a bound, colour-managed mesh-map set.

Capabilities: mesh-maps, image-io, color-management.
A vendor delivers baked maps as one layered PSD. The script splits the layers,
resolves what colour space each delivered channel meant, binds scalar maps to a
texture set, proves a missing map is named rather than guessed, generates a wear
mask, re-bakes one map through versioned tokens, and releases what it made
resident.
"""

from __future__ import annotations

import argparse
import ctypes
import json
import struct
from pathlib import Path

import cybertexel
import numpy as np


CAPABILITIES = ("mesh-maps", "image-io", "color-management")
CAPI = cybertexel.capi
OK = CAPI.CTEX_RESULT_SUCCESS
TEXTURE_SET_ID = b"material/4:mesh/uv/3:uv0"
AMBIENT = CAPI.CTEX_MESH_MAP_AMBIENT_OCCLUSION
CURVATURE = CAPI.CTEX_MESH_MAP_CURVATURE
THICKNESS = CAPI.CTEX_MESH_MAP_THICKNESS
LINEAR = CAPI.CTEX_COLOR_SPACE_LINEAR_REC709
SRGB = CAPI.CTEX_COLOR_SPACE_SRGB_REC709
MAP_SIZE, MASK_SIZE, PREVIEW = 4, 8, 128
SUMMARY: dict[str, object] = {"capabilities": list(CAPABILITIES)}


def call(name: str, *arguments: object, expect: int = OK) -> None:
    assert getattr(CAPI, name)(*arguments) == expect


def sized(name: str, **fields: object) -> object:
    """Allocate a versioned ABI struct with its CURRENT_SIZE and named fields."""
    value = getattr(CAPI, name)()
    value.size = getattr(CAPI, f"{name.upper()}_CURRENT_SIZE")
    for field, item in fields.items():
        setattr(value, field, item)
    return value


def pixel_buffer(pixels: np.ndarray, width: int, height: int) -> object:
    return sized("ctex_mesh_map_pixel_buffer_descriptor", width=width, height=height,
                 component_type=CAPI.CTEX_TRANSPORT_COMPONENT_UINT8_UNORM, component_count=1,
                 row_stride_bytes=width, pixels=pixels.ctypes.data_as(ctypes.c_void_p),
                 pixel_bytes=pixels.nbytes)


def layered_psd() -> bytes:
    """Two 1x1 RGBA raster layers named Top and Bottom, hand-assembled."""
    def record(name: bytes) -> bytes:
        padded = ((len(name) + 4) // 4) * 4
        channels = b"".join(struct.pack(">hI", channel, 3) for channel in (0, 1, 2, -1))
        return (struct.pack(">iiiiH", 0, 0, 1, 1, 4) + channels + b"8BIMnorm"
                + bytes([255, 0, 0, 0]) + struct.pack(">III", 8 + padded, 0, 0)
                + bytes([len(name)]) + name + bytes(padded - len(name) - 1))

    body = struct.pack(">h", 2) + record(b"Top") + record(b"Bottom")
    for rgba in ((255, 0, 0, 128), (0, 0, 255, 255)):
        body += b"".join(struct.pack(">HB", 0, value) for value in rgba)
    tail = struct.pack(">H", 0) + bytes([128, 0, 127, 255])
    header = b"8BPS" + struct.pack(">H6xHIIHHII", 1, 4, 1, 1, 8, 3, 0, 0)
    return header + struct.pack(">II", len(body) + len(tail) + 4, len(body)) + body + tail


def decode_delivery() -> list[tuple[str, list[int]]]:
    """Individual-layer decoding is sized first and publishes atomically."""
    encoded = layered_psd()
    source = (CAPI.c_ubyte * len(encoded)).from_buffer_copy(encoded)
    descriptor = sized("ctex_layered_image_decode_descriptor",
                       mode=CAPI.CTEX_LAYERED_IMAGE_DECODE_INDIVIDUAL,
                       intended_channel=CAPI.CTEX_CHANNEL_SEMANTIC_BASE_COLOR,
                       input_color_space=CAPI.CTEX_INPUT_COLOR_SPACE_AUTOMATIC,
                       maximum_image_count=8)
    info = sized("ctex_layered_image_decode_info")

    def decode(buffers, expect=OK, name=b"delivery.psd", data=source) -> None:
        call("ctex_image_decode_layered_memory", data, len(data), CAPI.String(name),
             CAPI.byref(descriptor), None, None, None, CAPI.byref(info), *buffers, expect=expect)

    empty = (None, 0, None, 0, None, 0)
    decode(empty)
    assert info.detected_format == CAPI.CTEX_IMAGE_FILE_FORMAT_PSD and info.image_count == 2
    assert info.source_was_layered == 1
    assert info.required_name_buffer_size == 11 and info.required_pixel_buffer_size == 8
    layers = (CAPI.ctex_layered_decoded_image_info * info.required_image_info_count)()
    layers[0].width = 77
    pixels = (CAPI.c_ubyte * info.required_pixel_buffer_size)()
    names = CAPI.create_string_buffer(info.required_name_buffer_size)
    cramped = CAPI.create_string_buffer(len(names) - 1)
    decode((layers, len(layers), cramped, len(cramped), pixels, len(pixels)),
           CAPI.CTEX_RESULT_BUFFER_TOO_SMALL)
    assert layers[0].width == 77
    decode((layers, len(layers), names, len(names), pixels, len(pixels)))
    raw, data = bytes(names), bytes(pixels)
    decoded = [(raw[item.name_offset :][: item.name_size].decode(),
                list(data[item.pixel_offset :][: item.pixel_size])) for item in layers]
    assert decoded == [("Top", [255, 0, 0, 128]), ("Bottom", [0, 0, 255, 255])]
    assert all(item.width == 1 and item.channel_count == 4 for item in layers)
    flat = cybertexel.encode_image(np.full((2, 2, 3), 200, dtype=np.uint8))
    buffer = (CAPI.c_ubyte * len(flat)).from_buffer_copy(flat)
    decode(empty, CAPI.CTEX_RESULT_UNSUPPORTED_OPERATION, b"flat.png", buffer)
    SUMMARY.update(delivery_bytes=len(encoded), delivery_layers=[n for n, _ in decoded])
    return decoded


def colour_policy(tint: list[int]) -> tuple[float, float, float]:
    """What the delivered bytes meant, and what the artist sees in preview."""
    resolved = sized("ctex_resolved_input_color_space")

    def resolve(semantic) -> int:
        call("ctex_resolve_input_color_space", CAPI.CTEX_INPUT_COLOR_SPACE_AUTOMATIC, semantic,
             CAPI.byref(resolved))
        assert resolved.inferred == 1
        return int(resolved.color_space)

    base, rough = CAPI.CTEX_CHANNEL_SEMANTIC_BASE_COLOR, CAPI.CTEX_CHANNEL_SEMANTIC_ROUGHNESS
    normal = CAPI.CTEX_CHANNEL_SEMANTIC_NORMAL
    assert resolve(base) == SRGB and resolve(rough) == LINEAR and resolve(normal) == LINEAR
    encoded = CAPI.ctex_rgb_color(*(value / 255.0 for value in tint[:3]), SRGB)
    working, data, back = (CAPI.ctex_rgb_color() for _ in range(3))
    call("ctex_color_input_to_working", CAPI.byref(encoded), base, CAPI.byref(working))
    assert working.color_space == LINEAR
    call("ctex_color_input_to_working", CAPI.byref(encoded), rough, CAPI.byref(data))
    assert (data.red, data.green, data.blue) == (encoded.red, encoded.green, encoded.blue)
    call("ctex_color_convert", CAPI.byref(working), SRGB, CAPI.byref(back))
    np.testing.assert_allclose([back.red, back.green, back.blue],
                               [encoded.red, encoded.green, encoded.blue], atol=1e-12)
    unknown = CAPI.ctex_rgb_color(0.0, 0.0, 0.0, 99)
    call("ctex_color_convert", CAPI.byref(unknown), LINEAR, CAPI.byref(back),
         expect=CAPI.CTEX_RESULT_UNSUPPORTED_OPERATION)
    assert CAPI.ctex_get_last_diagnostic_code() == CAPI.CTEX_DIAGNOSTIC_UNSUPPORTED_COLOR_SPACE
    warning = sized("ctex_bit_depth_warning")
    call("ctex_channel_get_bit_depth_warning", normal, 8, CAPI.byref(warning))
    assert warning.warning == 1 and warning.recommended_bit_depth == 16
    call("ctex_channel_get_bit_depth_warning", normal, 16, CAPI.byref(warning))
    assert warning.warning == 0
    cube = b"LUT_3D_SIZE 2\n1 1 1\n0 1 1\n1 0 1\n0 0 1\n1 1 0\n0 1 0\n1 0 0\n0 0 0\n"
    lut, preview = ctypes.POINTER(CAPI.ctex_cube_lut)(), CAPI.ctex_rgb_color()
    call("ctex_cube_lut_create", CAPI.String(cube), len(cube), CAPI.byref(lut))
    try:
        authored = (working.red, working.green, working.blue)
        call("ctex_cube_lut_apply_preview", lut, CAPI.byref(working), CAPI.byref(preview))
        assert preview.color_space == working.color_space
        np.testing.assert_allclose([preview.red, preview.green, preview.blue],
                                   [1.0 - value for value in authored], atol=1e-12)
        assert (working.red, working.green, working.blue) == authored
    finally:
        CAPI.ctex_cube_lut_destroy(lut)
    malformed = b"LUT_3D_SIZE 2\n0 0 0\n"
    broken = ctypes.POINTER(CAPI.ctex_cube_lut)()
    call("ctex_cube_lut_create", CAPI.String(malformed), len(malformed), CAPI.byref(broken),
         expect=CAPI.CTEX_RESULT_INVALID_ARGUMENT)
    assert not broken
    assert CAPI.ctex_get_last_diagnostic_code() == CAPI.CTEX_DIAGNOSTIC_INVALID_CUBE_LUT
    tinted = (float(preview.red), float(preview.green), float(preview.blue))
    SUMMARY["preview_tint"] = [round(value, 6) for value in tinted]
    SUMMARY["working_tint"] = [round(value, 6) for value in authored]
    return tinted


def bind_delivered_maps(maps: object) -> None:
    """Import the two scalar maps, then prove the third is named, not guessed."""
    info = sized("ctex_mesh_map_set_info")
    identifier, uv_set = CAPI.create_string_buffer(64), CAPI.create_string_buffer(16)

    def metadata():
        call("ctex_mesh_map_set_get_info", maps, CAPI.byref(info), identifier, 64, uv_set, 16)
        return info

    assert metadata().bound_map_count == 0 and info.resident_pixel_bytes == 0
    assert identifier.value == TEXTURE_SET_ID and uv_set.value == b"uv0"
    assert info.texture_set_width == MASK_SIZE and info.mesh_revision != 0
    report = sized("ctex_mesh_map_import_info")

    def deliver(kind, pixels, components=1, expect=OK) -> None:
        descriptor = sized("ctex_mesh_map_import_descriptor", kind=kind, color_space=LINEAR,
                           channel_meaning=CAPI.CTEX_MESH_MAP_SCALAR_DATA,
                           buffer=pixel_buffer(pixels, MAP_SIZE, MAP_SIZE))
        descriptor.buffer.component_count = components
        call("ctex_mesh_map_set_import_external", maps, CAPI.byref(descriptor),
             CAPI.byref(report), expect=expect)

    occlusion = np.arange(16, dtype=np.uint8) * 17
    deliver(AMBIENT, occlusion)
    assert report.resolution_mismatch == 1 and report.replaced_existing == 0
    assert (report.map_width, report.texture_set_width) == (MAP_SIZE, MASK_SIZE)
    deliver(AMBIENT, occlusion, 257, CAPI.CTEX_RESULT_INVALID_ARGUMENT)
    assert metadata().bound_map_count == 1
    deliver(CURVATURE, np.arange(16, dtype=np.uint8) * 8 + 64)
    sample = sized("ctex_mesh_map_sample_info")

    def read(u, v) -> float:
        call("ctex_mesh_map_set_sample", maps, AMBIENT, u, v, CAPI.byref(sample))
        assert sample.component_count == 1 and sample.stale == 0
        return round(float(sample.values[0]), 6)

    assert [read(0.0, 0.0), read(0.5, 0.5), read(1.0, 1.0)] == [0.8, 0.5, 0.2]
    call("ctex_mesh_map_set_sample", maps, THICKNESS, 0.5, 0.5, CAPI.byref(sample),
         expect=CAPI.CTEX_RESULT_MISSING_RESOURCE)
    assert CAPI.ctex_get_last_diagnostic_code() == CAPI.CTEX_DIAGNOSTIC_INVALID_MESH_MAP
    entries, count = (CAPI.ctex_mesh_map_entry_info * 4)(), CAPI.c_size_t()
    call("ctex_mesh_map_set_get_entries", maps, entries, 4, CAPI.byref(count))
    bound = sorted(int(entry.kind) for entry in entries[:2])
    assert count.value == 2 and bound == sorted([AMBIENT, CURVATURE])
    assert all(entry.stale == 0 and entry.width == MAP_SIZE for entry in entries[:2])
    resident = sum(int(entry.resident_pixel_bytes) for entry in entries[:2])
    assert metadata().resident_pixel_bytes == resident and resident > 0
    required, missing = (CAPI.uint32_t * 2)(AMBIENT, THICKNESS), (CAPI.uint32_t * 2)()
    requirement, message = sized("ctex_mesh_map_requirement_info"), CAPI.create_string_buffer(256)
    call("ctex_mesh_map_set_check_requirements", maps, CAPI.String(b"wear"), required, 2, missing,
         2, None, 0, CAPI.byref(requirement), message, len(message))
    assert requirement.required_missing_map_count == 1
    assert requirement.required_stale_map_count == 0 and missing[0] == THICKNESS
    assert b"thickness" in message.value
    SUMMARY.update(bound_kinds=bound, resident_pixel_bytes=resident,
                   missing_diagnostic=message.value.decode())


def generate_wear(maps: object) -> np.ndarray:
    """The catalogue names its inputs and bounds; generation reports both."""
    catalogue = sized("ctex_mesh_map_generator_info")
    book, kind = CAPI.byref(catalogue), CAPI.CTEX_MESH_MAP_GENERATOR_DIRT
    call("ctex_mesh_map_generator_get_info", kind, book, None, 0, None, 0, None, 0)
    inputs = (CAPI.uint32_t * catalogue.required_map_count)()
    declared = (CAPI.ctex_mesh_map_generator_parameter_descriptor * catalogue.parameter_count)()
    strings = CAPI.create_string_buffer(catalogue.required_string_size)
    call("ctex_mesh_map_generator_get_info", kind, book, inputs, len(inputs), declared,
         len(declared), strings, len(strings))
    text = bytes(strings)
    assert text[catalogue.name_offset :][: catalogue.name_size - 1] == b"dirt"
    parameters = [text[item.name_offset :][: item.name_size - 1].decode() for item in declared]
    assert list(inputs) == [AMBIENT, CURVATURE]
    assert parameters == ["strength", "contrast", "curvature-weight"]
    assert declared[0].minimum == 0.0 and declared[0].maximum == 2.0
    call("ctex_mesh_map_generator_get_info", 99, None, None, 0, None, 0, None, 0,
         expect=CAPI.CTEX_RESULT_INVALID_ARGUMENT)
    parameter = CAPI.ctex_mesh_map_generator_parameter
    current = CAPI.CTEX_MESH_MAP_GENERATOR_PARAMETER_CURRENT_SIZE
    requested = (parameter * 2)(parameter(current, CAPI.String(b"strength"), 3.0),
                                parameter(current, CAPI.String(b"contrast"), 2.0))
    result = sized("ctex_mesh_map_generator_result_info")

    def generate(buffers, generator=kind, expect=OK) -> None:
        call("ctex_mesh_map_generator_generate", maps, generator, MASK_SIZE, MASK_SIZE, requested,
             2, CAPI.byref(result), *buffers, expect=expect)

    empty = (None, 0, None, 0, None, 0, None, 0, None, 0)
    generate(empty)
    assert result.required_mask_value_count == MASK_SIZE * MASK_SIZE
    assert result.required_resolved_parameter_count == 3
    assert result.required_parameter_clamp_count == 1 and result.required_string_size > 0
    values = (CAPI.c_float * result.required_mask_value_count)()
    resolved = (CAPI.ctex_mesh_map_generator_resolved_parameter * 3)()
    clamps = (CAPI.ctex_mesh_map_generator_parameter_clamp * 1)()
    stale = (CAPI.ctex_mesh_map_staleness * 2)()
    report = CAPI.create_string_buffer(result.required_string_size)
    buffers = (values, len(values), resolved, 3, clamps, 1, stale, 2, report, len(report))
    generate(buffers)
    mask = np.ctypeslib.as_array(values).reshape(MASK_SIZE, MASK_SIZE).copy()
    generate(buffers)
    np.testing.assert_array_equal(np.ctypeslib.as_array(values).reshape(mask.shape), mask)
    text, clamped = bytes(report), clamps[0]
    assert clamped.supplied == 3.0 and clamped.resolved == 2.0
    assert text[clamped.name_offset :][: clamped.name_size - 1] == b"strength"
    assert 0.0 <= float(mask.min()) and float(mask.max()) <= 1.0 and len(np.unique(mask)) > 1
    generate(empty, CAPI.CTEX_MESH_MAP_GENERATOR_SCRATCHES, CAPI.CTEX_RESULT_MISSING_RESOURCE)
    SUMMARY.update(generator_inputs=list(inputs), generator_parameters=parameters,
                   mask_mean=round(float(mask.mean()), 6), strength_clamped_to=clamped.resolved)
    return mask


def rebake_curvature(maps: object) -> None:
    """Two outstanding requests for one map: the superseded one cannot bind."""
    session = ctypes.POINTER(CAPI.ctex_mesh_map_bake_session)()
    call("ctex_mesh_map_bake_session_create", maps, 7, CAPI.byref(session))
    tokens: list[object] = []
    try:
        for _ in range(2):
            token = ctypes.POINTER(CAPI.ctex_mesh_map_bake_request_token)()
            call("ctex_mesh_map_bake_session_begin", session, CURVATURE, 2, 2, CAPI.byref(token))
            tokens.append(token)
        identity = sized("ctex_mesh_map_bake_token_info")
        identifier, uv_set = CAPI.create_string_buffer(64), CAPI.create_string_buffer(16)
        call("ctex_mesh_map_bake_request_token_get_info", tokens[1], CAPI.byref(identity),
             identifier, 64, uv_set, 16, None, 0)
        assert identity.bake_settings_revision == 7 and identity.session_identity != 0
        assert identity.kind == CURVATURE and (identity.width, identity.height) == (2, 2)
        assert identifier.value == TEXTURE_SET_ID and uv_set.value == b"uv0"
        superseded, published = np.full(4, 32, np.uint8), np.array([200, 210, 220, 230], np.uint8)

        def complete(token, pixels) -> int:
            output = sized("ctex_mesh_map_bake_output_descriptor",
                           buffer=pixel_buffer(pixels, 2, 2))
            completion = sized("ctex_mesh_map_bake_completion_info")
            call("ctex_mesh_map_bake_session_complete", session, token, CAPI.byref(output),
                 CAPI.byref(completion))
            return int(completion.disposition)

        assert complete(tokens[0], superseded) == CAPI.CTEX_MESH_MAP_BAKE_STALE
        assert complete(tokens[1], published) == CAPI.CTEX_MESH_MAP_BAKE_BOUND
        assert complete(tokens[1], superseded) == CAPI.CTEX_MESH_MAP_BAKE_UNKNOWN_TOKEN
        sample = sized("ctex_mesh_map_sample_info")
        call("ctex_mesh_map_set_sample", maps, CURVATURE, 0.0, 1.0, CAPI.byref(sample))
        assert abs(sample.values[0] - 200.0 / 255.0) < 1e-6
        state = sized("ctex_mesh_map_bake_session_info")
        call("ctex_mesh_map_bake_session_get_info", session, CAPI.byref(state))
        assert state.settings_revision == 7 and state.pending_request_count == 0
        SUMMARY.update(bake_settings_revision=7, baked_corner=round(200.0 / 255.0, 6))
    finally:
        for token in tokens:
            CAPI.ctex_mesh_map_bake_request_token_destroy(token)
        CAPI.ctex_mesh_map_bake_session_destroy(session)


def retire(maps: object, mesh_pointer: object) -> None:
    """Nothing is stale while the mesh holds still, and release is accounted for."""
    count = CAPI.c_size_t()
    call("ctex_mesh_map_set_synchronize_mesh", maps, mesh_pointer, None, 0, CAPI.byref(count))
    released = sized("ctex_mesh_map_release_info")
    call("ctex_mesh_map_set_release", maps, CURVATURE, CAPI.byref(released))
    assert count.value == 0 and released.released_map_count == 1
    single = int(released.resident_pixel_bytes_released)
    call("ctex_mesh_map_set_release_all", maps, CAPI.byref(released))
    assert single > 0 and released.released_map_count == 1
    info = sized("ctex_mesh_map_set_info")
    call("ctex_mesh_map_set_get_info", maps, CAPI.byref(info), None, 0, None, 0)
    assert info.bound_map_count == 0 and info.resident_pixel_bytes == 0
    SUMMARY.update(released_bytes_per_map=single, stale_after_synchronize=0)


def publish_mask(mask: np.ndarray, tint: tuple[float, float, float]) -> np.ndarray:
    """Ordered dithering spreads a half tone that plain quantization rounds up."""
    quantized = np.zeros(mask.shape, dtype=np.uint8)
    dithered, plain = CAPI.uint8_t(), CAPI.uint8_t()
    for y, x in np.ndindex(mask.shape):
        call("ctex_quantize_unorm8", float(mask[y, x]), x, y, 1, CAPI.byref(dithered))
        quantized[y, x] = dithered.value
    call("ctex_quantize_unorm8", 0.5, 0, 0, 0, CAPI.byref(plain))
    call("ctex_quantize_unorm8", 0.5, 0, 0, 1, CAPI.byref(dithered))
    assert plain.value == 128 and dithered.value == 127
    assert int(quantized.max()) > int(quantized.min())
    factor = PREVIEW // MASK_SIZE
    scaled = np.repeat(np.repeat(quantized, factor, axis=0), factor, axis=1)
    colour = np.asarray(tint, dtype=np.float64).clip(0.0, 1.0)
    return np.rint(scaled[:, :, None] * colour[None, None, :]).astype(np.uint8)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--executor", default="cpu")
    arguments = parser.parse_args()

    tint = colour_policy(decode_delivery()[0][1])
    uv = np.asarray([(0.0, 0.0), (1.0, 0.0), (1.0, 1.0), (0.0, 1.0)], dtype=np.float32)
    positions, normals = np.pad(uv, ((0, 0), (0, 1))), np.full((4, 3), (0, 0, 1), np.float32)
    triangles = np.asarray([[0, 1, 2], [0, 2, 3]], dtype=np.uint32)
    with cybertexel.Document() as document, cybertexel.Mesh(
        positions, triangles, normals=normals, uv=uv
    ) as mesh:
        texture_set = document.create_texture_set("Panel", partition_key="mesh",
                                                  width=MASK_SIZE, height=MASK_SIZE)
        assert texture_set.identifier.encode() == TEXTURE_SET_ID
        mesh_pointer = ctypes.cast(mesh._require_open(), ctypes.POINTER(CAPI.ctex_mesh))
        frame, frame_uv = sized("ctex_mesh_tangent_frame_info"), CAPI.create_string_buffer(16)
        call("ctex_mesh_get_tangent_frame", mesh_pointer, CAPI.byref(frame), frame_uv, 16)
        assert frame.source == CAPI.CTEX_TANGENT_FRAME_GENERATED
        assert frame.corner_tangent_count == 6 and frame_uv.value == b"uv0"
        owner = ctypes.cast(document._require_open(), ctypes.POINTER(CAPI.ctex_document))
        maps = ctypes.POINTER(CAPI.ctex_mesh_map_set)()
        call("ctex_mesh_map_set_create", owner, CAPI.String(TEXTURE_SET_ID), mesh_pointer,
             CAPI.byref(maps))
        try:
            bind_delivered_maps(maps)
            mask = generate_wear(maps)
            rebake_curvature(maps)
            retire(maps, mesh_pointer)
        finally:
            CAPI.ctex_mesh_map_set_destroy(maps)

    arguments.output.mkdir(parents=True, exist_ok=True)
    preview = cybertexel.encode_image(publish_mask(mask, tint))
    (arguments.output / "wear_mask.png").write_bytes(preview)
    (arguments.output / "summary.json").write_text(
        json.dumps(SUMMARY, indent=2, sort_keys=True) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
