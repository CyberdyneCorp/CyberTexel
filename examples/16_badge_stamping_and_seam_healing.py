#!/usr/bin/env python3
"""Stamp a badge onto a crate panel with the retained tools, then heal its seams.

Capabilities: paint-tools, paint-engine, stroke-model
The badge pass end to end: restore the saved preset against the published
catalogue, stamp a decal and a text label, project, spray, filter, pad, defer a
dilation session and report what a cancelled preview leaves behind.
"""

from __future__ import annotations

import argparse
import ctypes
import json
from pathlib import Path

import cybertexel
import numpy as np


CAPABILITIES = ("paint-tools", "paint-engine", "stroke-model")
SUCCESS = cybertexel.capi.CTEX_RESULT_SUCCESS
TOO_SMALL = cybertexel.capi.CTEX_RESULT_BUFFER_TOO_SMALL
REFUSED = cybertexel.capi.CTEX_RESULT_INVALID_ARGUMENT
PRESET_NAME, TIP_IDENTITY = b"Badge chalk", b"brushes/chalk"
TEXTURE_SET, LABEL = b"material/5:crate/uv/3:uv0", "çç\nB".encode()
POSITIONS = np.array([[0.0, 0.0, 0.0], [10.0, 0.0, 0.0], [10.0, 1.0, 0.0], [0.0, 1.0, 0.0]],
                     dtype=np.float32)
UV = np.array([[0.0, 0.0], [1.0, 0.0], [1.0, 1.0], [0.0, 1.0]], dtype=np.float32)
TRIANGLES = np.array([[0, 1, 2], [0, 2, 3]], dtype=np.uint32)
NORMALS = np.tile(np.array([[0.0, 0.0, 1.0]], dtype=np.float32), (4, 1))
BLACK, RED = [[0.0, 0.0, 0.0, 1.0]], [[1.0, 0.0, 0.0, 1.0]]


def sized(name: str, **fields: object) -> object:
    """One descriptor or info record, stamped with its current ABI size."""
    value = getattr(cybertexel.capi, name)()
    value.size = getattr(cybertexel.capi, f"{name.upper()}_CURRENT_SIZE")
    for field, item in fields.items():
        setattr(value, field, item)
    return value


def doubles(values: object) -> object:
    return (cybertexel.capi.c_double * len(values))(*(float(value) for value in values))


def channel(rows: object) -> object:
    capi = cybertexel.capi
    pixels = (capi.ctex_vec4f * len(rows))(*(capi.ctex_vec4f(*row) for row in rows))
    return capi.pointer(capi.ctex_paint_tool_channel_descriptor(
        capi.CTEX_PAINT_TOOL_CHANNEL_DESCRIPTOR_CURRENT_SIZE,
        capi.String(b"pbr.base_color"), 3, pixels, len(rows)))


def texel(position: object, normal: object, uv: object, triangle: int) -> object:
    capi = cybertexel.capi
    return capi.ctex_paint_surface_texel(
        capi.ctex_vec3d(*position), capi.ctex_vec3d(*normal), capi.ctex_vec3d(0.0, 0.0, 1.0),
        capi.ctex_vec2d(*uv), triangle)


def output_of(pixels: object) -> object:
    return cybertexel.capi.pointer(cybertexel.capi.ctex_paint_tool_channel_output(
        cybertexel.capi.CTEX_PAINT_TOOL_CHANNEL_OUTPUT_CURRENT_SIZE, pixels, len(pixels)))


def restore_preset(capi: object) -> dict[str, object]:
    """Round-trip the saved preset, then check it against the published catalogue."""
    settings = capi.ctex_stroke_settings_descriptor()
    assert capi.ctex_stroke_settings_init(capi.byref(settings)) == SUCCESS
    curve = (capi.ctex_response_curve_point * 3)(*(capi.ctex_response_curve_point(*point)
             for point in ((0.0, 0.1), (0.4, 0.25), (1.0, 0.9))))
    settings.tip_mode, settings.radius, settings.flow = (
        capi.CTEX_STROKE_TIP_DISCRETE_ALPHA, 3.5, 0.45)
    settings.tip_resource_identity = capi.String(TIP_IDENTITY)
    settings.pressure_radius.points, settings.pressure_radius.point_count = curve, 3
    settings.symmetry.mirror_x, settings.symmetry.radial_count = 1, 3
    required, write = capi.c_size_t(), (capi.String(PRESET_NAME), capi.byref(settings))
    assert capi.ctex_stroke_preset_serialize(*write, None, 0, capi.byref(required)) == SUCCESS
    blob = capi.create_string_buffer(required.value)
    assert capi.ctex_stroke_preset_serialize(
        *write, blob, len(blob), capi.byref(required)) == SUCCESS
    info, load = sized("ctex_stroke_preset_info"), capi.ctex_stroke_preset_deserialize
    assert load(blob, required.value, capi.byref(info), None, None) == SUCCESS
    sizes = (info.required_name_size, info.required_tip_resource_identity_size,
             info.required_curve_point_count)
    assert sizes == (12, 14, 15), "the preset reports its name, tip and curve-point sizes"
    restored, points = sized("ctex_stroke_settings_descriptor"), (
        capi.ctex_response_curve_point * sizes[2])()
    name_buffer, tip_buffer = (capi.create_string_buffer(sizes[0]),
                               capi.create_string_buffer(sizes[1]))
    buffers = sized(
        "ctex_stroke_preset_buffers_descriptor", curve_points=points,
        curve_point_capacity=len(points), name_buffer=capi.String(name_buffer),
        name_buffer_size=len(name_buffer), tip_resource_identity_buffer=capi.String(tip_buffer),
        tip_resource_identity_buffer_size=len(tip_buffer))
    assert load(blob, required.value, capi.byref(info), capi.byref(restored),
                capi.byref(buffers)) == SUCCESS
    assert name_buffer.value == PRESET_NAME and tip_buffer.value == TIP_IDENTITY
    assert (restored.radius, restored.flow) == (3.5, 0.45)
    assert restored.pressure_radius.point_count == 3 and restored.symmetry.radial_count == 3
    catalogue = sized("ctex_paint_parameter_catalogue_info")
    listing = capi.ctex_paint_get_parameter_catalogue
    assert listing(None, 0, None, 0, capi.byref(catalogue)) == SUCCESS
    count = catalogue.required_parameter_count
    entries = (capi.ctex_paint_parameter_descriptor * count)()
    names = capi.create_string_buffer(catalogue.required_name_size)
    assert listing(entries, count, names, len(names), capi.byref(catalogue)) == SUCCESS
    published = {names.raw[e.name_offset : e.name_offset + e.name_size - 1]: e for e in entries
                 if e.context == capi.CTEX_PAINT_PARAMETER_CONTEXT_GENERAL}
    tracking = published[b"text.tracking_em"]
    assert (count, len(published)) == (74, 66) and tracking.maximum == -tracking.minimum == 10.0
    return {"preset_bytes": required.value, "preset_schema_version": int(info.schema_version),
            "catalogue_parameter_count": int(count)}


def stamp_decal_and_text(capi: object) -> dict[str, object]:
    """Rasterize the badge decal, then set its label with a caller-supplied font."""
    placement = capi.ctex_paint_decal_placement(
        capi.ctex_vec3d(0, 0, 0), capi.ctex_vec3d(0, 0, 1),
        capi.ctex_paint_decal_transform(0.0, 1.0, capi.ctex_vec2d(1.0, 1.0)))
    surface = (capi.ctex_paint_surface_texel * 2)(
        texel((0, 0, 0), (0, 0, 1), (0.25, 0.5), 0),
        texel((0.25, -0.25, 0), (0, 0, 1), (0.75, 0.5), 1))
    decal = sized("ctex_paint_decal_descriptor", width=2, height=1, surface_texels=surface,
                  surface_texel_count=2, coverage=(capi.c_ubyte * 2)(1, 1), coverage_count=2,
                  placement=placement, material_width=2, material_height=1,
                  material_opacity=doubles([0.5, 0.5]), material_opacity_count=2,
                  material=channel([[0.2, 0, 0, 1], [0.8, 0, 0, 1]]), material_channel_count=1,
                  enabled_layer_snapshot=channel(BLACK * 2), enabled_layer_channel_count=1,
                  blend_mode=capi.String(b"normal"))
    info, samples = sized("ctex_paint_decal_info"), (capi.c_size_t * 2)()
    strength, pixels = doubles([-1.0] * 2), (capi.ctex_vec4f * 2)()
    outputs = sized("ctex_paint_decal_outputs", source_sample_indices=samples,
                    source_sample_capacity=2, strength=strength, strength_capacity=2,
                    channels=output_of(pixels), channel_count=1)
    rasterize = (capi.byref(decal), capi.byref(info), capi.byref(outputs))
    assert capi.ctex_paint_rasterize_decal(*rasterize) == SUCCESS
    assert (samples[0], samples[1]) == (1, 1) and list(strength) == [0.5, 0.5]
    assert abs(pixels[1].x - 0.4) < 1.0e-6, "half opacity over black halves the material"
    glyphs = (capi.ctex_paint_font_glyph_descriptor * 2)(
        sized("ctex_paint_font_glyph_descriptor", codepoint=0x00E7, width=2, height=2,
              bearing_y=2.0, advance=2.0, coverage=doubles([1.0] * 4), coverage_count=4),
        sized("ctex_paint_font_glyph_descriptor", codepoint=ord("B"), width=1, height=2,
              bearing_y=2.0, advance=1.0, coverage=doubles([1.0] * 2), coverage_count=2))
    text = sized("ctex_paint_text_descriptor", width=1, height=1, surface_texel_count=1,
                 surface_texels=(capi.ctex_paint_surface_texel * 1)(
                     texel((0, 0, 0), (0, 0, 1), (0.5, 0.5), 0)),
                 coverage=(capi.c_ubyte * 1)(1), coverage_count=1, material_channel_count=1,
                 font=capi.pointer(sized("ctex_paint_font_descriptor", glyph_count=2,
                                         identity=capi.String(b"font:badge"), pixels_per_em=2.0,
                                         ascent=2.0, glyphs=glyphs)),
                 utf8=capi.String(LABEL), utf8_size=len(LABEL), tracking_em=20.0, text_size=2.0,
                 alignment=capi.CTEX_PAINT_TEXT_ALIGN_RIGHT, placement=placement,
                 material=capi.pointer(capi.ctex_paint_text_material_value(
                     capi.CTEX_PAINT_TEXT_MATERIAL_VALUE_CURRENT_SIZE,
                     capi.String(b"pbr.base_color"), 3, capi.ctex_vec4f(1.0, 0.0, 0.0, 1.0))),
                 enabled_layer_snapshot=channel(BLACK), enabled_layer_channel_count=1,
                 blend_mode=capi.String(b"normal"))
    text_info = sized("ctex_paint_text_info")
    apply = (capi.byref(text), capi.byref(text_info))
    assert capi.ctex_paint_apply_text(*apply, None) == SUCCESS
    assert (text_info.tracking_clamped, text_info.resolved_tracking_em) == (1, 10.0)
    assert text_info.required_raster_opacity_count == 96, "10em of tracking widens the raster"
    text.tracking_em = 0.0
    assert capi.ctex_paint_apply_text(*apply, None) == SUCCESS
    assert (text_info.required_codepoint_count, text_info.required_raster_opacity_count) == (4, 16)
    codepoints = (capi.c_uint32 * text_info.required_codepoint_count)()
    raster = doubles([-1.0] * text_info.required_raster_opacity_count)
    sample, text_strength, pixel = capi.c_size_t(99), doubles([-1.0]), (capi.ctex_vec4f * 1)()
    text_outputs = sized(
        "ctex_paint_text_outputs", codepoints=codepoints, codepoint_capacity=len(codepoints),
        raster_opacity=raster, raster_opacity_capacity=len(raster), strength_capacity=1,
        source_sample_indices=capi.pointer(sample), source_sample_capacity=1,
        strength=text_strength, channels=output_of(pixel), channel_count=1)
    assert capi.ctex_paint_apply_text(*apply, capi.byref(text_outputs)) == SUCCESS
    assert (text_info.raster_width, text_info.raster_height, text_info.line_count) == (4, 4, 2)
    assert text_info.frame_scale.x == 4.0 and text_strength[0] == 1.0 and pixel[0].x == 1.0
    assert list(codepoints) == [0x00E7, 0x00E7, ord("\n"), ord("B")] and sample.value != 99
    assert raster[8] == 0.0 and raster[11] == 1.0, "the newline row of the raster stays empty"
    text.utf8, text.utf8_size = capi.String(b"\xc0\x80"), 2
    assert capi.ctex_paint_apply_text(*apply, capi.byref(text_outputs)) == REFUSED
    assert list(codepoints) == [0x00E7, 0x00E7, ord("\n"), ord("B")], "the refusal is atomic"
    return {"decal_strength": list(strength)}


def project_and_spray(capi: object, mesh: object, index: object,
                      revision: int) -> dict[str, object]:
    """Project the badge material two ways, then spray deterministic particle wear."""
    tile = capi.ctex_paint_tile_coverage_descriptor(
        capi.CTEX_PAINT_TILE_COVERAGE_DESCRIPTOR_CURRENT_SIZE,
        capi.String(b"uv0"), 1, 1, capi.ctex_vec2d(0.0, 0.0))
    coordinates = sized("ctex_paint_material_coordinate_descriptor",
                        mode=capi.CTEX_PAINT_MATERIAL_COORDINATE_UV)
    sample, count = capi.ctex_paint_material_coordinate_sample(), capi.c_size_t()
    assert capi.ctex_paint_evaluate_material_coordinates(
        mesh, capi.byref(tile), capi.byref(coordinates), capi.byref(sample), 1,
        capi.byref(count)) == SUCCESS
    assert count.value == sample.covered == 1 and sample.projection_count == 1
    assert (sample.coordinates[0].x, sample.coordinates[0].y) == (0.5, 0.5)
    projection = sized(
        "ctex_paint_projection_descriptor", width=1, height=1, surface_texel_count=1,
        surface_texels=(capi.ctex_paint_surface_texel * 1)(
            texel((0.25, 0.75, 0.25), (0.7071067811865476, 0.5, 0.5), (0.0, 0.0), 0)),
        coverage=(capi.c_ubyte * 1)(1), coverage_count=1, material_channel_count=1,
        mode=capi.CTEX_PAINT_PROJECTION_CAMERA, camera_visible_surface=doubles([0.5]),
        camera_visible_surface_count=1, triplanar_scale=1.0, material_width=2, material_height=2,
        material=channel([[0.1, 0, 0, 1], [0.2, 0, 0, 1], [0.3, 0, 0, 1], [0.4, 0, 0, 1]]),
        material_opacity=doubles([1.0] * 4), material_opacity_count=4,
        enabled_layer_snapshot=channel(BLACK), enabled_layer_channel_count=1,
        blend_mode=capi.String(b"normal"))
    projection.camera_view_projection[:] = [float(r == c) for r in range(4) for c in range(4)]
    info, pixel = sized("ctex_paint_projection_info"), (capi.ctex_vec4f * 1)()
    projected, strength = capi.ctex_paint_projection_sample(), doubles([-1.0])
    outputs = sized("ctex_paint_projection_outputs", samples=capi.pointer(projected),
                    sample_capacity=1, strength=strength, strength_capacity=1,
                    channels=output_of(pixel), channel_count=1)
    apply = (capi.byref(projection), capi.byref(info))
    assert capi.ctex_paint_apply_projection(*apply, capi.byref(outputs)) == SUCCESS
    assert info.required_sample_count == projected.count == 1 and projected.source_indices[0] == 1
    assert strength[0] == 0.5 and abs(pixel[0].x - 0.1) < 1.0e-6, "the camera sees half the texel"
    projection.mode = capi.CTEX_PAINT_PROJECTION_TRIPLANAR
    assert capi.ctex_paint_apply_projection(*apply, capi.byref(outputs)) == SUCCESS
    assert projected.count == 3 and list(projected.source_indices) == [3, 2, 0]
    assert np.allclose(list(projected.weights), [0.5, 0.25, 0.25]), "three planes blend the map"
    binding = capi.ctex_pick_texture_set_binding_descriptor(
        capi.CTEX_PICK_TEXTURE_SET_BINDING_DESCRIPTOR_CURRENT_SIZE, 0, capi.String(b"uv0"))
    particles = sized(
        "ctex_paint_particle_descriptor", width=2, height=2, surface_texel_count=4,
        texture_set_id=capi.String(TEXTURE_SET), mesh_revision=revision,
        surface_texels=(capi.ctex_paint_surface_texel * 4)(
            *(texel((0, 0, 0), (0, 0, 1), (0, 0), 1) for _ in range(4))),
        coverage=(capi.c_ubyte * 4)(1, 1, 1, 1), coverage_count=4, material_channel_count=1,
        triangle_identity=(capi.c_uint32 * 4)(1, 1, 1, 1), triangle_identity_count=4,
        texture_sets=capi.pointer(binding), texture_set_count=1,
        emitter_position=capi.ctex_vec3d(1.0, 0.5, 1.0),
        emitter_direction=capi.ctex_vec3d(0.0, 0.0, -1.0),
        simulation=capi.ctex_paint_particle_settings(
            2, 1.0, 2.0, 1.0, capi.ctex_vec3d(0.0, 0.0, 0.0), 0.0, 0.0, 0.1, 42),
        material=channel(RED * 4), enabled_layer_snapshot=channel(BLACK * 4),
        enabled_layer_channel_count=1, blend_mode=capi.String(b"normal"))
    wear = sized("ctex_paint_particle_info")
    spray = (index, capi.byref(particles), capi.byref(wear))
    assert capi.ctex_paint_apply_particles(*spray, None) == SUCCESS
    counts = (wear.emitted_count, wear.required_final_state_count, wear.required_contact_count)
    assert counts == (2, 2, 2) and wear.required_texture_set_id_size == 52, "two contacts, two ids"
    contacts = (capi.ctex_paint_particle_contact * wear.required_contact_count)()
    states = (capi.ctex_paint_particle_state * wear.required_final_state_count)()
    identifiers = capi.create_string_buffer(wear.required_texture_set_id_size)
    wear_pixels, wear_strength = (capi.ctex_vec4f * 4)(), doubles([-1.0] * 4)
    outputs = sized(
        "ctex_paint_particle_outputs", contacts=contacts, contact_capacity=len(contacts),
        final_states=states, final_state_capacity=len(states),
        texture_set_ids=capi.String(identifiers), texture_set_id_size=len(identifiers),
        strength=wear_strength, strength_capacity=4, channels=output_of(wear_pixels),
        channel_count=1)
    assert capi.ctex_paint_apply_particles(*spray, capi.byref(outputs)) == SUCCESS
    first = (contacts[0].particle_ordinal, contacts[0].time_seconds, contacts[0].position.x,
             contacts[0].impulse, contacts[0].mapped_texel, states[0].velocity.z)
    start, size = contacts[0].texture_set_id_offset, contacts[0].texture_set_id_size
    assert identifiers.raw[start : start + size - 1] == TEXTURE_SET
    assert wear_strength[contacts[0].mapped_texel] > 0.0, "the contact deposits on its texel"
    for buffer in (contacts, states):
        ctypes.memset(buffer, 0, ctypes.sizeof(buffer))
    assert capi.ctex_paint_apply_particles(*spray, capi.byref(outputs)) == SUCCESS
    assert first == (contacts[0].particle_ordinal, contacts[0].time_seconds,
                     contacts[0].position.x, contacts[0].impulse, contacts[0].mapped_texel,
                     states[0].velocity.z), "a seeded replay is identical"
    return {"particle_contacts": int(wear.required_contact_count)}


def filter_and_pad(capi: object) -> dict[str, object]:
    """Filter across the seam with mirrored frames, then pad the UV islands."""
    axes = (((1, 0, 0), (0, 1, 0), (0, 0, 1)), ((1, 0, 0), (0, -1, 0), (0, 0, 1)))
    frames = [capi.ctex_stroke_frame(*(capi.ctex_vec3d(*axis) for axis in triple))
              for triple in axes]
    entry = capi.ctex_paint_surface_filter_sample
    samples = (entry * 2)(entry(0, frames[0], 0, 0, 1.0), entry(2, frames[0], 1, 0, 1.0))
    descriptor = sized("ctex_paint_surface_filter_descriptor", samples=samples, sample_count=2,
                       operation=capi.CTEX_PAINT_SURFACE_FILTER_BLUR, radius_x=1, radius_y=0,
                       output_frame=frames[0])
    scalar = capi.c_double(-1.0)
    blur = (capi.byref(descriptor), doubles([2.0, 100.0, 6.0, 10.0]), 4, capi.byref(scalar))
    assert capi.ctex_paint_filter_surface_scalar(*blur) == SUCCESS
    assert scalar.value == 4.0, "the blur averages the two neighbours across the seam"
    samples[0].weight = 0.5
    samples[1].texel_index, samples[1].tangent_frame, samples[1].weight = 1, frames[1], 0.5
    descriptor.operation = capi.CTEX_PAINT_SURFACE_FILTER_MIP_GENERATION
    vectors = (capi.ctex_vec3d * 2)(capi.ctex_vec3d(0.0, 0.6, 0.8), capi.ctex_vec3d(0.0, -0.6, 0.8))
    vector = capi.ctex_vec3d(-1.0, -1.0, -1.0)
    assert capi.ctex_paint_filter_surface_tangent_vector(
        capi.byref(descriptor), vectors, 2, capi.byref(vector)) == SUCCESS
    assert (vector.x, vector.y, vector.z) == (0.0, 0.6, 0.8), "the mirrored frame is un-mirrored"
    none = capi.CTEX_NO_UV_ISLAND
    padding = sized(
        "ctex_paint_island_padding_descriptor", width=7, height=1, component_count=1, radius_x=1,
        radius_y=0, requested_mip_levels=2, island_identity_count=7, pixel_count=7,
        island_identity=(capi.c_uint32 * 7)(none, 10, none, none, none, 20, none),
        pixels=doubles([-1.0, 10.0, -1.0, -1.0, -1.0, 20.0, -1.0]))
    info, ownership = sized("ctex_paint_island_padding_info"), (capi.c_uint32 * 7)()
    unsupported, affected = (capi.ctex_paint_unsupported_mip_level * 1)(), (capi.c_uint32 * 2)()
    owned, levels, touched = capi.c_size_t(), capi.c_size_t(), capi.c_size_t()
    plan, pad = (capi.byref(padding), capi.byref(info)), capi.ctex_paint_plan_island_padding
    assert pad(*plan, None, 0, capi.byref(owned), None, 0, capi.byref(levels), None, 0,
               capi.byref(touched)) == SUCCESS
    assert (owned.value, levels.value, touched.value) == (7, 1, 2)
    assert pad(*plan, ownership, 7, capi.byref(owned), unsupported, 1, capi.byref(levels),
               affected, 2, capi.byref(touched)) == SUCCESS
    assert (unsupported[0].mip_level, unsupported[0].required_gutter_radius) == (1, 3)
    padded, pixel_count = doubles([-9.0] * 7), capi.c_size_t()
    fill = capi.ctex_paint_apply_island_padding
    assert fill(*plan, padded, 6, capi.byref(pixel_count)) == TOO_SMALL
    assert padded[0] == -9.0, "a refused padding writes nothing"
    assert fill(*plan, padded, 7, capi.byref(pixel_count)) == SUCCESS
    assert list(padded) == [10.0, 10.0, 10.0, -1.0, 20.0, 20.0, 20.0], "gutters copy inward"
    assert pixel_count.value == 7 and info.padded_texel_count == 4
    return {"padded_texels": int(info.padded_texel_count)}


def defer_dilation(capi: object) -> dict[str, object]:
    """Stage the stroke's tiles and keep them provisional until one idempotent finish."""
    session = capi.POINTER(capi.ctex_paint_dilation_session)()
    assert capi.ctex_paint_dilation_session_create(2, capi.byref(session)) == SUCCESS
    for u, values in ((1, [-1.0, 4.0, 5.0, 6.0, -1.0]), (0, [-1.0, 1.0, 2.0, 3.0, -1.0]),
                      (0, [-1.0, 10.0, 11.0, 12.0, -1.0])):
        staged = sized("ctex_paint_dilation_tile_descriptor", u=u, v=0, width=5, height=1,
                       component_count=1, pixels=doubles(values), pixel_count=5,
                       coverage=(capi.c_ubyte * 5)(0, 1, 1, 1, 0), coverage_count=5)
        assert capi.ctex_paint_dilation_session_stage_tile(session, capi.byref(staged)) == SUCCESS
    info, reports = sized("ctex_paint_dilation_session_info"), (
        capi.ctex_paint_dilation_tile_info * 2)()
    pixels, tile_count, pixel_count = doubles([0.0] * 10), capi.c_size_t(), capi.c_size_t()
    read = (session, capi.byref(info), reports, 2, capi.byref(tile_count), pixels, 10,
            capi.byref(pixel_count))
    assert capi.ctex_paint_dilation_session_get_preview(*read) == SUCCESS
    assert info.state == capi.CTEX_PAINT_DILATION_PROVISIONAL and info.resolved_radius == 2
    assert (tile_count.value, pixel_count.value) == (2, 10) and info.dilation_pass_count == 0
    assert (reports[0].u, reports[0].pixel_offset, reports[1].u) == (0, 0, 1)
    assert pixels[1] == 10.0, "the latest staging of a tile wins"
    assert capi.ctex_paint_dilation_session_finish(*read) == SUCCESS
    assert info.state == capi.CTEX_PAINT_DILATION_FINAL and info.dilation_pass_count == 2
    assert reports[0].dilated_texel_count == 2, "each tile gained its own gutter"
    assert [pixels[0], pixels[4], pixels[5], pixels[9]] == [9.0, 13.0, 3.0, 7.0]
    assert capi.ctex_paint_dilation_session_stage_tile(session, capi.byref(staged)) == REFUSED
    capi.ctex_paint_dilation_session_destroy(session)
    return {"session_pass_count": int(info.dilation_pass_count)}


def preview_tiles(capi: object) -> dict[str, object]:
    """One provisional pixel dirties one storage tile, and cancelling leaves no trace."""
    with cybertexel.Document() as document:
        texture_set = document.create_texture_set(
            "Badge", partition_key="badge", width=128, height=128, default_bit_depth=16)
        document.set_channel_enabled(texture_set, "pbr.height", True, bit_depth=16)
        handle = ctypes.cast(document._require_open(), ctypes.POINTER(capi.ctex_document))
        session = capi.POINTER(capi.ctex_paint_preview_session)()
        assert capi.ctex_paint_preview_session_create(
            handle, capi.String(texture_set.identifier.encode()), capi.String(b"pbr.height"),
            capi.byref(session)) == SUCCESS
        value, write = capi.c_uint16(4096), capi.ctex_paint_preview_session_write_pixel
        assert write(session, 70, 70, capi.byref(value), 2) == SUCCESS
        count, changed = capi.c_size_t(), (capi.ctex_paint_tile_coordinate * 1)()
        assert capi.ctex_paint_preview_session_get_changed_tiles(
            session, changed, 1, capi.byref(count)) == SUCCESS
        assert count.value == 1 and (changed[0].x, changed[0].y) == (1, 1)
        assert capi.ctex_paint_preview_session_cancel(session) == SUCCESS
        assert write(session, 0, 0, capi.byref(value), 2) == REFUSED
        capi.ctex_paint_preview_session_destroy(session)
        assert document.read_channel(texture_set, "pbr.height").max() == 0, "nothing committed"
    return {"changed_tiles": [[int(changed[0].x), int(changed[0].y)]]}


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--executor", default="cpu")
    arguments = parser.parse_args()
    capi = cybertexel.capi
    summary = {"capabilities": list(CAPABILITIES), **restore_preset(capi)}
    summary.update(stamp_decal_and_text(capi))
    with cybertexel.Mesh(POSITIONS, TRIANGLES, normals=NORMALS, uv=UV,
                         partition_key="crate", partition_name="Crate") as mesh:
        pointer = ctypes.cast(mesh._require_open(), ctypes.POINTER(capi.ctex_mesh))
        info, index = sized("ctex_mesh_info"), capi.POINTER(capi.ctex_pick_index)()
        assert capi.ctex_mesh_get_info(pointer, capi.byref(info)) == SUCCESS
        assert capi.ctex_pick_index_create(pointer, capi.byref(index)) == SUCCESS
        summary.update(project_and_spray(capi, pointer, index, info.revision))
        capi.ctex_pick_index_destroy(index)
    summary.update(filter_and_pad(capi))
    summary.update(defer_dilation(capi))
    summary.update(preview_tiles(capi))
    arguments.output.mkdir(parents=True, exist_ok=True)
    (arguments.output / "summary.json").write_text(
        json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
