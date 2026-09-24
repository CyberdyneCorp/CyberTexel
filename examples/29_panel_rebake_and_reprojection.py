#!/usr/bin/env python3
"""Rebake a panel's mesh maps and reproject its paint onto a retopology.

Capabilities: mesh-maps, mesh-and-texture-sets, texture-document, editable-authoring.
A panel arrives with a hand-authored tangent frame, so it is created and re-published
with the frame it declares. A host-owned baker produces its occlusion map, which
CyberTexel validates, copies and accounts for but never bakes, while an asynchronous
session cancels what the artist abandoned, versions a settings change and takes it
back. Then the retopology lands: every destination texel and the seam the artist
painted are preflighted into an inspectable mapping before the mesh is published.
"""

from __future__ import annotations

import argparse
import ctypes
import json
from pathlib import Path

import cybertexel
import numpy as np


CAPABILITIES = ("mesh-maps", "mesh-and-texture-sets", "texture-document", "editable-authoring")
CAPI = cybertexel.capi
OK = CAPI.CTEX_RESULT_SUCCESS
BYREF, TEXT, BUFFER = CAPI.byref, CAPI.String, CAPI.create_string_buffer  # pointers, not values
AMBIENT, CURVATURE, THICKNESS = (CAPI.CTEX_MESH_MAP_AMBIENT_OCCLUSION,
                                 CAPI.CTEX_MESH_MAP_CURVATURE, CAPI.CTEX_MESH_MAP_THICKNESS)
UINT8, SUPPLIED = CAPI.CTEX_TRANSPORT_COMPONENT_UINT8_UNORM, CAPI.CTEX_TANGENT_FRAME_SUPPLIED
UP, DOWN = CAPI.CTEX_UV_V_AXIS_UPWARD, CAPI.CTEX_UV_V_AXIS_DOWNWARD
SET_ID = b"material/5:panel/uv/3:uv0"
EXTENT, VERTICES, FACES, CORNERS = 8, 4, 2, 6
POSITIONS = ((0.0, 0.0, 0.0), (1.0, 0.0, 0.0), (1.0, 1.0, 0.0), (0.0, 1.0, 0.0))
SOURCE_UV = ((0.0, 0.0), (1.0, 0.0), (1.0, 1.0), (0.0, 1.0))
RETOPOLOGY_UV = ((0.1, 0.1), (0.9, 0.1), (0.9, 0.9), (0.1, 0.9))  # re-packed inside a gutter
TRIANGLES = (0, 1, 2, 0, 2, 3)
SUMMARY: dict[str, object] = {"capabilities": list(CAPABILITIES)}


def expect(result: int, wanted: int = OK) -> None:
    assert result == wanted, f"expected C ABI result {wanted}, got {result}"


def sized(struct: str, /, **fields: object) -> object:
    """Allocate a versioned ABI struct with its CURRENT_SIZE and named fields."""
    value = getattr(CAPI, struct)()
    value.size = getattr(CAPI, f"{struct.upper()}_CURRENT_SIZE")
    for field, item in fields.items():
        setattr(value, field, item)
    return value


class Panel:
    """Owns every array a mesh descriptor points at for the descriptor's life."""
    def __init__(self) -> None:
        self.positions = (CAPI.ctex_vec3f * VERTICES)(*(CAPI.ctex_vec3f(*p) for p in POSITIONS))
        self.normals = (CAPI.ctex_vec3f * VERTICES)(*(CAPI.ctex_vec3f(0, 0, 1),) * VERTICES)
        self.uv = (CAPI.ctex_vec2f * VERTICES)(*(CAPI.ctex_vec2f(*v) for v in SOURCE_UV))
        self.indices = (CAPI.uint32_t * len(TRIANGLES))(*TRIANGLES)
        self.faces = (CAPI.uint32_t * FACES)(0, 0), (CAPI.uint32_t * FACES)(1, 1)
        self.uv_sets = (CAPI.ctex_uv_set_descriptor * 1)(CAPI.ctex_uv_set_descriptor(
            CAPI.CTEX_UV_SET_DESCRIPTOR_CURRENT_SIZE, TEXT(b"uv0"), self.uv, VERTICES))
        self.partitions = (CAPI.ctex_mesh_partition_descriptor * 1)(
            CAPI.ctex_mesh_partition_descriptor(CAPI.CTEX_MESH_PARTITION_DESCRIPTOR_CURRENT_SIZE,
                                                CAPI.CTEX_PARTITION_SOURCE_MATERIAL,
                                                TEXT(b"panel"), TEXT(b"Panel")))
        self.descriptor = CAPI.ctex_mesh_descriptor(
            CAPI.CTEX_MESH_DESCRIPTOR_CURRENT_SIZE, self.positions, VERTICES, self.normals,
            VERTICES, None, 0, self.indices, len(TRIANGLES), self.uv_sets, 1, TEXT(b"uv0"),
            self.partitions, 1, self.faces[0], FACES, self.faces[1], FACES)
        self.tangents = (CAPI.ctex_vec4f * CORNERS)(*(CAPI.ctex_vec4f(1, 0, 0, 1),) * CORNERS)
        self.frame = CAPI.ctex_tangent_frame_descriptor(
            CAPI.CTEX_TANGENT_FRAME_DESCRIPTOR_CURRENT_SIZE, CAPI.CTEX_TANGENT_BASIS_UV_DERIVATIVE,
            1, CAPI.CTEX_TANGENT_NORMAL_VERTEX, CAPI.CTEX_COORDINATE_RIGHT_HANDED, UP,
            CAPI.CTEX_TANGENT_HANDEDNESS_W_SIGN, TEXT(b"uv0"))
        self.tangent_data = CAPI.ctex_mesh_tangent_data_descriptor(
            CAPI.CTEX_MESH_TANGENT_DATA_DESCRIPTOR_CURRENT_SIZE, self.frame, self.tangents, CORNERS)


def mesh_revision(mesh: object) -> int:
    info = sized("ctex_mesh_info")
    expect(CAPI.ctex_mesh_get_info(mesh, BYREF(info)))
    return int(info.revision)


def declared_frame(mesh: object) -> tuple[object, bytes]:
    """Two-call sizing: the frame first, then its UV-set name into the size it published."""
    info = sized("ctex_mesh_tangent_frame_info")
    expect(CAPI.ctex_mesh_get_tangent_frame(mesh, BYREF(info), None, 0))
    name = BUFFER(info.required_uv_set_size)
    expect(CAPI.ctex_mesh_get_tangent_frame(mesh, BYREF(info), name, len(name)))
    return info, name.value


def create_panel(panel: Panel) -> object:
    """A supplied frame survives creation, a refused replacement and republication."""
    mesh, replace = ctypes.POINTER(CAPI.ctex_mesh)(), CAPI.ctex_mesh_replace_with_tangent_data
    expect(CAPI.ctex_mesh_create_with_tangent_data(BYREF(panel.descriptor), BYREF(panel.tangent_data), BYREF(mesh)))
    created, uv_set = declared_frame(mesh)
    assert created.source == SUPPLIED and uv_set == b"uv0" and created.uv_v_axis == UP
    assert created.corner_tangent_count == CORNERS and created.required_uv_set_size == 4
    before = mesh_revision(mesh)
    panel.tangent_data.corner_tangent_count = CORNERS - 1
    expect(replace(mesh, BYREF(panel.descriptor), BYREF(panel.tangent_data)), CAPI.CTEX_RESULT_INVALID_ARGUMENT)
    assert mesh_revision(mesh) == before, "a refused tangent replacement published the mesh"
    # The modeller re-exports with a downward V axis; replacement retains what it declares.
    panel.tangent_data.corner_tangent_count, panel.frame.uv_v_axis = CORNERS, DOWN
    panel.tangent_data.frame = panel.frame
    expect(replace(mesh, BYREF(panel.descriptor), BYREF(panel.tangent_data)))
    republished, uv_set = declared_frame(mesh)
    assert republished.uv_v_axis == DOWN and uv_set == b"uv0" and republished.source == SUPPLIED
    assert mesh_revision(mesh) > before
    return mesh


def memory_report(document: object) -> object:
    """Enabling a channel declares intent; a constant channel allocates no resident tile."""
    report = sized("ctex_texture_set_memory_report")
    expect(CAPI.ctex_texture_set_get_memory_report(document, TEXT(SET_ID), BYREF(report)))
    # The total is the sum of its parts; `>=` would pass a double count.
    assert report.channel_pixel_bytes == 0, "a constant channel allocated a resident tile"
    assert report.total_resident_bytes == report.channel_pixel_bytes + report.mesh_map_pixel_bytes
    return report


def derive_texture_set(mesh: object) -> object:
    """One texture set per mesh partition, named by the library, with two channels enabled."""
    document, read = ctypes.POINTER(CAPI.ctex_document)(), CAPI.ctex_document_get_texture_set_ids
    expect(CAPI.ctex_document_create(BYREF(document)))
    expect(CAPI.ctex_document_create_texture_sets_from_mesh(document, mesh, TEXT(b"uv0"), EXTENT, EXTENT, 8))
    required, count = CAPI.c_size_t(), CAPI.c_size_t()
    expect(read(document, None, 0, BYREF(required), BYREF(count)))
    assert (count.value, required.value) == (1, len(SET_ID) + 1)
    identifiers = BUFFER(required.value)
    expect(read(document, identifiers, len(identifiers), BYREF(required), BYREF(count)))
    assert identifiers.value == SET_ID and memory_report(document).enabled_channel_count == 0
    for semantic, depth in ((b"pbr.base_color", 0), (b"pbr.height", 16)):
        expect(CAPI.ctex_texture_set_set_channel_enabled(document, TEXT(SET_ID), TEXT(semantic), 1, depth))
    info, mapping = sized("ctex_channel_info"), CAPI.c_size_t()
    expect(CAPI.ctex_texture_set_get_channel_info(document, TEXT(SET_ID), TEXT(b"pbr.height"),
                                                  BYREF(info), None, 0, BYREF(mapping)))
    assert info.enabled == 1 and info.storage_bit_depth == 16
    return document


def name_the_manifest() -> dict[str, str]:
    """A NULL first call publishes the size each stable mesh-map name needs."""
    read, names = CAPI.ctex_mesh_map_kind_get_name, {}
    for kind in (AMBIENT, CURVATURE, THICKNESS):
        required = CAPI.c_size_t()
        expect(read(kind, None, 0, BYREF(required)))
        name = BUFFER(required.value)
        expect(read(kind, name, len(name) - 1, BYREF(required)), CAPI.CTEX_RESULT_BUFFER_TOO_SMALL)
        expect(read(kind, name, len(name), BYREF(required)))
        assert len(name.value) + 1 == required.value == len(name)
        names[str(kind)] = name.value.decode()
    assert list(names.values()) == ["ambient-occlusion", "curvature", "thickness"]
    return names


class HostBaker:
    """A host-owned baker: CyberTexel validates and copies its output but never bakes."""
    def __init__(self, pixels: np.ndarray) -> None:
        self.pixels, self.requests, self.progress = pixels, [], []
        self._produce = CAPI.ctex_mesh_map_bake_can_produce_fn(
            lambda data, kind: 1 if kind in (AMBIENT, CURVATURE) else 0)
        self._request = CAPI.ctex_mesh_map_bake_request_fn(self.on_request)
        self._cancelled = CAPI.ctex_mesh_map_bake_is_cancelled_fn(lambda data: 0)
        self._progress = CAPI.ctex_mesh_map_bake_report_progress_fn(
            lambda data, fraction: self.progress.append(float(fraction)))
        self.provider = sized("ctex_mesh_map_bake_provider_descriptor", name=TEXT(b"example-baker"),
                              can_produce=self._produce, request=self._request)
        self.control = sized("ctex_mesh_map_bake_control_descriptor", is_cancelled=self._cancelled,
                             report_progress=self._progress)

    # The callback only records. A failed assertion here returns 0 to the library, and 0
    # is CTEX_RESULT_SUCCESS, so every claim about a request is made after the call ends.
    def on_request(self, data: object, request: object, control: object, out: object) -> int:
        item, driver, target = request.contents, control.contents, out.contents
        frame = item.tangent_frame.contents if item.tangent_frame else None
        self.requests.append((int(item.kind), item.texture_set_id.data.decode(),
                              item.uv_set.data.decode(), int(item.mesh_revision),
                              int(item.bake_settings_revision), int(item.request_generation),
                              (item.width, item.height), frame and frame.uv_set.data.decode(),
                              frame and int(frame.uv_v_axis)))
        driver.report_progress(driver.user_data, 0.5)
        target.buffer.width, target.buffer.height = item.width, item.height
        target.buffer.component_type, target.buffer.component_count = UINT8, 1
        target.buffer.row_stride_bytes = item.width
        target.buffer.pixels = self.pixels.ctypes.data_as(ctypes.c_void_p)
        target.buffer.pixel_bytes = self.pixels.nbytes
        return CAPI.CTEX_MESH_MAP_BAKE_PROVIDER_COMPLETED


def bake_occlusion(maps: object, baker: HostBaker, revision: int) -> None:
    """The library hands the baker its own request, then owns a copy of the answer."""
    info, bake = sized("ctex_mesh_map_bake_result_info"), CAPI.ctex_mesh_map_set_request_bake
    expect(bake(maps, BYREF(baker.provider), AMBIENT, EXTENT, EXTENT, 17, 23, BYREF(baker.control), BYREF(info)))
    assert info.status == CAPI.CTEX_MESH_MAP_BAKE_COMPLETED and info.has_binding == 1
    assert info.resolution_mismatch == 0 and info.stale == 0
    assert info.current_mesh_revision == info.produced_mesh_revision == revision
    # 0.0 and 1.0 are the library's own bracket around the 0.5 the provider reported, and
    # the frame it handed the baker is the one the mesh republished, not a default.
    assert baker.progress == [0.0, 0.5, 1.0] and len(baker.requests) == 1
    assert baker.requests[0] == (AMBIENT, SET_ID.decode(), "uv0", revision, 17, 23, (EXTENT, EXTENT), "uv0", DOWN)
    baker.pixels[:] = 0  # the binding kept its own copy of what the provider returned
    for (u, v), value in (((0.0, 1.0), 16), ((1.0, 0.0), 240)):
        sample = sized("ctex_mesh_map_sample_info")
        expect(CAPI.ctex_mesh_map_set_sample(maps, AMBIENT, u, v, BYREF(sample)))
        # x = u*(width-1) and y = (1-v)*(height-1), so these land on opposite corner texels.
        assert sample.component_count == 1 and sample.stale == 0
        assert abs(sample.values[0] - value / 255.0) < 1e-9
    expect(bake(maps, BYREF(baker.provider), THICKNESS, EXTENT, EXTENT, 17, 24,
                BYREF(baker.control), BYREF(info)), CAPI.CTEX_RESULT_UNSUPPORTED_OPERATION)
    assert len(baker.requests) == 1, "an unadvertised kind reached the provider"
    assert b"cannot produce 'thickness'" in CAPI.ctex_get_last_diagnostic()


def version_the_settings(maps: object) -> dict[str, int]:
    """Cancel what the artist abandoned, version the settings change, then take it back."""
    session, tokens = ctypes.POINTER(CAPI.ctex_mesh_map_bake_session)(), []
    expect(CAPI.ctex_mesh_map_bake_session_create(maps, 7, BYREF(session)))

    def begin(kind: int) -> object:
        tokens.append(ctypes.POINTER(CAPI.ctex_mesh_map_bake_request_token)())
        expect(CAPI.ctex_mesh_map_bake_session_begin(session, kind, 1, 1, BYREF(tokens[-1])))
        return tokens[-1]

    try:
        abandoned, cancelled = begin(THICKNESS), CAPI.uint32_t(0)
        expect(CAPI.ctex_mesh_map_bake_session_cancel(session, abandoned, BYREF(cancelled)))
        assert cancelled.value == 1
        expect(CAPI.ctex_mesh_map_bake_session_cancel(session, abandoned, BYREF(cancelled)))
        assert cancelled.value == 0, "a second cancel reported fresh work"
        begin(CURVATURE)  # still in flight when the artist changes the bake settings
        edit = sized("ctex_mesh_map_bake_settings_edit_info")
        expect(CAPI.ctex_mesh_map_bake_session_edit_settings(session, 8, BYREF(edit)))
        assert (edit.previous_revision, edit.current_revision) == (7, 8)
        assert edit.invalidated_request_count == 1, "the in-flight request outlived its settings"
        undo, state = (sized("ctex_mesh_map_bake_settings_undo_info"), sized("ctex_mesh_map_bake_session_info"))
        expect(CAPI.ctex_mesh_map_bake_session_undo_settings(session, BYREF(undo)))
        assert undo.restored == 1 and (undo.previous_revision, undo.restored_revision) == (8, 7)
        assert undo.restored_map_count == 1 and undo.invalidated_request_count == 0
        expect(CAPI.ctex_mesh_map_bake_session_get_info(session, BYREF(state)))
        # Neither request was completed, so the session still tracks both tokens it issued.
        assert state.settings_revision == 7 and state.pending_request_count == 2
        return {"settings_revision": int(state.settings_revision),
                "restored_map_count": int(undo.restored_map_count),
                "invalidated_by_edit": int(edit.invalidated_request_count)}
    finally:
        for token in tokens:
            CAPI.ctex_mesh_map_bake_request_token_destroy(token)
        CAPI.ctex_mesh_map_bake_session_destroy(session)


def attach_seam(document: object, revision: int) -> None:
    """A surface path the artist painted, named against the mesh revision it was painted on."""
    def point(x: float, y: float, triangle: int) -> object:
        return CAPI.ctex_editable_surface_point_descriptor(
            CAPI.CTEX_EDITABLE_SURFACE_POINT_DESCRIPTOR_CURRENT_SIZE, CAPI.ctex_vec3d(x, y, 0.0),
            CAPI.ctex_vec3d(0.0, 0.0, 1.0), triangle, (CAPI.c_double * 3)(0.5, 0.25, 0.25), 0.05)

    points = (CAPI.ctex_editable_surface_point_descriptor * 2)(point(0.6, 0.2, 0), point(0.4, 0.8, 1))
    tiles = (CAPI.ctex_editable_tile_dependency_descriptor * 1)(
        CAPI.ctex_editable_tile_dependency_descriptor(
            CAPI.CTEX_EDITABLE_TILE_DEPENDENCY_DESCRIPTOR_CURRENT_SIZE, TEXT(b"pbr.base_color"), 0, 0))
    entry = sized("ctex_editable_entry_descriptor", identifier=TEXT(b"panel-seam"),
                  kind=CAPI.CTEX_EDITABLE_ENTRY_SURFACE_PATH, expected_revision=0,
                  placement=CAPI.ctex_editable_placement_frame(
                      CAPI.ctex_vec3d(0.0, 0.0, 0.0), CAPI.ctex_vec3d(0.0, 0.0, 1.0), 0.0, 1.0,
                      CAPI.ctex_vec2d(1.0, 1.0)),
                  material_identity=TEXT(b"paint/seam"), mesh_revision=revision,
                  surface_points=points, surface_point_count=2, dependent_tiles=tiles,
                  dependent_tile_count=1)
    info = sized("ctex_editable_entry_info")
    expect(CAPI.ctex_texture_set_editable_entry_add(document, TEXT(SET_ID), BYREF(entry), BYREF(info), None, 0))
    assert info.surface_point_count == 2 and info.entry_count == 1 and info.entry_revision == 1


def retopologise(document: object, mesh: object, panel: Panel, channels: int) -> dict[str, object]:
    """One plan, one policy per changed set, and nothing published until reprojection."""
    for vertex, (u, v) in enumerate(RETOPOLOGY_UV):
        panel.uv[vertex] = CAPI.ctex_vec2f(u, v)
    plan, entries = ctypes.POINTER(CAPI.ctex_mesh_replacement_plan)(), (CAPI.ctex_mesh_replacement_entry * 1)()
    expect(CAPI.ctex_mesh_replacement_plan_create_with_tangent_data(
        document, mesh, BYREF(panel.descriptor), BYREF(panel.tangent_data), BYREF(plan)))
    names, info = BUFFER(len(SET_ID) + 1), sized("ctex_mesh_replacement_plan_info")
    expect(CAPI.ctex_mesh_replacement_plan_get_info(plan, BYREF(info), entries, 1, names, len(names)))
    assert (info.texture_set_count, info.changed_texture_set_count) == (1, 1)
    assert info.required_texture_set_id_size == len(names)
    assert entries[0].uv_change == CAPI.CTEX_MESH_UV_CHANGED
    decisions = (CAPI.ctex_mesh_replacement_decision * 1)(CAPI.ctex_mesh_replacement_decision(
        CAPI.CTEX_MESH_REPLACEMENT_DECISION_CURRENT_SIZE, TEXT(SET_ID),
        CAPI.CTEX_MESH_REPLACEMENT_REQUEST_REPROJECTION))
    applied = sized("ctex_mesh_replacement_apply_info")
    expect(CAPI.ctex_mesh_replacement_plan_apply(plan, decisions, 1, BYREF(applied)))
    assert applied.replacement_applied == 0 and applied.reprojection_pending_texture_set_count == 1
    try:
        return preflight_and_commit(plan, mesh, channels)
    finally:
        CAPI.ctex_mesh_replacement_plan_destroy(plan)


def preflight_and_commit(plan: object, mesh: object, channels: int) -> dict[str, object]:
    """Preflight is inspectable and mutates nothing; commit publishes in one operation."""
    seen: list[int] = []
    limits = sized("ctex_mesh_reprojection_descriptor", maximum_distance=1.0,
                   maximum_normal_angle_radians=0.5, require_visibility=1, visibility_epsilon=1e-5,
                   ambiguity_distance_epsilon=1e-9, maximum_work_items=100000, progress_interval=8,
                   is_cancelled=CAPI.ctex_mesh_reprojection_cancel_callback(lambda data: 1),
                   report_progress=CAPI.ctex_mesh_reprojection_progress_callback(
                       lambda completed, data: seen.append(int(completed))))
    look = CAPI.ctex_mesh_replacement_plan_preflight_reprojection
    before, preflight = mesh_revision(mesh), sized("ctex_mesh_reprojection_preflight_info")
    expect(look(plan, BYREF(limits), BYREF(preflight), None, 0), CAPI.CTEX_RESULT_CANCELLED)
    assert CAPI.ctex_get_last_diagnostic_code() == CAPI.CTEX_DIAGNOSTIC_INVALID_MESH_REPROJECTION
    assert mesh_revision(mesh) == before, "a cancelled preflight published the mesh"
    limits.is_cancelled = CAPI.ctex_mesh_reprojection_cancel_callback()
    expect(look(plan, BYREF(limits), BYREF(preflight), None, 0))
    assert preflight.source_mesh_revision == before < preflight.replacement_mesh_revision
    assert preflight.mapped_texel_count > 0 and preflight.unmapped_texel_count == 0
    assert preflight.affected_entry_count == 1, "the painted seam was not carried"
    assert preflight.tested_candidate_count > preflight.mapped_texel_count
    assert seen and seen[-1] == preflight.tested_candidate_count
    mapped = int(preflight.mapped_texel_count)
    covered = mapped + int(preflight.ambiguous_texel_count)
    # The gutter leaves a 6x6 block of covered texel centres; those on the shared diagonal
    # see two equidistant source triangles, so the commit needs an ambiguity policy.
    assert covered == 36 and preflight.ambiguous_texel_count > 0
    mapping = BUFFER(preflight.required_mapping_json_size)
    expect(look(plan, BYREF(limits), BYREF(preflight), mapping, len(mapping) - 1), CAPI.CTEX_RESULT_BUFFER_TOO_SMALL)
    expect(look(plan, BYREF(limits), BYREF(preflight), mapping, len(mapping)))
    inspected = json.loads(mapping.value.decode())
    statuses = [texel["status"] for texel in inspected["texels"]]
    assert statuses.count("mapped") == mapped and len(mapping.value) + 1 == len(mapping)
    assert [entry["entry_id"] for entry in inspected["affected_entries"]] == ["panel-seam"]
    assert mesh_revision(mesh) == before, "an inspected preflight published the mesh"
    commit = sized("ctex_mesh_reprojection_commit_info")
    expect(CAPI.ctex_mesh_replacement_plan_commit_reprojection(
        plan, CAPI.CTEX_MESH_REPROJECTION_RETAIN_TARGET,
        CAPI.CTEX_MESH_REPROJECTION_NEAREST_LOWEST_TRIANGLE, BYREF(commit)))
    # Commit stages every enabled channel, so it counts each covered texel once per channel.
    assert commit.reprojected_texel_count == channels * covered
    assert commit.resolved_ambiguity_count == channels * preflight.ambiguous_texel_count
    assert commit.retained_hole_count == 0 and commit.defaulted_hole_count == 0
    assert commit.reprojected_entry_count == 1, "the painted seam was not carried over"
    assert mesh_revision(mesh) == commit.replacement_mesh_revision > before
    assert declared_frame(mesh)[0].source == SUPPLIED
    return {"covered_texels": covered, "unmapped_texels": int(preflight.unmapped_texel_count),
            "reprojected_entries": int(commit.reprojected_entry_count),
            "retained_holes": int(commit.retained_hole_count), "channels_staged": channels}


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--executor", default="cpu")
    arguments = parser.parse_args()
    panel = Panel()
    baked = np.linspace(16, 240, EXTENT * EXTENT, dtype=np.uint8).reshape(EXTENT, EXTENT)
    mesh = create_panel(panel)
    document = derive_texture_set(mesh)
    maps = ctypes.POINTER(CAPI.ctex_mesh_map_set)()
    try:
        SUMMARY["map_kind_names"] = name_the_manifest()
        channels = int(memory_report(document).enabled_channel_count)
        expect(CAPI.ctex_mesh_map_set_create(document, TEXT(SET_ID), mesh, BYREF(maps)))
        baker = HostBaker(baked.copy())
        bake_occlusion(maps, baker, mesh_revision(mesh))
        occlusion = memory_report(document)
        # Residency is tile-granular: 64 baked bytes cost one whole tile.
        assert occlusion.mesh_map_pixel_bytes > EXTENT * EXTENT
        SUMMARY["bake_session"] = version_the_settings(maps)
        bound, info = memory_report(document), sized("ctex_mesh_map_set_info")
        expect(CAPI.ctex_mesh_map_set_get_info(maps, BYREF(info), None, 0, None, 0))
        assert (info.bound_map_count, info.texture_set_width) == (1, EXTENT)
        assert bound.mesh_map_pixel_bytes == info.resident_pixel_bytes == occlusion.mesh_map_pixel_bytes
        SUMMARY["mesh_map_pixel_bytes"] = int(bound.mesh_map_pixel_bytes)
        attach_seam(document, mesh_revision(mesh))
        SUMMARY["reprojection"] = retopologise(document, mesh, panel, channels)
        released = sized("ctex_mesh_map_release_info")
        expect(CAPI.ctex_mesh_map_set_release_all(maps, BYREF(released)))
        assert released.resident_pixel_bytes_released == bound.mesh_map_pixel_bytes
        assert memory_report(document).mesh_map_pixel_bytes == 0
    finally:
        CAPI.ctex_mesh_map_set_destroy(maps)
        CAPI.ctex_document_destroy(document)
        CAPI.ctex_mesh_destroy(mesh)

    arguments.output.mkdir(parents=True, exist_ok=True)
    (arguments.output / "baked_occlusion.png").write_bytes(
        cybertexel.encode_image(np.repeat(np.repeat(baked, 16, axis=0), 16, axis=1)))
    (arguments.output / "summary.json").write_text(
        json.dumps(SUMMARY, indent=2, sort_keys=True) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
