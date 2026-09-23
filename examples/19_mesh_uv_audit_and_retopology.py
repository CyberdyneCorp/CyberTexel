#!/usr/bin/env python3
"""Audit a scan's UV sets, derive texture sets from it and stage a retopology.

Capabilities: mesh-and-texture-sets.
A scanned mesh arrives with two UV sets: the raw ``scan`` layout the scanner
produced and a hand-packed ``packed`` layout. The example measures both, derives
one texture set per mesh partition from the layout that survived the audit, packs
those sets into an atlas, picks it back by face material, and stages a
retopologised replacement that carries a declared tangent frame, so every
affected texture set gets an explicit policy before any mesh is published.
"""

from __future__ import annotations

import argparse
import ctypes
import json
from pathlib import Path

import cybertexel


CAPABILITIES = ("mesh-and-texture-sets",)

CAPI = cybertexel.capi
OK = CAPI.CTEX_RESULT_SUCCESS
BYREF = CAPI.byref  # the generated declarations take pointers, not Python values
TEXT = CAPI.String  # every const char* argument is a NUL-terminated byte string

POSITIONS = ((0.0, 0.0, 0.0), (1.0, 0.0, 0.0), (0.0, 1.0, 0.0),
             (0.1, 0.1, 0.0), (0.7, 0.1, 0.0), (0.1, 0.7, 0.0),
             (2.0, 0.0, 0.0), (3.0, 0.0, 0.0), (2.0, 1.0, 0.0))
# The scanner's layout: the second body island sits on top of the first.
SCAN_UV = ((0.0, 0.0), (1.0, 0.0), (0.0, 1.0),
           (0.1, 0.1), (0.7, 0.1), (0.1, 0.7),
           (0.0, 0.0), (1.0, 0.0), (0.0, 1.0))
# The hand-packed layout: the two body islands only touch at one corner.
PACKED_UV = ((0.0, 0.0), (0.5, 0.0), (0.0, 0.5),
             (0.5, 0.5), (1.0, 0.5), (0.5, 1.0),
             (0.0, 0.0), (1.0, 0.0), (0.0, 1.0))
# The retopology moves both body islands and leaves the trim island alone.
RETOPOLOGY_UV = ((0.0, 0.0), (0.45, 0.0), (0.0, 0.45),
                 (0.55, 0.55), (1.0, 0.55), (0.55, 1.0),
                 (0.0, 0.0), (1.0, 0.0), (0.0, 1.0))
APPLY_SIZE = CAPI.CTEX_MESH_REPLACEMENT_APPLY_INFO_CURRENT_SIZE
COVERAGE_SIZE = CAPI.CTEX_MESH_UV_COVERAGE_INFO_CURRENT_SIZE
VERTICES = 9
FACES = 3
SET_IDS, ATLAS_IDS = CAPI.ctex_document_get_texture_set_ids, CAPI.ctex_document_get_atlas_ids
BODY_ID = "material/4:body/uv/6:packed"
TRIM_ID = "material/4:trim/uv/6:packed"


def expect(result: int, wanted: int = OK) -> None:
    assert result == wanted, f"expected C ABI result {wanted}, got {result}"


def unpack(buffer: object, size: int) -> list[str]:
    return [part.decode("utf-8") for part in buffer.raw[:size].split(b"\0") if part]


def at(buffer: object, offset: int, size: int) -> str:
    return buffer.raw[offset : offset + size - 1].decode("utf-8")


def sized(structure: object, size: int) -> object:
    structure.size = size  # every info struct declares the layout the caller compiled against
    return structure


class MeshArrays:
    """Holds every array a mesh descriptor points at for the descriptor's life."""

    def __init__(self, uv: tuple[tuple[float, float], ...]) -> None:
        self.positions = (CAPI.ctex_vec3f * VERTICES)(*(CAPI.ctex_vec3f(*v) for v in POSITIONS))
        self.normals = (CAPI.ctex_vec3f * VERTICES)(*(CAPI.ctex_vec3f(0, 0, 1),) * VERTICES)
        self.scan = (CAPI.ctex_vec2f * VERTICES)(*(CAPI.ctex_vec2f(*v) for v in SCAN_UV))
        self.packed = (CAPI.ctex_vec2f * VERTICES)(*(CAPI.ctex_vec2f(*v) for v in uv))
        self.indices = (CAPI.uint32_t * VERTICES)(*range(VERTICES))
        self.face_partitions = (CAPI.uint32_t * FACES)(0, 0, 1)
        self.face_materials = (CAPI.uint32_t * FACES)(1, 1, 2)
        uv_size = CAPI.CTEX_UV_SET_DESCRIPTOR_CURRENT_SIZE
        self.uv_sets = (CAPI.ctex_uv_set_descriptor * 2)(
            CAPI.ctex_uv_set_descriptor(uv_size, TEXT(b"scan"), self.scan, VERTICES),
            CAPI.ctex_uv_set_descriptor(uv_size, TEXT(b"packed"), self.packed, VERTICES))
        part_size = CAPI.CTEX_MESH_PARTITION_DESCRIPTOR_CURRENT_SIZE
        material = CAPI.CTEX_PARTITION_SOURCE_MATERIAL
        self.partitions = (CAPI.ctex_mesh_partition_descriptor * 2)(
            CAPI.ctex_mesh_partition_descriptor(part_size, material, TEXT(b"body"), TEXT(b"Body")),
            CAPI.ctex_mesh_partition_descriptor(part_size, material, TEXT(b"trim"), TEXT(b"Trim")))
        self.descriptor = CAPI.ctex_mesh_descriptor(
            CAPI.CTEX_MESH_DESCRIPTOR_CURRENT_SIZE, self.positions, VERTICES, self.normals,
            VERTICES, None, 0, self.indices, VERTICES, self.uv_sets, 2, TEXT(b"scan"),
            self.partitions, 2, self.face_partitions, FACES, self.face_materials, FACES)


def mesh_revision(mesh: object) -> int:
    info = sized(CAPI.ctex_mesh_info(), CAPI.CTEX_MESH_INFO_CURRENT_SIZE)
    expect(CAPI.ctex_mesh_get_info(mesh, BYREF(info)))
    return int(info.revision)


def tangent_frame(mesh: object) -> tuple[object, bytes]:
    """The frame a mesh retains: generated for it, or supplied alongside it."""
    info = sized(CAPI.ctex_mesh_tangent_frame_info(),
                 CAPI.CTEX_MESH_TANGENT_FRAME_INFO_CURRENT_SIZE)
    expect(CAPI.ctex_mesh_get_tangent_frame(mesh, BYREF(info), None, 0))
    uv_set = CAPI.create_string_buffer(info.required_uv_set_size)
    expect(CAPI.ctex_mesh_get_tangent_frame(mesh, BYREF(info), uv_set, len(uv_set)))
    return info, uv_set.value


def read_uv_set_names(mesh: object) -> list[str]:
    """Two-call sizing: ask with NULL, then read into the reported buffer."""
    required, count = CAPI.c_size_t(), CAPI.c_size_t()
    expect(CAPI.ctex_mesh_get_uv_set_names(mesh, None, 0, BYREF(required), BYREF(count)))
    assert (required.value, count.value) == (len(b"scan\0packed\0"), 2)
    names = CAPI.create_string_buffer(required.value)
    expect(CAPI.ctex_mesh_get_uv_set_names(mesh, names, len(names), BYREF(required),
                                           BYREF(count)))
    assert names.raw == b"scan\0packed\0"
    return unpack(names, required.value)


def audit_uv_set(mesh: object, uv_set: bytes) -> dict[str, object]:
    """Island overlap and unit-square coverage for the body partition."""
    name = TEXT(uv_set)
    overlaps = sized(CAPI.ctex_mesh_uv_overlap_info(), CAPI.CTEX_MESH_UV_OVERLAP_INFO_CURRENT_SIZE)
    expect(CAPI.ctex_mesh_analyze_uv_overlaps(mesh, name, 0, BYREF(overlaps), None, 0))
    faces = (CAPI.uint32_t * overlaps.required_face_count)()
    if overlaps.required_face_count:
        expect(CAPI.ctex_mesh_analyze_uv_overlaps(mesh, name, 0, BYREF(overlaps), faces, 0),
               CAPI.CTEX_RESULT_BUFFER_TOO_SMALL)
        expect(CAPI.ctex_mesh_analyze_uv_overlaps(mesh, name, 0, BYREF(overlaps), faces,
                                                  len(faces)))
    coverage = sized(CAPI.ctex_mesh_uv_coverage_info(), COVERAGE_SIZE)
    expect(CAPI.ctex_mesh_analyze_uv_coverage(mesh, name, 0, 4, 4, BYREF(coverage), None, 0))
    # Covered and uncovered are exact for the requested grid; the tested count is
    # a broad-phase work counter and is deliberately not the grid size.
    assert coverage.covered_texel_count + coverage.uncovered_texel_count == 16
    assert (coverage.width, coverage.height, coverage.selected_face_count) == (4, 4, 2)
    assert coverage.required_outside_face_count == 0
    assert coverage.uncovered_fraction == coverage.uncovered_texel_count / 16.0
    return {
        "candidate_pairs": overlaps.candidate_pair_count,
        "covered_texels": coverage.covered_texel_count,
        "overlapping_faces": [int(value) for value in faces],
        "overlap_pairs": overlaps.overlap_pair_count,
        "tested_texels": coverage.tested_texel_count,
        "uncovered_fraction": coverage.uncovered_fraction,
    }


def refuse_impossible_audits(mesh: object) -> None:
    overlaps = sized(CAPI.ctex_mesh_uv_overlap_info(), CAPI.CTEX_MESH_UV_OVERLAP_INFO_CURRENT_SIZE)
    expect(CAPI.ctex_mesh_analyze_uv_overlaps(mesh, TEXT(b"absent"), 0, BYREF(overlaps), None, 0),
           CAPI.CTEX_RESULT_MISSING_RESOURCE)
    assert CAPI.ctex_get_last_diagnostic_code() == CAPI.CTEX_DIAGNOSTIC_MISSING_UV_SET
    scan = TEXT(b"scan")
    expect(CAPI.ctex_mesh_analyze_uv_overlaps(mesh, scan, 2, BYREF(overlaps), None, 0),
           CAPI.CTEX_RESULT_INVALID_ARGUMENT)
    coverage = sized(CAPI.ctex_mesh_uv_coverage_info(), COVERAGE_SIZE)
    measure = CAPI.ctex_mesh_analyze_uv_coverage
    expect(measure(mesh, scan, 0, 0, 4, BYREF(coverage), None, 0),
           CAPI.CTEX_RESULT_INVALID_ARGUMENT)
    expect(measure(mesh, scan, 0, 16385, 16384, BYREF(coverage), None, 0),
           CAPI.CTEX_RESULT_OVER_BUDGET)
    assert b"268435456" in CAPI.ctex_get_last_diagnostic()


def listed(query: object, document: object) -> list[str]:
    """Both document listings share one two-call sizing contract."""
    required, count = CAPI.c_size_t(), CAPI.c_size_t()
    expect(query(document, None, 0, BYREF(required), BYREF(count)))
    buffer = CAPI.create_string_buffer(max(required.value, 1))
    expect(query(document, buffer, len(buffer), BYREF(required), BYREF(count)))
    identifiers = unpack(buffer, required.value)
    assert len(identifiers) == count.value
    return identifiers


def derive_texture_sets(document: object, mesh: object) -> list[str]:
    derive = CAPI.ctex_document_create_texture_sets_from_mesh
    expect(derive(document, mesh, TEXT(b"absent"), 8, 8, 16), CAPI.CTEX_RESULT_MISSING_RESOURCE)
    assert CAPI.ctex_get_last_diagnostic_code() == CAPI.CTEX_DIAGNOSTIC_MISSING_UV_SET
    assert listed(SET_IDS, document) == [], "a refused derivation created a texture set"
    expect(derive(document, mesh, TEXT(b"packed"), 8, 8, 16))
    identifiers = listed(SET_IDS, document)
    assert identifiers == [BODY_ID, TRIM_ID], identifiers
    return identifiers


def atlas_descriptor(identifiers: list[str], second_x: int) -> tuple[object, object]:
    region_size = CAPI.CTEX_ATLAS_REGION_DESCRIPTOR_CURRENT_SIZE
    regions = (CAPI.ctex_atlas_region_descriptor * 2)(
        CAPI.ctex_atlas_region_descriptor(region_size, TEXT(identifiers[0].encode()), 0, 0, 8, 8),
        CAPI.ctex_atlas_region_descriptor(region_size, TEXT(identifiers[1].encode()), second_x, 0,
                                          8, 8))
    descriptor = CAPI.ctex_atlas_descriptor(CAPI.CTEX_ATLAS_DESCRIPTOR_CURRENT_SIZE,
                                            TEXT(b"props"), TEXT(b"Props"), 16, 8, regions, 2)
    return regions, descriptor


def pack_atlas(document: object, identifiers: list[str]) -> dict[str, object]:
    _overlapping, invalid = atlas_descriptor(identifiers, 4)
    expect(CAPI.ctex_document_create_atlas(document, BYREF(invalid)),
           CAPI.CTEX_RESULT_INVALID_ARGUMENT)
    assert b"atlas regions overlap" in CAPI.ctex_get_last_diagnostic()
    assert listed(ATLAS_IDS, document) == [], "a refused atlas was published"

    _regions, valid = atlas_descriptor(identifiers, 8)
    expect(CAPI.ctex_document_create_atlas(document, BYREF(valid)))
    assert listed(ATLAS_IDS, document) == ["props"]

    name = TEXT(b"props")
    info = sized(CAPI.ctex_atlas_info(), CAPI.CTEX_ATLAS_INFO_CURRENT_SIZE)
    expect(CAPI.ctex_document_get_atlas(document, name, BYREF(info), None, 0, None, 0, None, 0))
    assert (info.width, info.height, info.region_count) == (16, 8, 2)
    assert info.required_display_name_size == len(b"Props\0")
    placed = (CAPI.ctex_atlas_region * 2)()
    placed[0].texture_set_id_offset = 77
    display = CAPI.create_string_buffer(info.required_display_name_size)
    packed = CAPI.create_string_buffer(info.required_texture_set_id_size)
    expect(CAPI.ctex_document_get_atlas(document, name, BYREF(info), placed, 1, display,
                                        len(display), packed, len(packed)),
           CAPI.CTEX_RESULT_BUFFER_TOO_SMALL)
    assert placed[0].texture_set_id_offset == 77 and display.raw[0] == 0
    expect(CAPI.ctex_document_get_atlas(document, name, BYREF(info), placed, 2, display,
                                        len(display), packed, len(packed)))
    assert display.value == b"Props"
    assert [(region.x, region.width) for region in placed] == [(0, 8), (8, 8)]
    resolved = [
        at(packed, region.texture_set_id_offset, region.texture_set_id_size) for region in placed
    ]
    assert resolved == identifiers
    expect(CAPI.ctex_document_get_atlas(document, TEXT(b"absent"), BYREF(info), None, 0, None, 0,
                                        None, 0), CAPI.CTEX_RESULT_MISSING_RESOURCE)
    return {"identifier": "props", "regions": resolved, "size": [16, 8]}


def stage_retopology(document: object, mesh: object, replacement: object) -> dict[str, object]:
    """One plan, one decision per changed set, nothing published until then."""
    tangents = (CAPI.ctex_vec4f * VERTICES)(*(CAPI.ctex_vec4f(1.0, 0.0, 0.0, 1.0),) * VERTICES)
    frame = CAPI.ctex_tangent_frame_descriptor(
        CAPI.CTEX_TANGENT_FRAME_DESCRIPTOR_CURRENT_SIZE, CAPI.CTEX_TANGENT_BASIS_UV_DERIVATIVE, 1,
        CAPI.CTEX_TANGENT_NORMAL_VERTEX, CAPI.CTEX_COORDINATE_RIGHT_HANDED,
        CAPI.CTEX_UV_V_AXIS_UPWARD, CAPI.CTEX_TANGENT_HANDEDNESS_W_SIGN, TEXT(b"packed"))
    tangent_data = CAPI.ctex_mesh_tangent_data_descriptor(
        CAPI.CTEX_MESH_TANGENT_DATA_DESCRIPTOR_CURRENT_SIZE, frame, tangents, VERTICES)
    generated, source_uv = tangent_frame(mesh)
    assert (generated.source, source_uv) == (CAPI.CTEX_TANGENT_FRAME_GENERATED, b"scan")
    before = mesh_revision(mesh)
    plan = ctypes.POINTER(CAPI.ctex_mesh_replacement_plan)()
    stage = CAPI.ctex_mesh_replacement_plan_create_with_tangent_data
    # Every supplied tangent is read: one along the vertex normal is refused outright.
    flat = (CAPI.ctex_vec4f * VERTICES)(*(CAPI.ctex_vec4f(0.0, 0.0, 1.0, 1.0),) * VERTICES)
    tangent_data.corner_tangents = flat
    expect(stage(document, mesh, BYREF(replacement), BYREF(tangent_data), BYREF(plan)),
           CAPI.CTEX_RESULT_INVALID_ARGUMENT)
    assert b"orthogonal to its vertex normal" in CAPI.ctex_get_last_diagnostic()
    tangent_data.corner_tangents = tangents
    expect(stage(document, mesh, BYREF(replacement), BYREF(tangent_data), BYREF(plan)))
    info = sized(CAPI.ctex_mesh_replacement_plan_info(),
                 CAPI.CTEX_MESH_REPLACEMENT_PLAN_INFO_CURRENT_SIZE)
    expect(CAPI.ctex_mesh_replacement_plan_get_info(plan, BYREF(info), None, 0, None, 0))
    assert info.source_mesh_revision == before
    assert (info.texture_set_count, info.changed_texture_set_count) == (2, 1)
    entries = (CAPI.ctex_mesh_replacement_entry * info.texture_set_count)()
    names = CAPI.create_string_buffer(info.required_texture_set_id_size)
    expect(CAPI.ctex_mesh_replacement_plan_get_info(plan, BYREF(info), entries, len(entries),
                                                    names, len(names)))
    changed = {at(names, item.texture_set_id_offset, item.texture_set_id_size): item
               for item in entries}
    assert changed[BODY_ID].uv_change == CAPI.CTEX_MESH_UV_CHANGED
    assert changed[TRIM_ID].uv_change == CAPI.CTEX_MESH_UV_UNCHANGED
    assert changed[BODY_ID].source_face_count == changed[BODY_ID].replacement_face_count == 2
    assert changed[TRIM_ID].source_partition_index == 1

    decisions = (CAPI.ctex_mesh_replacement_decision * 1)(CAPI.ctex_mesh_replacement_decision(
        CAPI.CTEX_MESH_REPLACEMENT_DECISION_CURRENT_SIZE, TEXT(BODY_ID.encode()),
        CAPI.CTEX_MESH_REPLACEMENT_REQUEST_REPROJECTION))
    applied = sized(CAPI.ctex_mesh_replacement_apply_info(), APPLY_SIZE)
    expect(CAPI.ctex_mesh_replacement_plan_apply(plan, decisions, 1, BYREF(applied)))
    assert applied.replacement_applied == 0 and applied.reprojection_pending_texture_set_count == 1
    assert mesh_revision(mesh) == before, "a pending reprojection published the mesh"

    decisions[0].policy = CAPI.CTEX_MESH_REPLACEMENT_CLEAR
    applied.size = APPLY_SIZE
    expect(CAPI.ctex_mesh_replacement_plan_apply(plan, decisions, 1, BYREF(applied)))
    assert applied.replacement_applied == 1
    assert (applied.cleared_texture_set_count, applied.kept_texture_set_count) == (1, 1)
    published = mesh_revision(mesh)
    assert published == applied.replacement_mesh_revision > before
    supplied, uv_set = tangent_frame(mesh)
    # The replacement published the declared frame, one tangent per triangle corner.
    assert (supplied.source, supplied.algorithm, supplied.algorithm_version) == (
        CAPI.CTEX_TANGENT_FRAME_SUPPLIED, CAPI.CTEX_TANGENT_BASIS_UV_DERIVATIVE, 1)
    assert (supplied.uv_v_axis, uv_set, supplied.corner_tangent_count) == (
        CAPI.CTEX_UV_V_AXIS_UPWARD, b"packed", 3 * FACES)
    CAPI.ctex_mesh_replacement_plan_destroy(plan)
    return {"changed": BODY_ID, "cleared": applied.cleared_texture_set_count, "revision": published}


def refuse_stale_plan(document: object, mesh: object, arrays: MeshArrays) -> int:
    """A plan is bound to the mesh revision it was computed against."""
    plan = ctypes.POINTER(CAPI.ctex_mesh_replacement_plan)()
    expect(CAPI.ctex_mesh_replacement_plan_create(document, mesh, BYREF(arrays.descriptor),
                                                  BYREF(plan)))
    expect(CAPI.ctex_mesh_replace(mesh, BYREF(arrays.descriptor)))
    replaced = mesh_revision(mesh)
    applied = sized(CAPI.ctex_mesh_replacement_apply_info(), APPLY_SIZE)
    expect(CAPI.ctex_mesh_replacement_plan_apply(plan, None, 0, BYREF(applied)),
           CAPI.CTEX_RESULT_INVALID_ARGUMENT)
    assert b"stale" in CAPI.ctex_get_last_diagnostic()
    CAPI.ctex_mesh_replacement_plan_destroy(plan)

    arrays.descriptor.default_uv_set = TEXT(b"absent")
    expect(CAPI.ctex_mesh_replace(mesh, BYREF(arrays.descriptor)),
           CAPI.CTEX_RESULT_INVALID_ARGUMENT)
    assert CAPI.ctex_get_last_diagnostic_code() == CAPI.CTEX_DIAGNOSTIC_INVALID_MESH
    arrays.descriptor.default_uv_set = TEXT(b"scan")
    assert mesh_revision(mesh) == replaced, "a refused replace published the mesh"
    return replaced


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--executor", default="cpu")
    arguments = parser.parse_args()

    source = MeshArrays(PACKED_UV)
    retopology = MeshArrays(RETOPOLOGY_UV)
    mesh = ctypes.POINTER(CAPI.ctex_mesh)()
    expect(CAPI.ctex_mesh_create(BYREF(source.descriptor), BYREF(mesh)))
    document = ctypes.POINTER(CAPI.ctex_document)()
    expect(CAPI.ctex_document_create(BYREF(document)))
    try:
        info = sized(CAPI.ctex_mesh_info(), CAPI.CTEX_MESH_INFO_CURRENT_SIZE)
        expect(CAPI.ctex_mesh_get_info(mesh, BYREF(info)))
        assert (info.vertex_count, info.triangle_count) == (VERTICES, FACES)
        assert (info.uv_set_count, info.partition_count) == (2, 2)
        assert info.has_vertex_colors == 0 and info.revision != 0

        assert read_uv_set_names(mesh) == ["scan", "packed"]
        scan = audit_uv_set(mesh, b"scan")
        packed = audit_uv_set(mesh, b"packed")
        # The scan layout stacks both body islands. The packed layout only meets
        # at a corner: a broad-phase candidate, but not a positive-area overlap.
        assert (scan["overlap_pairs"], scan["overlapping_faces"]) == (1, [0, 1])
        assert (packed["candidate_pairs"], packed["overlap_pairs"]) == (1, 0)
        assert packed["overlapping_faces"] == []
        assert (scan["covered_texels"], packed["covered_texels"]) == (10, 6)
        assert (scan["tested_texels"], packed["tested_texels"]) == (17, 8)
        refuse_impossible_audits(mesh)

        identifiers = derive_texture_sets(document, mesh)
        # A UV pick reads the packed layout back through its face materials and normals.
        index = ctypes.POINTER(CAPI.ctex_uv_pick_index)()
        expect(CAPI.ctex_uv_pick_index_create(mesh, TEXT(b"packed"), BYREF(index)))
        hit = CAPI.ctex_pick_hit()
        query = sized(CAPI.ctex_pick_query_info(), CAPI.CTEX_PICK_QUERY_INFO_CURRENT_SIZE)
        for partition, uv, wanted in ((0, (0.1, 0.1), (0, 1)), (1, (0.7, 0.2), (2, 2))):
            binding = CAPI.ctex_pick_texture_set_binding_descriptor(
                CAPI.CTEX_PICK_TEXTURE_SET_BINDING_DESCRIPTOR_CURRENT_SIZE, partition,
                TEXT(b"packed"))
            expect(CAPI.ctex_pick_uv_query(index, CAPI.ctex_vec2f(*uv), BYREF(binding),
                                           BYREF(hit), None, 0, BYREF(query)))
            assert (hit.triangle_index, hit.material_id) == wanted
            assert hit.interpolated_normal.z == 1.0, "the pick interpolated the vertex normals"
        CAPI.ctex_uv_pick_index_destroy(index)
        atlas = pack_atlas(document, identifiers)
        staged = stage_retopology(document, mesh, retopology.descriptor)
        assert refuse_stale_plan(document, mesh, source) > staged["revision"]
    finally:
        CAPI.ctex_document_destroy(document)
        CAPI.ctex_mesh_destroy(mesh)

    arguments.output.mkdir(parents=True, exist_ok=True)
    summary = {
        "atlas": atlas,
        "capabilities": list(CAPABILITIES),
        "packed_audit": packed,
        "retopology": {"changed": staged["changed"], "cleared": staged["cleared"]},
        "scan_audit": scan,
        "texture_sets": identifiers,
        "uv_sets": ["scan", "packed"],
    }
    (arguments.output / "summary.json").write_text(
        json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )


if __name__ == "__main__":
    main()
