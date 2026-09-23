#include <ctex/capi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int expect(int condition, const char* message) {
    if (!condition) {
        fprintf(stderr, "%s: %s\n", message, ctex_get_last_diagnostic());
        return 0;
    }
    return 1;
}

static int make_document(ctex_document** document, char* texture_set_id, size_t id_size) {
    ctex_texture_set_descriptor set = {0};
    size_t required = 0;
    size_t count = 0;
    set.size = CTEX_TEXTURE_SET_DESCRIPTOR_CURRENT_SIZE;
    set.display_name = "Body";
    set.partition_kind = CTEX_PARTITION_SOURCE_MATERIAL;
    set.partition_key = "body";
    set.uv_set = "uv0";
    set.width = 128;
    set.height = 128;
    set.default_bit_depth = 8;
    if (!expect(ctex_document_create(document) == CTEX_RESULT_SUCCESS, "create document") ||
        !expect(ctex_document_create_texture_set(*document, &set) == CTEX_RESULT_SUCCESS,
                "create texture set") ||
        !expect(ctex_document_get_texture_set_ids(*document, NULL, 0, &required, &count) ==
                    CTEX_RESULT_SUCCESS,
                "size texture set id") ||
        !expect(required <= id_size, "texture set id capacity") ||
        !expect(ctex_document_get_texture_set_ids(*document, texture_set_id, id_size, &required,
                                                  &count) == CTEX_RESULT_SUCCESS,
                "read texture set id")) {
        return 0;
    }
    return 1;
}

static ctex_editable_entry_descriptor text_descriptor(
    const char* text, uint64_t expected_revision,
    const ctex_editable_tile_dependency_descriptor* tiles, size_t tile_count,
    const ctex_editable_material_parameter_descriptor* parameter) {
    ctex_editable_entry_descriptor entry = {0};
    entry.size = CTEX_EDITABLE_ENTRY_DESCRIPTOR_CURRENT_SIZE;
    entry.identifier = "title";
    entry.kind = CTEX_EDITABLE_ENTRY_TEXT;
    entry.expected_revision = expected_revision;
    entry.placement.position = (ctex_vec3d){1.0, 2.0, 3.0};
    entry.placement.normal = (ctex_vec3d){0.0, 0.0, 1.0};
    entry.placement.rotation_radians = 0.25;
    entry.placement.uniform_scale = 2.0;
    entry.placement.axis_scale = (ctex_vec2d){1.0, 0.5};
    entry.material_identity = "paint/gold";
    entry.material_parameters = parameter;
    entry.material_parameter_count = 1;
    entry.text = text;
    entry.font_identity = "fonts/inter-bold-v4";
    entry.dependent_tiles = tiles;
    entry.dependent_tile_count = tile_count;
    return entry;
}

int main(void) {
    ctex_document* document = NULL;
    ctex_document* reopened = NULL;
    char texture_set_id[256] = {0};
    char reopened_set_id[256] = {0};
    char report[8192] = {0};
    ctex_editable_material_parameter_descriptor parameter = {0};
    ctex_editable_tile_dependency_descriptor text_tiles[2] = {{0}, {0}};
    ctex_editable_entry_info info = {0};
    ctex_editable_entry_descriptor text;
    ctex_editable_surface_point_descriptor points[2] = {{0}, {0}};
    ctex_editable_tile_dependency_descriptor path_tiles[2] = {{0}, {0}};
    ctex_editable_entry_descriptor path = {0};
    ctex_stroke_settings_descriptor stroke = {0};
    ctex_resolved_stroke_info stroke_info = {0};
    ctex_resolved_stamp stamps[64] = {0};
    ctex_swept_segment swept_segments[64] = {0};
    size_t stamp_count = 0;
    size_t segment_count = 0;
    ctex_project_container_info saved_info = {0};
    unsigned char* empty_project = NULL;
    unsigned char* saved_project = NULL;
    char* project_report = NULL;
    size_t empty_size = 0;

    if (!make_document(&document, texture_set_id, sizeof(texture_set_id))) goto fail;

    parameter.size = CTEX_EDITABLE_MATERIAL_PARAMETER_DESCRIPTOR_CURRENT_SIZE;
    parameter.identifier = "roughness";
    parameter.component_count = 1;
    parameter.value[0] = 0.35;
    text_tiles[0].size = CTEX_EDITABLE_TILE_DEPENDENCY_DESCRIPTOR_CURRENT_SIZE;
    text_tiles[0].semantic_id = "base-color";
    text_tiles[0].tile_x = 2;
    text_tiles[0].tile_y = 1;
    text_tiles[1] = text_tiles[0];
    text_tiles[1].tile_x = 3;

    text = text_descriptor("CyberTexel", 0, text_tiles, 1, &parameter);
    info.size = CTEX_EDITABLE_ENTRY_INFO_CURRENT_SIZE;
    if (!expect(ctex_texture_set_editable_entry_add(document, texture_set_id, &text, &info, report,
                                                    sizeof(report)) == CTEX_RESULT_SUCCESS,
                "add editable text") ||
        !expect(info.entry_revision == 1 && info.undo_step_count == 1 &&
                    strstr(report, "CyberTexel") != NULL,
                "inspect added text result")) {
        goto fail;
    }

    text = text_descriptor("CyberTexel 2", 1, text_tiles, 2, &parameter);
    info.size = CTEX_EDITABLE_ENTRY_INFO_CURRENT_SIZE;
    if (!expect(ctex_texture_set_editable_entry_edit(document, texture_set_id, &text, &info, report,
                                                     sizeof(report)) == CTEX_RESULT_SUCCESS,
                "edit editable text") ||
        !expect(info.entry_revision == 2 && info.invalidated_tile_count == 2,
                "edit invalidates old and new tiles")) {
        goto fail;
    }
    text.expected_revision = 2;
    text.text = "must-not-commit";
    info.size = CTEX_EDITABLE_ENTRY_INFO_CURRENT_SIZE;
    if (!expect(ctex_texture_set_editable_entry_edit(document, texture_set_id, &text, &info, report,
                                                     1) == CTEX_RESULT_BUFFER_TOO_SMALL,
                "short report atomically refuses edit")) {
        goto fail;
    }
    info.size = CTEX_EDITABLE_ENTRY_INFO_CURRENT_SIZE;
    if (!expect(
            ctex_texture_set_editable_entry_inspect(document, texture_set_id, "title", &info,
                                                    report, sizeof(report)) == CTEX_RESULT_SUCCESS,
            "inspect text after refused edit") ||
        !expect(info.entry_revision == 2 && strstr(report, "CyberTexel 2") != NULL &&
                    strstr(report, "must-not-commit") == NULL,
                "refused edit leaves entry unchanged")) {
        goto fail;
    }
    info.size = CTEX_EDITABLE_ENTRY_INFO_CURRENT_SIZE;
    if (!expect(ctex_texture_set_editable_entry_undo(document, texture_set_id, &info, report,
                                                     sizeof(report)) == CTEX_RESULT_SUCCESS,
                "undo editable text") ||
        !expect(info.entry_revision == 1 && strstr(report, "CyberTexel 2") == NULL,
                "undo restores original text")) {
        goto fail;
    }

    points[0].size = CTEX_EDITABLE_SURFACE_POINT_DESCRIPTOR_CURRENT_SIZE;
    points[0].position = (ctex_vec3d){0.0, 0.0, 0.0};
    points[0].normal = (ctex_vec3d){0.0, 0.0, 1.0};
    points[0].triangle = 7;
    points[0].barycentric[0] = 1.0;
    points[0].width = 0.5;
    points[1] = points[0];
    points[1].position.x = 2.0;
    points[1].triangle = 8;
    points[1].barycentric[0] = 0.0;
    points[1].barycentric[1] = 1.0;
    points[1].width = 1.0;
    path_tiles[0].size = CTEX_EDITABLE_TILE_DEPENDENCY_DESCRIPTOR_CURRENT_SIZE;
    path_tiles[0].semantic_id = "base-color";
    path_tiles[1] = path_tiles[0];
    path_tiles[1].tile_x = 1;
    path.size = CTEX_EDITABLE_ENTRY_DESCRIPTOR_CURRENT_SIZE;
    path.identifier = "seam-line";
    path.kind = CTEX_EDITABLE_ENTRY_SURFACE_PATH;
    path.placement.normal.z = 1.0;
    path.placement.uniform_scale = 1.0;
    path.placement.axis_scale = (ctex_vec2d){1.0, 1.0};
    path.material_identity = "paint/thread";
    path.material_parameters = &parameter;
    path.material_parameter_count = 1;
    path.mesh_revision = 44;
    path.surface_points = points;
    path.surface_point_count = 2;
    path.dependent_tiles = path_tiles;
    path.dependent_tile_count = 2;
    info.size = CTEX_EDITABLE_ENTRY_INFO_CURRENT_SIZE;
    if (!expect(ctex_texture_set_editable_entry_add(document, texture_set_id, &path, &info, report,
                                                    sizeof(report)) == CTEX_RESULT_SUCCESS,
                "add editable surface path") ||
        !expect(info.surface_point_count == 2 && info.entry_count == 2,
                "surface path retains control points")) {
        goto fail;
    }

    if (!expect(ctex_stroke_settings_init(&stroke) == CTEX_RESULT_SUCCESS,
                "initialize stroke settings")) {
        goto fail;
    }
    stroke.spacing_fraction = 0.5;
    stroke_info.size = CTEX_RESOLVED_STROKE_INFO_CURRENT_SIZE;
    if (!expect(ctex_texture_set_editable_surface_path_resolve(
                    document, texture_set_id, "seam-line", &stroke, &stroke_info, NULL, 0,
                    &stamp_count, NULL, 0, &segment_count) == CTEX_RESULT_SUCCESS,
                "resolve editable surface path") ||
        !expect(stamp_count >= 2 && segment_count >= 1,
                "surface path resolves through stroke model")) {
        goto fail;
    }
    stroke_info.size = CTEX_RESOLVED_STROKE_INFO_CURRENT_SIZE;
    if (!expect(stamp_count <= 64 && segment_count <= 64, "resolved path fits test buffers") ||
        !expect(ctex_texture_set_editable_surface_path_resolve(
                    document, texture_set_id, "seam-line", &stroke, &stroke_info, stamps, 64,
                    &stamp_count, swept_segments, 64, &segment_count) == CTEX_RESULT_SUCCESS,
                "read resolved editable surface path") ||
        !expect(stamps[0].position.y == 0.0 && stamps[0].radius == 0.5,
                "resolved path begins at the original control point and width")) {
        goto fail;
    }

    points[0].position.y = 1.0;
    points[0].width = 0.75;
    parameter.value[0] = 0.6;
    path.expected_revision = 1;
    info.size = CTEX_EDITABLE_ENTRY_INFO_CURRENT_SIZE;
    if (!expect(ctex_texture_set_editable_entry_edit(document, texture_set_id, &path, &info, report,
                                                     sizeof(report)) == CTEX_RESULT_SUCCESS,
                "edit editable surface path") ||
        !expect(info.entry_revision == 2 && info.invalidated_tile_count == 2 &&
                    strstr(report, "\"values\":[0.600000]") != NULL &&
                    strstr(report, "\"width\":0.750000") != NULL,
                "path edit retains material and invalidates the previous raster tiles")) {
        goto fail;
    }
    stroke_info.size = CTEX_RESOLVED_STROKE_INFO_CURRENT_SIZE;
    if (!expect(ctex_texture_set_editable_surface_path_resolve(
                    document, texture_set_id, "seam-line", &stroke, &stroke_info, stamps, 64,
                    &stamp_count, swept_segments, 64, &segment_count) == CTEX_RESULT_SUCCESS,
                "resolve edited surface path") ||
        !expect(stamps[0].position.y == 1.0 && stamps[0].radius == 0.75,
                "position and width edits reach stroke reconstruction")) {
        goto fail;
    }

    info.size = CTEX_EDITABLE_ENTRY_INFO_CURRENT_SIZE;
    if (!expect(ctex_texture_set_editable_entry_undo(document, texture_set_id, &info, report,
                                                     sizeof(report)) == CTEX_RESULT_SUCCESS,
                "undo editable surface path") ||
        !expect(info.entry_revision == 1 && strstr(report, "\"values\":[0.350000]") != NULL &&
                    strstr(report, "\"width\":0.500000") != NULL,
                "undo restores the original path material and width")) {
        goto fail;
    }
    stroke_info.size = CTEX_RESOLVED_STROKE_INFO_CURRENT_SIZE;
    if (!expect(ctex_texture_set_editable_surface_path_resolve(
                    document, texture_set_id, "seam-line", &stroke, &stroke_info, stamps, 64,
                    &stamp_count, swept_segments, 64, &segment_count) == CTEX_RESULT_SUCCESS,
                "resolve restored surface path") ||
        !expect(stamps[0].position.y == 0.0 && stamps[0].radius == 0.5,
                "undo restores the original position and stroke width")) {
        goto fail;
    }

    info.size = CTEX_EDITABLE_ENTRY_INFO_CURRENT_SIZE;
    if (!expect(ctex_texture_set_editable_entry_plan_rasterization(
                    document, texture_set_id, "seam-line", &info, report, sizeof(report)) ==
                    CTEX_RESULT_SUCCESS,
                "plan explicit path rasterization") ||
        !expect(info.invalidated_tile_count == 2 && strstr(report, "rasterize") != NULL,
                "rasterization plan reports dependent tiles")) {
        goto fail;
    }

    if (!expect(ctex_project_container_create_empty(NULL, 0, &empty_size) == CTEX_RESULT_SUCCESS,
                "size empty project")) {
        goto fail;
    }
    empty_project = (unsigned char*)malloc(empty_size);
    if (!expect(empty_project != NULL, "allocate empty project") ||
        !expect(ctex_project_container_create_empty(empty_project, empty_size, &empty_size) ==
                    CTEX_RESULT_SUCCESS,
                "create empty project")) {
        goto fail;
    }
    saved_info.size = CTEX_PROJECT_CONTAINER_INFO_CURRENT_SIZE;
    if (!expect(ctex_project_container_upsert_editable_authoring(
                    empty_project, empty_size, NULL, document, texture_set_id, "set/body/editable",
                    &saved_info, NULL, 0, NULL, 0) == CTEX_RESULT_SUCCESS,
                "size project with editable authoring")) {
        goto fail;
    }
    saved_project = (unsigned char*)malloc(saved_info.canonical_size);
    project_report = (char*)malloc(saved_info.report_size);
    if (!expect(saved_project != NULL && project_report != NULL, "allocate saved project") ||
        !expect(ctex_project_container_upsert_editable_authoring(
                    empty_project, empty_size, NULL, document, texture_set_id, "set/body/editable",
                    &saved_info, saved_project, saved_info.canonical_size, project_report,
                    saved_info.report_size) == CTEX_RESULT_SUCCESS,
                "save editable authoring in project") ||
        !expect(strstr(project_report, "editable-authoring") != NULL,
                "project report names editable asset")) {
        goto fail;
    }

    if (!make_document(&reopened, reopened_set_id, sizeof(reopened_set_id))) goto fail;
    info.size = CTEX_EDITABLE_ENTRY_INFO_CURRENT_SIZE;
    if (!expect(ctex_project_container_restore_editable_authoring(
                    saved_project, saved_info.canonical_size, NULL, "set/body/editable", reopened,
                    reopened_set_id, &info, report, sizeof(report)) == CTEX_RESULT_SUCCESS,
                "restore editable authoring") ||
        !expect(info.entry_count == 2, "restore all editable entries")) {
        goto fail;
    }
    info.size = CTEX_EDITABLE_ENTRY_INFO_CURRENT_SIZE;
    if (!expect(
            ctex_texture_set_editable_entry_inspect(reopened, reopened_set_id, "title", &info,
                                                    report, sizeof(report)) == CTEX_RESULT_SUCCESS,
            "inspect reopened text") ||
        !expect(strstr(report, "CyberTexel") != NULL && strstr(report, "inter-bold-v4") != NULL,
                "text and font identity survive project reopen")) {
        goto fail;
    }
    text = text_descriptor("Reopened edit", 1, text_tiles, 1, &parameter);
    info.size = CTEX_EDITABLE_ENTRY_INFO_CURRENT_SIZE;
    if (!expect(ctex_texture_set_editable_entry_edit(reopened, reopened_set_id, &text, &info,
                                                     report, sizeof(report)) == CTEX_RESULT_SUCCESS,
                "edit reopened text") ||
        !expect(ctex_texture_set_editable_entry_undo(reopened, reopened_set_id, &info, report,
                                                     sizeof(report)) == CTEX_RESULT_SUCCESS,
                "undo reopened text edit") ||
        !expect(strstr(report, "CyberTexel") != NULL, "undo reopened edit restores saved text")) {
        goto fail;
    }

    free(project_report);
    free(saved_project);
    free(empty_project);
    ctex_document_destroy(reopened);
    ctex_document_destroy(document);
    return 0;

fail:
    free(project_report);
    free(saved_project);
    free(empty_project);
    ctex_document_destroy(reopened);
    ctex_document_destroy(document);
    return 1;
}
